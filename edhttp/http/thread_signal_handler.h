// Copyright (c) 2011-2026  Made to Order Software Corp.  All Rights Reserved
//
// https://snapwebsites.org/project/edhttp
// contact@m2osw.com
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.
#pragma once

// eventdispatcher
//
#include    <eventdispatcher/thread_done_signal.h>


namespace edhttp
{
namespace detail
{



class http_tcp_client_async_connection;


class thread_signal_handler
    : public ed::thread_done_signal
{
public:
    typedef std::shared_ptr<thread_signal_handler>   pointer_t;

                            thread_signal_handler(http_tcp_client_async_connection * parent_impl);
                            thread_signal_handler(thread_signal_handler const &) = delete;

    thread_signal_handler & operator = (thread_signal_handler const &) = delete;

    virtual void            process_read();

private:
    http_tcp_client_async_connection *
                            f_parent = nullptr;
};



} // namespace detail
} // namespace edhttp
// vim: ts=4 sw=4 et
