#include <arpa/inet.h>
#include <chrono>
#include <functional>
#include <iostream>
#include <netinet/in.h>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <sys/socket.h>
#include <gtest/gtest.h>
#include <thread>
#include <http/http.h>

static std::string to_hex(std::size_t number)
{

  std::stringstream ss;
  ss << std::hex << number;

  return ss.str();

}

std::string chunk_1 = "Hello World!!!";
std::string chunk_2 = "hello world";
std::string chunk_3 = "123456789";

// there are no race conditions, cause we first recv to check for chunk_i equality, we can only get one byte after writeBody call
static int chunk_i = 0;

static bool handle_data(const http::request* request, std::optional<std::string_view> data)
{

  std::string_view chunk = data.value_or("error");

  if (chunk_i == 0) { EXPECT_EQ(chunk, chunk_1); }
  if (chunk_i == 1) { EXPECT_EQ(chunk, chunk_2); }
  if (chunk_i == 2) { EXPECT_EQ(chunk, chunk_3); }
  if (chunk_i == 3) { EXPECT_EQ(chunk, ""); }

  chunk_i++;

  if (chunk == "")
  {
    request->write_body("Hello World!!!");
    request->end();
  }

  return true;

}

static int init_socket()
{

  int client = socket(AF_INET, SOCK_STREAM, 0);

  if (client == -1) return -1;
  if (client == 0) return -1;

  sockaddr_in serverAddr;
  serverAddr.sin_family = AF_INET;
  serverAddr.sin_port = htons(42069);
  serverAddr.sin_addr.s_addr = inet_addr("127.0.0.1");

  int response = connect(client, (sockaddr*)&serverAddr, sizeof(serverAddr));

  return response == -1 ? -1 : client;

}

TEST(Request, readData_reading_incomplete_buffers)
{

  std::thread([] {

    
    http::server server = http::createServer([](const http::request* request) {

      request->read_data(std::bind(handle_data, request, std::placeholders::_1));

    });

    int status = server.listen("42069");
    ASSERT_NE(status, -1);

  }).detach();

  std::this_thread::sleep_for(std::chrono::milliseconds(500));

  int client = init_socket();

  ASSERT_NE(client, -1);

  std::string http = "POST / HTTP/1.1\r\nTransfer-Encoding: chunked\r\n\r\n";

  std::string packet = http + to_hex(chunk_1.size()) + "\r\n" + chunk_1 + "\r\n" + to_hex(chunk_2.size()) + "\r\n" + chunk_2 + "\r\n" + to_hex(chunk_3.size()) + "\r";
  std::string packet2 = "\n" + chunk_3 + "\r\n0\r\n\r\n";

  ASSERT_NE(send(client, packet.c_str(), packet.size(), 0), -1);

  for (std::size_t i = 0; i < packet2.size(); i++)
  {
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    ASSERT_NE(send(client, &packet2[i], 1, 0), -1);
  }

  char byte;
  ASSERT_NE(recv(client, &byte, 1, 0), -1);
  EXPECT_EQ(chunk_i, 4);

}
