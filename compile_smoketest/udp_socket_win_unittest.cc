#include <winsock2.h>

#include <atomic>

#include "gtest/gtest.h"
#include "platform/api/udp_socket.h"
#include "platform/test/fake_clock.h"
#include "platform/test/fake_task_runner.h"

using namespace openscreen;

class WinsockSetup : public ::testing::Environment {
 public:
  void SetUp() override {
    WSADATA wsaData;
    ASSERT_EQ(WSAStartup(MAKEWORD(2, 2), &wsaData), 0);
  }

  void TearDown() override { WSACleanup(); }
};

::testing::Environment* const winsock_env =
    ::testing::AddGlobalTestEnvironment(new WinsockSetup);

class MockClient : public UdpSocket::Client {
 public:
  explicit MockClient(std::atomic_bool* bound_flag) : bound_flag_(bound_flag) {}

  void OnBound(UdpSocket* socket) override { *bound_flag_ = true; }
  void OnError(UdpSocket* socket, const Error& error) override {}
  void OnSendError(UdpSocket* socket, const Error& error) override {}
  void OnRead(UdpSocket* socket, ErrorOr<UdpPacket> packet) override {}

 private:
  std::atomic_bool* bound_flag_;
};

TEST(UdpSocketWinTest, CreateAndBind) {
  FakeClock clock(Clock::now());
  FakeTaskRunner task_runner(clock);

  std::atomic_bool bound_called = false;
  MockClient client(&bound_called);
  IPEndpoint endpoint{{127, 0, 0, 1}, 12345};

  auto socket_or_error = UdpSocket::Create(task_runner, &client, endpoint);
  ASSERT_TRUE(socket_or_error.is_value());

  socket_or_error.value()->Bind();

  // Run the task runner until the OnBound task is processed.
  task_runner.RunTasksUntilIdle();

  EXPECT_TRUE(bound_called);
}