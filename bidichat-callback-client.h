#ifndef BIDICHATCALLBACKCLIENT_H
#define BIDICHATCALLBACKCLIENT_H

#include <string>
#include <queue>
#include <thread>
#include <condition_variable>

#include <grpc/grpc.h>
#include <grpcpp/channel.h>
#include <grpcpp/client_context.h>
#include "protos/bidichat.grpc.pb.h"

namespace CallbackClient
{
class ChatClient : public grpc::ClientBidiReactor<bidichat::Message, bidichat::Message>
{
public:
  ChatClient(std::string server, std::string name);
  ~ChatClient();
  void OnReadDone(bool ok);
  void WriteThread();
  void OnWriteDone(bool ok);
  void SendMessage(std::string message);

private:
  grpc::ClientContext context_;
  std::unique_ptr<bidichat::Chat::Stub> stub_;

  std::thread writer_thread;

  std::string client_name;
  bidichat::Message current_read;
  bidichat::Message current_write;
  std::queue<bidichat::Message> message_queue;
  std::condition_variable new_messages_cond;
  std::mutex mtx;
  bool currently_writing;
  bool shutdown_flag;
};
}
#endif