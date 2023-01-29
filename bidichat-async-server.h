#ifndef BIDICHATASYNCSERVER_H
#define BIDICHATASYNCSERVER_H

#include <iostream>
#include <string>
#include <thread>
#include <list>
#include <queue>
#include <condition_variable>

#include <grpc/grpc.h>
#include <grpcpp/security/server_credentials.h>
#include <grpcpp/server.h>
#include <grpcpp/server_builder.h>
#include <grpcpp/server_context.h>
#include "protos/bidichat.grpc.pb.h"

namespace AsyncServer {

using bidichat::Chat;
using bidichat::Message;
using grpc::CallbackServerContext;
using grpc::Server;
using grpc::ServerAsyncReaderWriter;
using grpc::ServerBuilder;
using grpc::ServerCompletionQueue;
using grpc::ServerContext;
using grpc::Status;


class CallData;
enum CallStatus
{
  REQUEST,
  CONNECTED,
  READ,
  WRITE,
  FINISH
};
/*
CallTag is passed to and returned from the grpc completionqueue. It is used to 
identify what was requested and completed. In most of the grpc examples I've 
seen this exists inside of the calldata which I think means that only one calldata 
can be in one state at a time. Here I've moved it outside of calldata so that 
we can be READing and WRITEing concurrently.
*/
struct CallTag
{
  CallTag(CallStatus status, CallData *calldata) : status(status), calldata(calldata){};
  CallStatus status;
  CallData *calldata;
};

class ChatServer
{
public:
  void HandleNewMessage(Message message);
  void Run();
  void AddClient(CallData *client) { client_list.push_back(client); }
  void RemoveClient(CallData *client) { client_list.remove(client); }

private:
  std::list<CallData *> client_list;
};

class CallData
{
public:
  CallData(Chat::AsyncService *service, ServerCompletionQueue *cq, ChatServer *server);
  void Proceed(CallTag *tag);
  void SendMessage(Message message);

private:
  Chat::AsyncService *service_;
  ServerCompletionQueue *cq_;
  ServerContext ctx_;
  Message current_read;
  ServerAsyncReaderWriter<Message, Message> responder_;
  ChatServer *server;
};
}
#endif