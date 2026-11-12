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

// self
//
#include    "http/http_tcp_client_connection.h"



// cppthread
//
#include    <cppthread/runner.h>


// eventdispatcher
//
//#include    <eventdispatcher/utils.h>


// libaddr
//
//#include    <libaddr/addr.h>



namespace edhttp
{
namespace detail
{



class http_tcp_client_async_connection;


class runner
    : public cppthread::runner
{
public:
                                    runner(
                                          http_tcp_client_async_connection * parent
                                        , addr::addr const & address
                                        , std::string const & host
                                        , ed::mode_t mode
                                        , bool allow_http2);

                                    runner(runner const &) = delete;
    runner &                        operator = (runner const &) = delete;

    // implement runner
    //
    virtual void                    run() override;

    void                            connect();
    addr::addr const &              get_address() const;
    http_tcp_client_connection::pointer_t
                                    release_client();
    std::string const &             get_last_error() const;
    void                            close();

private:
    http_tcp_client_async_connection *
                                    f_parent = nullptr;
    addr::addr const                f_address;
    std::string const               f_host;
    ed::mode_t const                f_mode;
    bool const                      f_allow_http2;
    http_tcp_client_connection::pointer_t
                                    f_tcp_connection = http_tcp_client_connection::pointer_t();
    std::string                     f_last_error = std::string();
};



} // namespace detail
} // namespace edhttp
// vim: ts=4 sw=4 et
