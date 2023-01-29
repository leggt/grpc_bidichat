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
#include "bidichat.grpc.pb.h"

using bidichat::Chat;
using bidichat::Message;
using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;

class ChatClient : public grpc::ClientBidiReactor<Message, Message>
{
public:
  ChatClient(std::shared_ptr<Channel> channel, std::string name)
      : stub_(Chat::NewStub(channel)), client_name(name)
  {
    stub_->async()->Chat(&context_, this);
    StartRead(&current_read);
    StartCall();

    writer_thread = std::thread(&ChatClient::WriteThread, this);
  }

  void OnReadDone(bool ok)
  {
    if (ok)
    {
      std::cout << "(on client " << client_name << ") message from " << current_read.name() << ": " << current_read.message() << std::endl;
      StartRead(&current_read);
    }
  }

  void WriteThread()
  {
    while (true)
    {
      std::unique_lock<std::mutex> lock(mtx);
      new_messages_cond.wait(lock, [this]
                             { return !message_queue.empty() and !currently_writing; });

      current_write = message_queue.front();
      message_queue.pop();

      currently_writing = true;

      lock.unlock();

      StartWrite(&current_write);
    }
  }

  void OnWriteDone(bool ok)
  {
    if (ok)
    {
      std::lock_guard<std::mutex> guard(mtx);
      currently_writing = false;
      new_messages_cond.notify_one();
    }
  }

  void SendMessage(std::string message)
  {
    std::lock_guard<std::mutex> guard(mtx);
    Message new_message;
    new_message.set_name(client_name);
    new_message.set_message(message);
    message_queue.push(new_message);
    new_messages_cond.notify_one();
  }

private:
  ClientContext context_;
  std::unique_ptr<Chat::Stub> stub_;

  std::thread writer_thread;

  std::string client_name;
  Message current_read;
  Message current_write;
  std::queue<Message> message_queue;
  std::condition_variable new_messages_cond;
  std::mutex mtx;
  bool currently_writing;
};

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