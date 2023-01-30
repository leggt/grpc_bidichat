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

#include "bidichat-async-server.h"

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

void ChatServer::Shutdown()
{
  server->Shutdown();
  cq->Shutdown();
}

void ChatServer::Run()
{
  std::string server_address("0.0.0.0:50051");
  Chat::AsyncService service;

  ServerBuilder builder;
  builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
  builder.RegisterService(&service);
  cq = builder.AddCompletionQueue();
  server = builder.BuildAndStart();
  std::cout << "Server listening on " << server_address << std::endl;

  new CallData(&service, cq.get(), this);

  while (true)
  {
    void *tag;
    bool ok;
    if(!cq->Next(&tag, &ok))
    {
      break;
    }

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
      break;
    }
  }
}
}

/*
int main(int argc, char **argv)
{
  ChatServer server;
  server.Run();

  return 0;
}
*/