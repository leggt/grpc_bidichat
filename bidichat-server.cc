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

#include <grpc/grpc.h>
#include <grpcpp/security/server_credentials.h>
#include <grpcpp/server.h>
#include <grpcpp/server_builder.h>
#include <grpcpp/server_context.h>
#include "bidichat.grpc.pb.h"

using grpc::CallbackServerContext;
using grpc::Server;
using grpc::ServerBuilder;
using grpc::Status;
using bidichat::Chat;
using std::chrono::system_clock;

class ChatImpl final : public Chat::CallbackService {

};

// class RouteGuideImpl final : public RouteGuide::CallbackService {
//  public:
//   explicit RouteGuideImpl(const std::string& db) {
//     routeguide::ParseDb(db, &feature_list_);
//   }

//   grpc::ServerUnaryReactor* GetFeature(CallbackServerContext* context,
//                                        const Point* point,
//                                        Feature* feature) override {
//     feature->set_name(GetFeatureName(*point, feature_list_));
//     feature->mutable_location()->CopyFrom(*point);
//     auto* reactor = context->DefaultReactor();
//     reactor->Finish(Status::OK);
//     return reactor;
//   }

//   grpc::ServerWriteReactor<Feature>* ListFeatures(
//       CallbackServerContext* context,
//       const routeguide::Rectangle* rectangle) override {
//     class Lister : public grpc::ServerWriteReactor<Feature> {
//      public:
//       Lister(const routeguide::Rectangle* rectangle,
//              const std::vector<Feature>* feature_list)
//           : left_((std::min)(rectangle->lo().longitude(),
//                              rectangle->hi().longitude())),
//             right_((std::max)(rectangle->lo().longitude(),
//                               rectangle->hi().longitude())),
//             top_((std::max)(rectangle->lo().latitude(),
//                             rectangle->hi().latitude())),
//             bottom_((std::min)(rectangle->lo().latitude(),
//                                rectangle->hi().latitude())),
//             feature_list_(feature_list),
//             next_feature_(feature_list_->begin()) {
//         NextWrite();
//       }
//       void OnDone() override { delete this; }
//       void OnWriteDone(bool /*ok*/) override { NextWrite(); }

//      private:
//       void NextWrite() {
//         while (next_feature_ != feature_list_->end()) {
//           const Feature& f = *next_feature_;
//           next_feature_++;
//           if (f.location().longitude() >= left_ &&
//               f.location().longitude() <= right_ &&
//               f.location().latitude() >= bottom_ &&
//               f.location().latitude() <= top_) {
//             StartWrite(&f);
//             return;
//           }
//         }
//         // Didn't write anything, all is done.
//         Finish(Status::OK);
//       }
//       const long left_;
//       const long right_;
//       const long top_;
//       const long bottom_;
//       const std::vector<Feature>* feature_list_;
//       std::vector<Feature>::const_iterator next_feature_;
//     };
//     return new Lister(rectangle, &feature_list_);
//   }

//   grpc::ServerReadReactor<Point>* RecordRoute(CallbackServerContext* context,
//                                               RouteSummary* summary) override {
//     class Recorder : public grpc::ServerReadReactor<Point> {
//      public:
//       Recorder(RouteSummary* summary, const std::vector<Feature>* feature_list)
//           : start_time_(system_clock::now()),
//             summary_(summary),
//             feature_list_(feature_list) {
//         StartRead(&point_);
//       }
//       void OnDone() { delete this; }
//       void OnReadDone(bool ok) {
//         if (ok) {
//           point_count_++;
//           if (!GetFeatureName(point_, *feature_list_).empty()) {
//             feature_count_++;
//           }
//           if (point_count_ != 1) {
//             distance_ += GetDistance(previous_, point_);
//           }
//           previous_ = point_;
//           StartRead(&point_);
//         } else {
//           summary_->set_point_count(point_count_);
//           summary_->set_feature_count(feature_count_);
//           summary_->set_distance(static_cast<long>(distance_));
//           auto secs = std::chrono::duration_cast<std::chrono::seconds>(
//               system_clock::now() - start_time_);
//           summary_->set_elapsed_time(secs.count());
//           Finish(Status::OK);
//         }
//       }

//      private:
//       system_clock::time_point start_time_;
//       RouteSummary* summary_;
//       const std::vector<Feature>* feature_list_;
//       Point point_;
//       int point_count_ = 0;
//       int feature_count_ = 0;
//       float distance_ = 0.0;
//       Point previous_;
//     };
//     return new Recorder(summary, &feature_list_);
//   }

//   grpc::ServerBidiReactor<RouteNote, RouteNote>* RouteChat(
//       CallbackServerContext* context) override {
//     class Chatter : public grpc::ServerBidiReactor<RouteNote, RouteNote> {
//      public:
//       Chatter(absl::Mutex* mu, std::vector<RouteNote>* received_notes)
//           : mu_(mu), received_notes_(received_notes) {
//         StartRead(&note_);
//       }
//       void OnDone() override { delete this; }
//       void OnReadDone(bool ok) override {
//         if (ok) {
//           // Unlike the other example in this directory that's not using
//           // the reactor pattern, we can't grab a local lock to secure the
//           // access to the notes vector, because the reactor will most likely
//           // make us jump threads, so we'll have to use a different locking
//           // strategy. We'll grab the lock locally to build a copy of the
//           // list of nodes we're going to send, then we'll grab the lock
//           // again to append the received note to the existing vector.
//           mu_->Lock();
//           std::copy_if(received_notes_->begin(), received_notes_->end(),
//                        std::back_inserter(to_send_notes_),
//                        [this](const RouteNote& note) {
//                          return note.location().latitude() ==
//                                     note_.location().latitude() &&
//                                 note.location().longitude() ==
//                                     note_.location().longitude();
//                        });
//           mu_->Unlock();
//           notes_iterator_ = to_send_notes_.begin();
//           NextWrite();
//         } else {
//           Finish(Status::OK);
//         }
//       }
//       void OnWriteDone(bool /*ok*/) override { NextWrite(); }

//      private:
//       void NextWrite() {
//         if (notes_iterator_ != to_send_notes_.end()) {
//           StartWrite(&*notes_iterator_);
//           notes_iterator_++;
//         } else {
//           mu_->Lock();
//           received_notes_->push_back(note_);
//           mu_->Unlock();
//           StartRead(&note_);
//         }
//       }
//       RouteNote note_;
//       absl::Mutex* mu_;
//       std::vector<RouteNote>* received_notes_;
//       std::vector<RouteNote> to_send_notes_;
//       std::vector<RouteNote>::iterator notes_iterator_;
//     };
//     return new Chatter(&mu_, &received_notes_);
//   }

//  private:
//   std::vector<Feature> feature_list_;
//   absl::Mutex mu_;
//   std::vector<RouteNote> received_notes_ ABSL_GUARDED_BY(mu_);
// };


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