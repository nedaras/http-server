#pragma once

#include "request.h"
#include "../minheap/MinHeap.h"
#include <functional>
#include <sys/epoll.h>

namespace http
{

  using request_handler = std::function<void(const http::request* request)>;

  class server
  {

    public:

      server(const request_handler& callback) : m_callback(std::move(callback)) {}

      int listen(const char* port);

    private:

      int remove_request(const http::request* request);

    private:

      friend class request;

      struct compare_requests 
      {

        bool operator()(const http::request* left, const http::request* right)
        {
          return left->m_timeout > right->m_timeout;
        }

      };

      request_handler m_callback;

      std::vector<epoll_event> m_events;
      MinHeap<const http::request*, compare_requests> m_timeouts; // remove const

      int m_listen_socket;
      int m_epoll;

  };

}

