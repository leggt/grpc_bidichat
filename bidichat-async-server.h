#ifndef BIDICHATASYNCSERVER_H
#define BIDICHATASYNCSERVER_H

#include <list>
#include <queue>

#include <grpc/grpc.h>
#include <grpcpp/security/server_credentials.h>
#include <grpcpp/server.h>
#include <grpcpp/server_context.h>

#include "protos/bidichat.grpc.pb.h"

namespace AsyncServer {


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
  void HandleNewMessage(bidichat::Message message);
  void Run();
  void Shutdown();
  void AddClient(CallData *client) { client_list.push_back(client); }
  void RemoveClient(CallData *client) { client_list.remove(client); }

private:
  std::list<CallData *> client_list;
  std::unique_ptr<grpc::Server> server;
  std::unique_ptr<grpc::ServerCompletionQueue> cq; 
};

class CallData
{
public:
  CallData(bidichat::Chat::AsyncService *service, grpc::ServerCompletionQueue *cq, ChatServer *server);
  void Proceed(CallTag *tag);
  void SendMessage(bidichat::Message message);

private:
  bidichat::Chat::AsyncService *service_;
  grpc::ServerCompletionQueue *cq_;
  grpc::ServerContext ctx_;
  bidichat::Message current_read;
  grpc::ServerAsyncReaderWriter<bidichat::Message, bidichat::Message> responder_;
  ChatServer *server;
};
}
#endif