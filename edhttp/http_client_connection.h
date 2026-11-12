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
#include    <edhttp/http_request.h>
#include    <edhttp/http_response.h>


// eventdispatcher
//
#include    <eventdispatcher/communicator.h>
#include    <eventdispatcher/timer.h>



namespace edhttp
{



namespace detail
{
class http_client_connection_impl;
} // namespace detail


typedef std::uint8_t                        connection_count_t;
constexpr connection_count_t const          MAXIMUM_CONNECTION_ATTEMPTS_DEFAULT = 6;

typedef int                                 delay_t;
constexpr delay_t const                     DEFAULT_CONNECTION_TIMEOUT = 10000; // in ms
constexpr delay_t const                     DEFAULT_HAPPY_EYEBALLS_DELAY = 250; // in ms

typedef std::uint32_t                       allowed_http_protocol_t;
constexpr allowed_http_protocol_t const     ALLOWED_HTTP_PROTOCOL_1_0 = 0x0001;
constexpr allowed_http_protocol_t const     ALLOWED_HTTP_PROTOCOL_1_1 = 0x0002;
constexpr allowed_http_protocol_t const     ALLOWED_HTTP_PROTOCOL_2   = 0x0004;
constexpr allowed_http_protocol_t const     ALLOWED_HTTP_PROTOCOL_3   = 0x0008;
constexpr allowed_http_protocol_t const     ALLOWED_HTTP_ALL          = ALLOWED_HTTP_PROTOCOL_1_0
                                                                      | ALLOWED_HTTP_PROTOCOL_1_1
                                                                      | ALLOWED_HTTP_PROTOCOL_2
                                                                      | ALLOWED_HTTP_PROTOCOL_3;


class http_client_connection
    : public ed::timer
{
public:
    typedef std::shared_ptr<http_client_connection>         pointer_t;

                                    http_client_connection();
                                    ~http_client_connection();
                                    http_client_connection(http_client_connection const &) = delete;
    http_client_connection &        operator = (http_client_connection const &) = delete;

    bool                            get_keep_alive() const;
    void                            set_keep_alive(bool keep_alive);
    allowed_http_protocol_t         get_allowed_http_protocols() const;
    void                            set_allowed_http_protocols(allowed_http_protocol_t allowed);
    delay_t                         get_http_connection_timeout() const;
    void                            set_http_connection_timeout(delay_t timeout);
    snapdev::timespec_ex const &    get_http_idle_timeout() const;
    void                            set_http_idle_timeout(snapdev::timespec_ex const & timeout);
    delay_t                         get_happy_eyeballs_delay() const;
    void                            set_happy_eyeballs_delay(delay_t delay);
    connection_count_t              get_maximum_connection_attempts() const;
    void                            set_maximum_connection_attempts(connection_count_t max);
    std::size_t                     get_maximum_header_size() const;
    void                            set_maximum_header_size(std::size_t max);

    void                            send_request(http_request::pointer_t request);

    // new callbacks
    //
    virtual void                    process_response(http_response::pointer_t response);
    virtual void                    process_connected(ed::connection::pointer_t c);
    virtual void                    process_connection_failed(ed::connection::pointer_t c);

private:
    std::shared_ptr<detail::http_client_connection_impl>
                                    f_impl = std::shared_ptr<detail::http_client_connection_impl>();

    ed::communicator::pointer_t     f_communicator = ed::communicator::pointer_t();
};



} // namespace edhttp
// vim: ts=4 sw=4 et
