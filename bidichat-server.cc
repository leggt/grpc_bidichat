/*
 *
 * Copyright 2021 gRPC authors.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

#include <algorithm>
#include <chrono>
#include <cmath>
#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <list>

#include <grpc/grpc.h>
#include <grpcpp/security/server_credentials.h>
#include <grpcpp/server.h>
#include <grpcpp/server_builder.h>
#include <grpcpp/server_context.h>
#include "bidichat.grpc.pb.h"
// #include "bidichat-server.h"

using grpc::CallbackServerContext;
using grpc::Server;
using grpc::ServerBuilder;
using grpc::Status;
using bidichat::Chat;
using bidichat::Message;
using std::chrono::system_clock;

class ChatImpl;

class ServerChatter : public grpc::ServerBidiReactor<Message, Message> {
  public:
  ServerChatter(ChatImpl* server):
  server(server)
  {
    StartRead(&current_read);
  }
  void OnDone() override { delete this; }
  // void OnWriteDone(bool /*ok*/) override { NextWrite(); }
  void OnReadDone(bool ok) override { 
    if (ok) 
    {
      StartRead(&current_read);
    } else {
      Finish(Status::OK);
    }
   }
  private:
  Message current_write;
  Message current_read;
  ChatImpl* server;
};

class ChatImpl final : public Chat::CallbackService {
  grpc::ServerBidiReactor<Message,Message>* Chat(CallbackServerContext* context) {
    return new ServerChatter(this);
  }
  private:
  std::mutex mtx;
  std::list <ServerChatter*> client_list;
};

int main(int argc, char** argv) {
  std::string server_address("0.0.0.0:50051");
  ChatImpl service;

  ServerBuilder builder;
  builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
  builder.RegisterService(&service);
  std::unique_ptr<Server> server(builder.BuildAndStart());
  std::cout << "Server listening on " << server_address << std::endl;
  server->Wait();

  return 0;
}
