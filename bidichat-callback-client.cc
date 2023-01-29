#include <condition_variable>
#include <iostream>
#include <mutex>
#include <string>
#include <thread>
#include <queue>
#include <condition_variable>

#include <grpc/grpc.h>
#include <grpcpp/alarm.h>
#include <grpcpp/channel.h>
#include <grpcpp/client_context.h>
#include <grpcpp/create_channel.h>
#include <grpcpp/security/credentials.h>
#include "protos/bidichat.grpc.pb.h"
#include "bidichat-callback-client.h"

namespace CallbackClient 
{
using bidichat::Chat;
using bidichat::Message;
using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;

  ChatClient::ChatClient(std::string server, std::string name)
      : stub_(Chat::NewStub( grpc::CreateChannel(server, grpc::InsecureChannelCredentials()))),
      client_name(name)
  {
    stub_->async()->Chat(&context_, this);
    StartRead(&current_read);
    StartCall();

    writer_thread = std::thread(&ChatClient::WriteThread, this);
  }

  ChatClient::~ChatClient()
  {
    std::lock_guard<std::mutex> guard(mtx);
    shutdown_flag = true;
    new_messages_cond.notify_one();

  }

  void ChatClient::OnReadDone(bool ok)
  {
    if (ok)
    {
      std::cout << "(on client " << client_name << ") message from " << current_read.name() << ": " << current_read.message() << std::endl;
      StartRead(&current_read);
    }
  }

  void ChatClient::WriteThread()
  {
    while (true)
    {
      std::unique_lock<std::mutex> lock(mtx);
      new_messages_cond.wait(lock, [this]
                             { return shutdown_flag or (!message_queue.empty() and !currently_writing); });


      if (shutdown_flag) {
        break;
      }

      current_write = message_queue.front();
      message_queue.pop();

      currently_writing = true;

      lock.unlock();

      StartWrite(&current_write);
    }
  }

  void ChatClient::OnWriteDone(bool ok)
  {
    if (ok)
    {
      std::lock_guard<std::mutex> guard(mtx);
      currently_writing = false;
      new_messages_cond.notify_one();
    }
  }

  void ChatClient::SendMessage(std::string message)
  {
    std::lock_guard<std::mutex> guard(mtx);
    Message new_message;
    new_message.set_name(client_name);
    new_message.set_message(message);
    message_queue.push(new_message);
    new_messages_cond.notify_one();
  }


}

/*
int main(int argc, char **argv)
{
  std::string client_name = "client";

  if (argc > 1)
  {
    client_name = argv[1];
  }

  ChatClient chatclient(
      grpc::CreateChannel("localhost:50051", grpc::InsecureChannelCredentials()),
      client_name);

  // Send some junk
  chatclient.SendMessage("sup");
  chatclient.SendMessage("yo");
  chatclient.SendMessage("what is grpc?");

  sleep(100);

  return 0;
}
*/