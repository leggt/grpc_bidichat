
#include <chrono>
#include <condition_variable>
#include <iostream>
#include <memory>
#include <mutex>
#include <random>
#include <string>
#include <thread>


#include <grpc/grpc.h>
#include <grpcpp/alarm.h>
#include <grpcpp/channel.h>
#include <grpcpp/client_context.h>
#include <grpcpp/create_channel.h>
#include <grpcpp/security/credentials.h>
#include "bidichat.grpc.pb.h"

using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;
using bidichat::Chat;
using bidichat::Message;

class Chatter : public grpc::ClientBidiReactor<Message,Message>
{
  public:
  explicit Chatter(Chat::Stub* stub) {
        stub->async()->Chat(&context_, this);
        NextWrite();
        StartRead(&current_read);
        StartCall();
  }
      Status Await() {
        std::unique_lock<std::mutex> l(mu_);
        cv_.wait(l, [this] { return done_; });
        return std::move(status_);
      }
  private:
      void NextWrite() {
        current_write.set_message("Sup");
        current_write.set_name("Tim");
        StartWrite(&current_write);
      };
      ClientContext context_;
      std::mutex mu_;
      std::condition_variable cv_;
      Status status_;
      bool done_ = false;
      Message current_read;
      Message current_write;
};

class ChatClient {
  public:
  ChatClient(std::shared_ptr<Channel> channel)
      : stub_(Chat::NewStub(channel)) {
      }

  void doChat() {
    Chatter chatter(stub_.get());

    Status status = chatter.Await();
    if (!status.ok()) {
      std::cout << "RouteChat rpc failed." << std::endl;
    }
  }

  private:
  std::unique_ptr<Chat::Stub> stub_;

};

int main(int argc, char** argv) {
  // Expect only arg: --db_path=path/to/route_guide_db.json.
  ChatClient chatclient(
      grpc::CreateChannel("localhost:50051",
                          grpc::InsecureChannelCredentials())
      );

  std::thread chatter(&ChatClient::doChat,&chatclient);
  // while(true) {
  //   std::string str_msg;
  //   std::cin >> str_msg;
  //   std::cout << "Sending message: " << str_msg << std::endl;
  // }

  chatter.join();




  return 0;
}
