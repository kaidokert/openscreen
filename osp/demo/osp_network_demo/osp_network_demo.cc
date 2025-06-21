// Copyright 2025 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <atomic>
#include <chrono>
#include <future>
#include <iostream>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "osp/public/connect_request.h"
#include "osp/public/message_demuxer.h"
#include "osp/public/network_service_manager.h"
#include "osp/public/protocol_connection_client_factory.h"
#include "osp/public/protocol_connection_server_factory.h"
#include "osp/public/protocol_connection_service_observer.h"
#include "osp/public/service_config.h"
#include "osp/public/service_listener_factory.h"
#include "osp/public/service_publisher_factory.h"
#include "platform/api/network_interface.h"
#include "platform/api/time.h"
#include "platform/impl/logging.h"
#include "platform/impl/platform_client_posix.h"

namespace openscreen::osp {

using openscreen::Clock;
using openscreen::Error;
using openscreen::ErrorOr;
using openscreen::GetNetworkInterfaces;
using openscreen::InterfaceInfo;
using openscreen::PlatformClientPosix;

static constexpr char kInstanceName[] = "SimpleConnectionExample";

// Helper function for network interface setup
std::vector<InterfaceInfo> GetNonLoopbackInterfaces() {
  std::vector<InterfaceInfo> interfaces;
  for (const InterfaceInfo& interface : GetNetworkInterfaces()) {
    if (!interface.addresses.empty() &&
        interface.type != InterfaceInfo::Type::kLoopback) {
      interfaces.push_back(interface);
    }
  }
  return interfaces;
}

// Helper functions for openscreen::msgs::PresentationConnectionMessage
void CreateStringMessage(openscreen::msgs::PresentationConnectionMessage& pcm,
                         const std::string& message) {
  pcm.message.which =
      openscreen::msgs::PresentationConnectionMessage::Message::Which::kString;
  new (&pcm.message.str) std::string(message);
}

std::optional<std::string> ParseStringMessage(const uint8_t* buffer,
                                              size_t buffer_size) {
  openscreen::msgs::PresentationConnectionMessage pcm;
  auto decode_result = openscreen::msgs::DecodePresentationConnectionMessage(
      buffer, buffer_size, pcm);
  if (decode_result < 0) {
    return std::nullopt;
  }

  if (pcm.message.which == openscreen::msgs::PresentationConnectionMessage::
                               Message::Which::kString) {
    return pcm.message.str;
  }

  return std::nullopt;
}

class ServerHandler final : public ProtocolConnectionServiceObserver,
                            public MessageDemuxer::MessageCallback {
 public:
  explicit ServerHandler(std::promise<bool>* promise = nullptr)
      : promise_(promise) {}

  // ProtocolConnectionServiceObserver
  void OnRunning() override { std::cout << "Server: running\n"; }
  void OnStopped() override { std::cout << "Server: stopped\n"; }
  void OnSuspended() override {}
  void OnMetrics(const NetworkMetrics&) override {}
  void OnError(const Error& e) override {
    std::cerr << "Server error: " << e << "\n";
  }
  void OnIncomingConnection(std::unique_ptr<ProtocolConnection> c) override {
    std::cout << "SERVER: Incoming connection established, instance_id="
              << c->GetInstanceID() << " connection_id=" << c->GetID() << "\n";

    // Store connection for later use (echoing responses)
    uint64_t instance_id = c->GetInstanceID();
    connections_.push_back(std::move(c));

    // Create response connection that will stay alive for echoes
    echo_conn_ = NetworkServiceManager::Get()
                     ->GetProtocolConnectionServer()
                     ->CreateProtocolConnection(instance_id);

    if (!echo_conn_) {
      std::cout << "SERVER: Failed to create response connection\n";
    }

    std::cout << "SERVER: Waiting for client message...\n";
  }

  // Method to send echo response
  void SendEcho(uint64_t instance_id, const std::string& original_message) {
    if (!echo_conn_) {
      std::cerr << "SERVER: No response connection available for echo\n";
      return;
    }

    std::string echo_msg = "ECHO: " + original_message;
    std::cout << "SERVER: Sending echo: '" << echo_msg << "'\n";

    openscreen::msgs::PresentationConnectionMessage pcm;
    CreateStringMessage(pcm, echo_msg);
    auto result = echo_conn_->WriteMessage(
        pcm, openscreen::msgs::EncodePresentationConnectionMessage);
    if (!result.ok()) {
      std::cerr << "SERVER: Failed to send echo: " << result << "\n";
    }
  }

  // MessageDemuxer::MessageCallback
  ErrorOr<size_t> OnStreamMessage(uint64_t instance_id,
                                  uint64_t connection_id,
                                  openscreen::msgs::Type message_type,
                                  const uint8_t* buffer,
                                  size_t buffer_size,
                                  Clock::time_point now) override {
    if (message_type !=
        openscreen::msgs::Type::kPresentationConnectionMessage) {
      std::cout << "SERVER: Received unknown message type: "
                << static_cast<int>(message_type) << "\n";
      return buffer_size;
    }

    auto message = ParseStringMessage(buffer, buffer_size);
    if (!message) {
      std::cout << "SERVER: Failed to decode or received non-string message\n";
      return openscreen::Error::Code::kCborParsing;
    }

    std::cout << "SERVER: Received message: '" << *message
              << "' from connection_id=" << connection_id << "\n";

    // Send echo response
    SendEcho(instance_id, *message);

    // Schedule server completion after delay to allow echo to be delivered
    auto& task_runner = PlatformClientPosix::GetInstance()->GetTaskRunner();
    task_runner.PostTaskWithDelay([this]() { SetCompleted(); },
                                  std::chrono::milliseconds(250));

    return buffer_size;  // Return consumed bytes
  }

  std::vector<std::unique_ptr<ProtocolConnection>> connections_;

 private:
  void SetCompleted() {
    if (!promise_set_) {
      promise_set_ = true;
      if (promise_) {
        promise_->set_value(true);
      }
    }
  }

  std::unique_ptr<ProtocolConnection> echo_conn_;
  std::promise<bool>* promise_;
  std::atomic<bool> promise_set_{false};
};

class ClientHandler final : public ProtocolConnectionServiceObserver,
                            public ServiceListener::Observer,
                            public ConnectRequestCallback,
                            public ProtocolConnection::Observer,
                            public MessageDemuxer::MessageCallback {
 public:
  explicit ClientHandler(std::string_view target_instance,
                         std::promise<bool>* promise)
      : target_instance_(target_instance), promise_(promise) {}

  // ProtocolConnectionServiceObserver
  void OnRunning() override {}  // Not used in this example
  void OnStopped() override {}  // Not used in this example
  void OnSuspended() override {}
  void OnMetrics(const NetworkMetrics&) override {}
  void OnError(const Error& e) override {
    std::cerr << "Client error: " << e << "\n";
    SetError();
  }
  void OnIncomingConnection(
      std::unique_ptr<ProtocolConnection> connection) override {
    std::cout << "CLIENT: Received incoming connection from server\n";

    // Store the incoming connection so it stays alive
    incoming_connections_.push_back(std::move(connection));
  }

  // ServiceListener::Observer
  void OnStarted() override {}
  void OnSearching() override {}

  void OnReceiverAdded(const ServiceInfo& info) override {
    std::cout << "Client: found receiver: " << info.instance_name << "\n";

    if (info.instance_name == target_instance_ && !connected_) {
      connected_ = true;

      bool connect_result =
          client_->Connect(info.instance_name, connect_request_, this);
      if (!connect_result) {
        std::cerr << "Client: connect request failed\n";
        SetError();
      }
    }
  }

  void OnReceiverChanged(const ServiceInfo&) override {}
  void OnReceiverRemoved(const ServiceInfo&) override {}
  void OnAllReceiversRemoved() override {}

  // ConnectRequestCallback
  void OnConnectSucceed(uint64_t request_id,
                        std::string_view instance_name,
                        uint64_t instance_id) override {
    std::cout << "SUCCESS: OSP connection established to " << instance_name
              << "\n";

    auto connection = client_->CreateProtocolConnection(instance_id);
    if (!connection) {
      std::cerr << "ERROR: Failed to create protocol connection\n";
      SetError();
      return;
    }

    connection_ = std::move(connection);
    connection_->SetObserver(this);

    // Send a test message
    const char* msg = "Hello from client!";
    std::cout << "CLIENT: Sending message: '" << msg << "'\n";

    openscreen::msgs::PresentationConnectionMessage pcm;
    CreateStringMessage(pcm, msg);
    auto result = connection_->WriteMessage(
        pcm, openscreen::msgs::EncodePresentationConnectionMessage);
    if (!result.ok()) {
      std::cerr << "CLIENT: Failed to send message: " << result << "\n";
    }
  }

  void OnConnectFailed(uint64_t request_id,
                       std::string_view instance_name) override {
    std::cout << "ERROR: Connection failed to " << instance_name << "\n";
    SetError();
  }

  // ProtocolConnection::Observer
  void OnConnectionClosed(const ProtocolConnection&) override {
    std::cout << "CLIENT: Connection closed by server\n";
    SetCompleted();
  }

  // MessageDemuxer::MessageCallback - handle echo responses
  ErrorOr<size_t> OnStreamMessage(uint64_t instance_id,
                                  uint64_t connection_id,
                                  openscreen::msgs::Type message_type,
                                  const uint8_t* buffer,
                                  size_t buffer_size,
                                  Clock::time_point now) override {
    if (message_type !=
        openscreen::msgs::Type::kPresentationConnectionMessage) {
      std::cout << "CLIENT: Received unknown message type: "
                << static_cast<int>(message_type) << "\n";
      return buffer_size;
    }

    auto message = ParseStringMessage(buffer, buffer_size);
    if (!message) {
      std::cout << "CLIENT: Failed to decode or received non-string message\n";
      return openscreen::Error::Code::kCborParsing;
    }

    std::cout << "CLIENT: Received echo response: '" << *message << "'\n";

    // Signal completion for client teardown
    SetCompleted();

    return buffer_size;  // Return consumed bytes
  }

  void SetClient(ProtocolConnectionClient* client) { client_ = client; }

 private:
  void SetCompleted() {
    if (!promise_set_) {
      promise_set_ = true;
      promise_->set_value(true);
    }
  }

  void SetError() {
    if (!promise_set_) {
      promise_set_ = true;
      promise_->set_value(false);
    }
  }

  std::string target_instance_;
  ProtocolConnectionClient* client_{nullptr};
  std::unique_ptr<ProtocolConnection> connection_;
  std::promise<bool>* promise_;
  bool connected_{false};
  std::atomic<bool> promise_set_{false};
  ConnectRequest connect_request_;
  std::vector<std::unique_ptr<ProtocolConnection>> incoming_connections_;
};

void RunServer() {
  constexpr uint16_t kPort = 9988;
  auto& runner = PlatformClientPosix::GetInstance()->GetTaskRunner();

  std::promise<bool> promise;
  auto completion_future = promise.get_future();

  ServiceConfig config;
  config.instance_name = kInstanceName;

  ServicePublisher::Config pub_config;
  pub_config.instance_name = kInstanceName;
  pub_config.connection_server_port = kPort;

  // Configure interfaces for service publishing and server endpoints
  auto interfaces = GetNonLoopbackInterfaces();
  for (const InterfaceInfo& interface : interfaces) {
    pub_config.network_interfaces.push_back(interface);
    config.connection_endpoints.push_back(
        {interface.addresses[0].address, kPort});
  }
  if (interfaces.empty()) {
    std::cerr
        << "No network interfaces had usable addresses for mDNS publishing.\n";
  }

  auto observer = std::make_unique<ServerHandler>(&promise);
  auto server = ProtocolConnectionServerFactory::Create(
      config, *observer, runner, MessageDemuxer::kDefaultBufferLimit);

  // Set up message handler with completion promise
  auto& demuxer = server->GetMessageDemuxer();
  auto watch = demuxer.SetDefaultMessageTypeWatch(
      openscreen::msgs::Type::kPresentationConnectionMessage, observer.get());

  // Calculate and publish certificate fingerprint and authentication token.
  pub_config.fingerprint = server->GetAgentFingerprint();
  OSP_CHECK(!pub_config.fingerprint.empty());
  pub_config.auth_token = server->GetAuthToken();
  OSP_CHECK(!pub_config.auth_token.empty());

  auto publisher = ServicePublisherFactory::Create(pub_config, runner);

  auto* network_service = NetworkServiceManager::Create(
      nullptr, std::move(publisher), nullptr, std::move(server));
  auto* server_endpoint = network_service->GetProtocolConnectionServer();
  auto* service_publisher = network_service->GetServicePublisher();

  service_publisher->Start();
  server_endpoint->Start();

  std::cout << "Server listening on port " << kPort
            << " (instance: " << kInstanceName << ")\n";

  // Wait for completion with timeout
  if (completion_future.wait_for(std::chrono::seconds(10)) ==
      std::future_status::ready) {
    bool success = completion_future.get();
    std::cout << "Server completed "
              << (success ? "successfully" : "with failure") << "\n";
  } else {
    std::cout << "Server timed out after 10 seconds\n";
  }

  // service_publisher->Stop();
  // server_endpoint->Stop();
  // NetworkServiceManager::Dispose();
}

void RunClient() {
  auto& runner = PlatformClientPosix::GetInstance()->GetTaskRunner();

  std::promise<bool> promise;
  auto completion_future = promise.get_future();

  // Configure interfaces for service discovery
  ServiceListener::Config lis_config;
  lis_config.network_interfaces = GetNonLoopbackInterfaces();
  if (lis_config.network_interfaces.empty()) {
    std::cerr
        << "No network interfaces had usable addresses for mDNS listening.\n";
  }
  auto listener = ServiceListenerFactory::Create(lis_config, runner);

  ServiceConfig config;
  config.instance_name = kInstanceName;
  // Configure endpoints for outgoing connections
  auto interfaces = GetNonLoopbackInterfaces();
  for (const InterfaceInfo& interface : interfaces) {
    config.connection_endpoints.push_back({interface.addresses[0].address, 0});
  }
  if (interfaces.empty()) {
    std::cerr << "Client: no network interfaces had usable addresses for "
                 "service discovery.\n";
  }

  auto handler = std::make_unique<ClientHandler>(kInstanceName, &promise);
  auto client = ProtocolConnectionClientFactory::Create(
      config, *handler, runner, MessageDemuxer::kDefaultBufferLimit);
  handler->SetClient(client.get());

  // Set up message watches using SetDefaultMessageTypeWatch
  auto& client_demuxer = client->GetMessageDemuxer();

  auto client_watch = client_demuxer.SetDefaultMessageTypeWatch(
      openscreen::msgs::Type::kPresentationConnectionMessage, handler.get());

  listener->AddObserver(*client);
  listener->AddObserver(*handler);

  auto* network_service = NetworkServiceManager::Create(
      std::move(listener), nullptr, std::move(client), nullptr);
  auto* service_listener = network_service->GetServiceListener();
  auto* client_endpoint = network_service->GetProtocolConnectionClient();

  service_listener->Start();
  client_endpoint->Start();

  std::cout << "Client: waiting for server instance \"" << kInstanceName
            << "\"...\n";

  // Wait for completion with timeout
  if (completion_future.wait_for(std::chrono::seconds(10)) ==
      std::future_status::ready) {
    bool success = completion_future.get();
    std::cout << "Client completed "
              << (success ? "successfully" : "with failure") << "\n";
  } else {
    std::cout << "Client timed out after 10 seconds\n";
  }

  // service_listener->Stop();
  // client_endpoint->Stop();
  // NetworkServiceManager::Dispose();
}

}  // namespace openscreen::osp

int main(int argc, char** argv) {
  openscreen::PlatformClientPosix::Create(std::chrono::milliseconds(50));

  bool is_server = (argc >= 2 && std::string(argv[1]) == "server");
  if (is_server) {
    openscreen::osp::RunServer();
  } else {
    openscreen::osp::RunClient();
  }

  openscreen::PlatformClientPosix::ShutDown();
  return 0;
}
