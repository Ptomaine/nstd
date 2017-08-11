//
// connect.hpp
// ~~~~~~~~~~~
//
// Copyright (c) 2009-2013 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef URDL_DETAIL_CONNECT_HPP
#define URDL_DETAIL_CONNECT_HPP

#include <sstream>
#include "coroutine.hpp"
#include "abi_prefix.hpp"

namespace urdl {
namespace detail {

inline asio::error_code connect(
    asio::ip::tcp::socket::lowest_layer_type& socket,
    asio::ip::tcp::resolver& resolver,
    const url& u, asio::error_code& ec)
{
  // Create a query that corresponds to the url.
  std::ostringstream port_string;
  port_string << u.port();
  asio::ip::tcp::resolver::query query(u.host(), port_string.str());

  // Get a list of endpoints corresponding to the query.
  asio::ip::tcp::resolver::iterator iter = resolver.resolve(query, ec);
  if (ec)
    return ec;

  // Try each endpoint until we successfully establish a connection.
  ec = asio::error::host_not_found;
  while (ec && iter != asio::ip::tcp::resolver::iterator())
  {
    socket.close(ec);
    socket.connect(*iter++, ec);
  }
  if (ec)
    return ec;

  // Disable the Nagle algorithm on all sockets.
  return socket.set_option(asio::ip::tcp::no_delay(true), ec);
}

template <typename Handler>
class connect_coro : coroutine
{
public:
  connect_coro(Handler handler,
      asio::ip::tcp::socket::lowest_layer_type& socket,
      asio::ip::tcp::resolver& resolver)
    : handler_(handler),
      socket_(socket),
      resolver_(resolver)
  {
  }

  void operator()(asio::error_code ec,
      asio::ip::tcp::resolver::iterator iter)
  {
    iter_ = iter;
    (*this)(ec);
  }

  void operator()(asio::error_code ec,
      const asio::ip::tcp::resolver::query* query = 0)
  {
    URDL_CORO_BEGIN;

    // Open the socket to give the caller something to close to cancel the
    // asynchronous operation.
    socket_.open(asio::ip::tcp::v4(), ec);
    if (ec)
    {
      URDL_CORO_YIELD(socket_.get_io_context().post(
            asio::detail::bind_handler(*this, ec)));
      handler_(ec);
      return;
    }

    // Get a list of endpoints corresponding to the host name.
    URDL_CORO_YIELD(resolver_.async_resolve(*query, *this));
    if (ec)
    {
      handler_(ec);
      return;
    }

    // Try each endpoint until we successfully establish a connection.
    ec = asio::error::host_not_found;
    while (ec && iter_ != asio::ip::tcp::resolver::iterator())
    {
      // Check whether the operation has been cancelled.
      if (!socket_.is_open())
      {
        ec = asio::error::operation_aborted;
        handler_(ec);
        return;
      }

      // Try next endpoint.
      socket_.close(ec);
      endpoint_ = *iter_++;
      URDL_CORO_YIELD(socket_.async_connect(endpoint_, *this));
    }
    if (ec)
    {
      handler_(ec);
      return;
    }

    // Check whether the operation has been cancelled.
    if (!socket_.is_open())
    {
      ec = asio::error::operation_aborted;
      handler_(ec);
      return;
    }

    // Disable the Nagle algorithm on all sockets.
    socket_.set_option(asio::ip::tcp::no_delay(true), ec);

    handler_(ec);

    URDL_CORO_END;
  }

  friend void* asio_handler_allocate(std::size_t size,
      connect_coro<Handler>* this_handler)
  {
    using asio::asio_handler_allocate;
    return asio_handler_allocate(size, &this_handler->handler_);
  }

  friend void asio_handler_deallocate(void* pointer, std::size_t size,
      connect_coro<Handler>* this_handler)
  {
    using asio::asio_handler_deallocate;
    asio_handler_deallocate(pointer, size, &this_handler->handler_);
  }

  template <typename Function>
  friend void asio_handler_invoke(Function& function,
      connect_coro<Handler>* this_handler)
  {
    using asio::asio_handler_invoke;
    asio_handler_invoke(function, &this_handler->handler_);
  }

  template <typename Function>
  friend void asio_handler_invoke(const Function& function,
      connect_coro<Handler>* this_handler)
  {
    using asio::asio_handler_invoke;
    asio_handler_invoke(function, &this_handler->handler_);
  }

private:
  Handler handler_;
  asio::ip::tcp::socket::lowest_layer_type& socket_;
  asio::ip::tcp::resolver& resolver_;
  asio::ip::tcp::resolver::iterator iter_;
  asio::ip::tcp::endpoint endpoint_;
};

template <typename Handler>
void async_connect(asio::ip::tcp::socket::lowest_layer_type& socket,
    asio::ip::tcp::resolver& resolver, const url& u, Handler handler)
{
  std::ostringstream port_string;
  port_string << u.port();
  asio::ip::tcp::resolver::query query(u.host(), port_string.str());
  connect_coro<Handler>(handler, socket, resolver)(
      asio::error_code(), &query);
}

} // namespace detail
} // namespace urdl

#include "abi_suffix.hpp"

#endif // URDL_DETAIL_CONNECT_HPP
