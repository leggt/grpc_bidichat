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
#include "bidichat.grpc.pb.h"

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

CallData::CallData(Chat::AsyncService *service, ServerCompletionQueue *cq, ChatServer *server)
    : service_(service), cq_(cq), responder_(&ctx_), server(server)
{
  CallTag *tag = new CallTag(CallStatus::REQUEST, this);
  Proceed(tag);
}

void CallData::Proceed(CallTag *tag)
{
  CallStatus status = tag->status;
  if (status == REQUEST)
  {
    tag->status = CallStatus::CONNECTED;
    service_->RequestChat(&ctx_, &responder_, cq_, cq_, (void *)tag);
  }
  else if (status == CONNECTED)
  {
    server->AddClient(this);

    tag->status = CallStatus::READ;
    responder_.Read(&current_read, tag);

    // Start listening for the next client
    new CallData(service_, cq_, server);
  }
  else if (status == READ)
  {
    server->HandleNewMessage(current_read);
    responder_.Read(&current_read, tag);
  }
}

void CallData::SendMessage(Message message)
{
  CallTag *tag = new CallTag(CallStatus::WRITE, this);

  responder_.Write(message, tag);
}

void ChatServer::HandleNewMessage(Message message)
{
  for (auto client : client_list)
  {
    client->SendMessage(message);
  }
}

void ChatServer::Run()
{
  std::string server_address("0.0.0.0:50051");
  Chat::AsyncService service;

  ServerBuilder builder;
  builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
  builder.RegisterService(&service);
  std::unique_ptr<grpc::ServerCompletionQueue> cq = builder.AddCompletionQueue();
  std::unique_ptr<Server> server(builder.BuildAndStart());
  std::cout << "Server listening on " << server_address << std::endl;

  new CallData(&service, cq.get(), this);

  while (true)
  {
    void *tag;
    bool ok;
    GPR_ASSERT(cq->Next(&tag, &ok));

    CallTag *ct = static_cast<CallTag *>(tag);
    CallData *calldata = ct->calldata;
    if (ok)
    {
      calldata->Proceed(ct);
    }
    else
    {
      RemoveClient(calldata);
      delete calldata;
      delete ct;
    }
  }
}

int main(int argc, char **argv)
{
  ChatServer server;
  server.Run();

  return 0;
}
