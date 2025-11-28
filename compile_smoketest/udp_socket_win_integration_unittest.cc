#include <atomic>
#include <memory>
#include <thread>
#include <vector>

#include "gtest/gtest.h"
#include "platform/api/udp_socket.h"
#include "platform/impl/platform_client_win.h"
#include "platform/impl/task_runner.h"
#include "util/osp_logging.h"

using namespace openscreen;

class UdpSocketIntegrationTest : public ::testing::Test {
 public:
  void SetUp() override {
    // PlatformClientWin handles WSAStartup/Cleanup and starts the network loop thread.
    PlatformClientWin::Create(std::chrono::milliseconds(50));
    task_runner_ = &PlatformClientWin::GetInstance()->GetTaskRunner();
  }

  void TearDown() override {
    PlatformClientWin::ShutDown();
  }

  TaskRunner* task_runner_;
};

class IntegrationClient : public UdpSocket::Client {
 public:
  void OnBound(UdpSocket* socket) override {
    bound_.store(true);
  }

  void OnRead(UdpSocket* socket, ErrorOr<UdpPacket> packet) override {
    if (packet.is_value()) {
      last_packet_ = std::move(packet.value());
      received_count_++;
    } else {
      OSP_LOG_ERROR << "Read error: " << packet.error();
    }
  }

  void OnError(UdpSocket* socket, const Error& error) override {
    OSP_LOG_ERROR << "Socket error: " << error;
  }
  
  void OnSendError(UdpSocket* socket, const Error& error) override {
     OSP_LOG_ERROR << "Socket send error: " << error;
  }

  std::atomic_bool bound_{false};
  std::atomic_int received_count_{0};
  UdpPacket last_packet_;
};

TEST_F(UdpSocketIntegrationTest, SendAndReceive) {
  IntegrationClient server_client;
  IntegrationClient client_client;

  IPEndpoint server_addr{{127, 0, 0, 1}, 0}; // 0 = Let OS choose port
  auto server_socket_result = UdpSocket::Create(*task_runner_, &server_client, server_addr);
  ASSERT_TRUE(server_socket_result.is_value());
  auto server_socket = std::move(server_socket_result.value());
  
  server_socket->Bind();
  
  // Wait for bind
  int waits = 0;
  while (!server_client.bound_ && waits++ < 100) {
      std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }
  ASSERT_TRUE(server_client.bound_);
  
  IPEndpoint actual_server_addr = server_socket->GetLocalEndpoint();
  ASSERT_NE(actual_server_addr.port, 0);
  
  auto client_socket_result = UdpSocket::Create(*task_runner_, &client_client, {{127,0,0,1}, 0});
  ASSERT_TRUE(client_socket_result.is_value());
  auto client_socket = std::move(client_socket_result.value());
  
  client_socket->Bind();
  waits = 0;
  while (!client_client.bound_ && waits++ < 100) {
       std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }
  ASSERT_TRUE(client_client.bound_);

  std::vector<uint8_t> data = {1, 2, 3, 4, 5};
  client_socket->SendMessage(data, actual_server_addr);
  
  // Wait for receive (max 2 seconds)
  for (int i=0; i < 200; ++i) {
      if (server_client.received_count_ > 0) break;
      std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }
  
  ASSERT_GT(server_client.received_count_, 0);
  ASSERT_EQ(server_client.last_packet_.size(), data.size());
  EXPECT_EQ(server_client.last_packet_[0], 1);
}
