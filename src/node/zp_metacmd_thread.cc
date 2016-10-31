#include "zp_metacmd_thread.h"

#include <glog/logging.h>
#include "zp_data_server.h"

#include "zp_command.h"
#include "zp_admin.h"

extern ZPDataServer* zp_data_server;

ZPMetacmdThread::ZPMetacmdThread() {
  cli_ = new pink::PbCli();
  cli_->set_connect_timeout(1500);
}

ZPMetacmdThread::~ZPMetacmdThread() {
  Stop();
  delete cli_;
  LOG(INFO) << "ZPMetacmd thread " << thread_id() << " exit!!!";
}

void ZPMetacmdThread::MetaUpdateTask() {
  int64_t receive_epoch = 0;
  if (!zp_data_server->ShouldPullMeta()) {
    return;
  }

  if (FetchMetaInfo(receive_epoch)) {
    // When we fetch OK, we will FinishPullMeta
    zp_data_server->FinishPullMeta(receive_epoch);
  } else {
    // Sleep and try again
    sleep(kMetacmdInterval);
    zp_data_server->AddMetacmdTask();
  }
}

pink::Status ZPMetacmdThread::Send() {
  ZPMeta::MetaCmd request;

  DLOG(INFO) << "MetacmdThead Pull MetaServer(" << zp_data_server->meta_ip() << ":"
    << zp_data_server->meta_port() + kMetaPortShiftCmd
    << ") with local("<< zp_data_server->local_ip() << ":" << zp_data_server->local_port() << ")";
  request.set_type(ZPMeta::MetaCmd_Type::MetaCmd_Type_PULL);
  return cli_->Send(&request);
}

pink::Status ZPMetacmdThread::Recv(int64_t &receive_epoch) {
  pink::Status result;
  ZPMeta::MetaCmdResponse response;
  result = cli_->Recv(&response); 
  DLOG(INFO) << "MetacmdThread recv: " << result.ToString();
  if (result.ok()) {
    switch (response.type()) {
      case ZPMeta::MetaCmdResponse_Type::MetaCmdResponse_Type_PULL: {
        if (response.status().code() != ZPMeta::StatusCode::kOk) {
          DLOG(INFO) << "receive Pull error: " << response.status().msg();
          return pink::Status::IOError(response.status().msg());
        }

        receive_epoch = response.pull().version();
        ZPMeta::MetaCmdResponse_Pull pull = response.pull();

        DLOG(INFO) << "receive Pull message, will handle " << pull.info_size() << " Partitions.";
        for (int i = 0; i < pull.info_size(); i++) {
          const ZPMeta::Partitions& partition = pull.info(i);
          DLOG(INFO) << " - handle Partition " << partition.id() << ": master is " << partition.master().ip() << ":" << partition.master().port();

          Node master_node(partition.master().ip(), partition.master().port());
          if (master_node.empty()) {
            // No master patitions, simply ignore
            continue;
          }
          std::vector<Node> slave_nodes;
          for (int j = 0; j < partition.slaves_size(); j++) {
            slave_nodes.push_back(Node(partition.slaves(j).ip(), partition.slaves(j).port()));
          }

          bool result = zp_data_server->UpdateOrAddPartition(partition.id(), master_node, slave_nodes);
          if (!result) {
            LOG(WARNING) << "AddPartition failed";
          }
        }

        break;
      }
      default:
        break;
    }
  }
  return result;
}

bool ZPMetacmdThread::FetchMetaInfo(int64_t &receive_epoch) {
  pink::Status s;
  // No more PickMeta, which should be done by ping thread
  assert(!zp_data_server->meta_ip().empty() && zp_data_server->meta_port() != 0);
  DLOG(INFO) << "MetacmdThread will connect ("<< zp_data_server->meta_ip() << ":" << zp_data_server->meta_port() + kMetaPortShiftCmd << ")";
  s = cli_->Connect(zp_data_server->meta_ip(), zp_data_server->meta_port() + kMetaPortShiftCmd);
  if (s.ok()) {
    DLOG(INFO) << "Metacmd connect ("<< zp_data_server->meta_ip() << ":" << zp_data_server->meta_port() + kMetaPortShiftCmd << ") ok!";
    cli_->set_send_timeout(1000);
    cli_->set_recv_timeout(1000);

    s = Send();
    if (!s.ok()) {
      DLOG(WARNING) << "Metacmd send failed: " << s.ToString();
      cli_->Close();
      return false;
    }
    DLOG(INFO) << "Metacmd send ok!";

    s = Recv(receive_epoch);
    if (!s.ok()) {
      DLOG(WARNING) << "Metacmd recv failed: " << s.ToString();
      cli_->Close();
      return false;
    }
    DLOG(INFO) << "Metacmd MetaServer success";
    cli_->Close();
    return true;
  } else {
    DLOG(WARNING) << "Metacmd connect failed: " << s.ToString();
    return false;
  }
}
