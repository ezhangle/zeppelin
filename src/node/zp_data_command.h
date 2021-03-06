#ifndef ZP_KV_H
#define ZP_KV_H

#include "zp_command.h"

////// kv //////
class SetCmd : public Cmd {
 public:
  SetCmd(int flag) : Cmd(flag) {}
  virtual std::string name() const override {
    return "Set"; 
  }
  virtual void Do(const google::protobuf::Message *req,
      google::protobuf::Message *res, void* partition) const;
  virtual std::string ExtractTable(const google::protobuf::Message *req) const {
    const client::CmdRequest* request = static_cast<const client::CmdRequest*>(req);
    return request->set().table_name();
  }
  virtual std::string ExtractKey(const google::protobuf::Message *req) const {
    const client::CmdRequest* request = static_cast<const client::CmdRequest*>(req);
    return request->set().key();
  }
};

class GetCmd : public Cmd {
 public:
  GetCmd(int flag) : Cmd(flag) {}
  virtual std::string name() const override {
    return "Get"; 
  }
  virtual void Do(const google::protobuf::Message *req,
      google::protobuf::Message *res, void* partition) const;
  virtual std::string ExtractTable(const google::protobuf::Message *req) const {
    const client::CmdRequest* request = static_cast<const client::CmdRequest*>(req);
    return request->get().table_name();
  }
  virtual std::string ExtractKey(const google::protobuf::Message *req) const {
    const client::CmdRequest* request = static_cast<const client::CmdRequest*>(req);
    return request->get().key();
  }
};

class DelCmd : public Cmd {
 public:
  DelCmd(int flag) : Cmd(flag) {}
  virtual std::string name() const override {
    return "Del"; 
  }
  virtual void Do(const google::protobuf::Message *req,
      google::protobuf::Message *res, void* partition) const;
  virtual std::string ExtractTable(const google::protobuf::Message *req) const {
    const client::CmdRequest* request = static_cast<const client::CmdRequest*>(req);
    return request->del().table_name();
  }
  virtual std::string ExtractKey(const google::protobuf::Message *req) const {
    const client::CmdRequest* request = static_cast<const client::CmdRequest*>(req);
    return request->del().key();
  }
};

////// Info Cmds /////
class InfoCmd : public Cmd {
 public:
  InfoCmd(int flag) : Cmd(flag) {}
  virtual std::string name() const override {
    return "Info"; 
  }
  virtual void Do(const google::protobuf::Message *req,
      google::protobuf::Message *res, void* parition = NULL) const;
  virtual std::string ExtractTable(const google::protobuf::Message *req) const {
    const client::CmdRequest* request = static_cast<const client::CmdRequest*>(req);
    if (request->has_info() && request->info().has_table_name()) {
      return request->info().table_name();
    }
    return "";
  }
};

////// Sync //////
class SyncCmd : public Cmd {
 public:
  SyncCmd(int flag) : Cmd(flag) {}
  virtual std::string name() const override {
    return "Sync"; 
  }
  virtual void Do(const google::protobuf::Message *req,
      google::protobuf::Message *res, void* partition) const;
  virtual std::string ExtractTable(const google::protobuf::Message *req) const {
    const client::CmdRequest* request = static_cast<const client::CmdRequest*>(req);
    return request->sync().table_name();
  }
  virtual int ExtractPartition(const google::protobuf::Message *req) const {
    const client::CmdRequest* request = static_cast<const client::CmdRequest*>(req);
    return request->sync().sync_offset().partition();
  }
};

#endif
