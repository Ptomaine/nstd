//
// file_read_stream.hpp
// ~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2009-2013 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef URDL_DETAIL_FILE_READ_STREAM_HPP
#define URDL_DETAIL_FILE_READ_STREAM_HPP

#include <cctype>
#include <fstream>
#include "../option_set.hpp"
#include "../url.hpp"
#include "abi_prefix.hpp"

namespace urdl {
namespace detail {

class file_read_stream
{
public:
  explicit file_read_stream(asio::io_context& io_context,
      option_set& options)
    : io_context_(io_context),
      options_(options)
  {
  }

  asio::error_code open(const url& u, asio::error_code& ec)
  {
    file_.clear();
    std::string path = u.path();
#if defined(_WIN32)
    if (path.length() >= 3 && path[0] == '/'
        && std::isalpha(path[1]) && path[2] == ':')
      path = path.substr(1);
#endif // defined(_WIN32)
    file_.open(path.c_str(), std::ios_base::in | std::ios_base::binary);
    if (!file_)
    {
      ec = make_error_code(std::errc::no_such_file_or_directory);
      return ec;
    }
    ec = asio::error_code();
    return ec;
  }

  template <typename Handler>
  void async_open(const url& u, Handler handler)
  {
    asio::error_code ec;
    open(u, ec);
    io_context_.post(asio::detail::bind_handler(handler, ec));
  }

  asio::error_code close(asio::error_code& ec)
  {
    file_.close();
    file_.clear();
    ec = asio::error_code();
    return ec;
  }

  bool is_open() const
  {
    // Some older versions of libstdc++ have a non-const is_open().
    return const_cast<std::ifstream&>(file_).is_open();
  }

  template <typename MutableBufferSequence>
  std::size_t read_some(const MutableBufferSequence& buffers,
      asio::error_code& ec)
  {
    if (!file_)
    {
      ec = asio::error::eof;
      return 0;
    }

    typename MutableBufferSequence::const_iterator iter = std::begin(buffers);
    typename MutableBufferSequence::const_iterator end = std::end(buffers);
    for (; iter != end; ++iter)
    {
      asio::mutable_buffer buffer(*iter);
      size_t length = asio::buffer_size(buffer);
      if (length > 0)
      {
        file_.read(asio::buffer_cast<char*>(buffer), length);
        length = file_.gcount();
        if (length == 0 && !file_)
          ec = asio::error::eof;
        return length;
      }
    }

    ec = asio::error_code();
    return 0;
  }

  template <typename MutableBufferSequence, typename Handler>
  void async_read_some(const MutableBufferSequence& buffers, Handler handler)
  {
    asio::error_code ec;
    std::size_t bytes_transferred = read_some(buffers, ec);
    io_context_.post(asio::detail::bind_handler(
          handler, ec, bytes_transferred));
  }

private:
  asio::io_context& io_context_;
  option_set& options_;
  std::ifstream file_;
};

} // namespace detail
} // namespace urdl

#include "abi_suffix.hpp"

#endif // URDL_DETAIL_FILE_READ_STREAM_HPP
