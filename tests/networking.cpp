#include <arpa/inet.h>
#include <chrono>
#include <iostream>
#include <netinet/in.h>
#include <sstream>
#include <string>
#include <string_view>
#include <sys/socket.h>

#include <gtest/gtest.h>
#include <thread>
#include "../src/networking/Server.h"

std::string to_hex(std::size_t number)
{

  std::stringstream ss;
  ss << std::hex << number;

  return ss.str();

}

TEST(Request, readData_reading_buffer)
{


  std::string data = "Hello World!!!";
  std::string data2 = "hello world";

  std::thread([&] {

    Server server([&](const Request* request) {
      int i = 0;

      request->readData([request, &i, data, data2](std::optional<std::string_view> response) {

          std::cout << response.value_or("null") << "\n";

          if (response.value_or("null") == "")
          {

            request->writeBody("Hi baby boy");
            response->end();

          }

          if (i == 0)
          {
            EXPECT_EQ(response.value_or("null"), data);
          }
          if (i == 1) 
          {
            EXPECT_EQ(response.value_or("null"), data2);
          }

          i++;
          return true;

      });

      EXPECT_EQ(i, 2);

    });

    int error = server.listen("42069");
    ASSERT_NE(error, -1);

  }).detach();

  std::this_thread::sleep_for(std::chrono::milliseconds(500));

  int client = socket(AF_INET, SOCK_STREAM, 0);
  
  ASSERT_NE(client, -1);
  ASSERT_NE(client, 0);

  sockaddr_in serverAddr;
  serverAddr.sin_family = AF_INET;
  serverAddr.sin_port = htons(42069);
  serverAddr.sin_addr.s_addr = inet_addr("127.0.0.1");

  ASSERT_NE(connect(client, (sockaddr*)&serverAddr, sizeof(serverAddr)), -1);

  std::string http = "POST / HTTP/1.1\r\nTransfer-Encoding: chunked\r\n\r\n";
  std::string packet = http + to_hex(data.size()) + "\r\n" + data + "\r\n" + to_hex(data2.size()) + "\r\n" + data2 + "\r\n9\r";
  std::string packet2 = "\n123456789\r\n0\r\n\r\n";
  send(client, packet.c_str(), packet.size(), 0);

  for (std::size_t i = 0; i < packet2.size(); i++)
  {
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    send(client, &packet2[i], 1, 0);  
  }


  char buffer[1024];
  ssize_t bytes = recv(client, buffer, sizeof(buffer), 0);

  std::cout.write(buffer, bytes);
  std::cout << "\n";

}

TEST(Request, readData)
{

}
