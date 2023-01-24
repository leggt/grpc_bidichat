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
#include <queue>
#include <condition_variable>

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

class ClientHandler;

// ChatImpl extends some grpc 'CallbackService'. grpc calls into the 'Chat' method when a new client connects
// I have chosen to use this class to maintain a list of all the connected clients and have added methods here 
// for communicating between them
class ChatServer final : public Chat::CallbackService {
  public:
    void AddClient(ClientHandler* client);
    void NewMessage(Message message);
  private:
    // grpc calls this method when a new client connects
    grpc::ServerBidiReactor<Message,Message>* Chat(CallbackServerContext* context) ;
    std::mutex mtx;
    std::list <ClientHandler*> client_list;
};

// One instance of this per client. This is what sends and receives messages to that specific client
class ClientHandler : public grpc::ServerBidiReactor<Message, Message> {
  public:
    ClientHandler(ChatServer* server);
    void OnDone() override { delete this; }

    void OnWriteDone(bool ok) override; 
    void OnReadDone(bool ok) override; 

    void SendNewMessage(Message new_message);

    void WriteThread();

  private:
    std::thread writer_thread;
    Message current_write;
    Message current_read;
    ChatServer *server;
    std::queue<Message> message_queue;
    std::condition_variable new_messages_cond;
    std::mutex mtx;
    bool currently_writing;
};


ClientHandler::ClientHandler(ChatServer* server):
server(server)
{
  StartRead(&current_read);
  writer_thread = std::thread(&ClientHandler::WriteThread,this);
}

void ClientHandler::WriteThread() {
  while (true)
  {
  std::unique_lock<std::mutex> lock(mtx);
  new_messages_cond.wait(lock, 
    [this] { return !message_queue.empty() and !currently_writing; });

  current_write = message_queue.front();
  message_queue.pop();

  lock.unlock();

  currently_writing=true;

  StartWrite(&current_write);
  
  }

}

void ClientHandler::OnWriteDone(bool ok) {
  if (ok){
    std::lock_guard<std::mutex> guard(mtx);
    currently_writing=false;
    new_messages_cond.notify_one();
  }
}


void ClientHandler::OnReadDone(bool ok) { 
  if (ok) 
  {
    server->NewMessage(current_read);
    StartRead(&current_read);
  } else {
    Finish(Status::OK);
  }
}

void ClientHandler::SendNewMessage(Message new_message)
{
  std::lock_guard<std::mutex> guard(mtx);
  message_queue.push(new_message);
  new_messages_cond.notify_one();
}

void ChatServer::AddClient(ClientHandler* client){
  std::lock_guard<std::mutex> guard(mtx);
  client_list.push_back(client);
}

void ChatServer::NewMessage(Message message){
  std::lock_guard<std::mutex> guard(mtx);
  for (auto client : client_list) {
    client->SendNewMessage(message);
  }
}

grpc::ServerBidiReactor<Message,Message>* ChatServer::Chat(CallbackServerContext* context) {
  ClientHandler* client = new ClientHandler(this);
  AddClient(client);
  return client;
}


int main(int argc, char** argv) {
  std::string server_address("0.0.0.0:50051");
  ChatServer service;

  ServerBuilder builder;
  builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
  builder.RegisterService(&service);
  std::unique_ptr<Server> server(builder.BuildAndStart());
  std::cout << "Server listening on " << server_address << std::endl;
  server->Wait();

  return 0;
}
