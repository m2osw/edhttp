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
#include    <edhttp/http_client_connection.h>

#include    "http/http_udp_connection.h"
#include    "http/http_domain.h"


// snapdev
//
//#include    <snapdev/timespec_ex.h>



namespace edhttp
{
namespace detail
{



class http_client_connection_impl
    : public ed::timer
{
public:
                                    http_client_connection_impl(http_client_connection * parent);
                                    http_client_connection_impl(http_client_connection_impl const &) = delete;
                                    ~http_client_connection_impl();

    http_client_connection_impl &   operator = (http_client_connection_impl const &) = delete;

    bool                            get_keep_alive() const;
    void                            set_keep_alive(bool keep_alive);
    allowed_http_protocol_t         get_allowed_http_protocols() const;
    void                            set_allowed_http_protocols(allowed_http_protocol_t allowed);
    delay_t                         get_http_connection_timeout() const;
    void                            set_http_connection_timeout(delay_t timeout);
    snapdev::timespec_ex const &    get_http_idle_timeout() const;
    void                            set_http_idle_timeout(snapdev::timespec_ex const & timeout);
    delay_t                         get_happy_eyeballs_delay() const;
    void                            set_happy_eyeballs_delay(delay_t timeout);
    connection_count_t              get_maximum_connection_attempts() const;
    void                            set_maximum_connection_attempts(connection_count_t max);
    std::size_t                     get_maximum_header_size() const;
    void                            set_maximum_header_size(std::size_t maximum_header_size);

    http_udp_connection::pointer_t  get_udp_ipv4_connection();
    http_udp_connection::pointer_t  get_udp_ipv6_connection();

    void                            process_connected(ed::connection::pointer_t c);
    void                            process_connection_failed(ed::connection::pointer_t c);
    bool                            send_request(http_request::pointer_t request);
    bool                            process_response(http_response::pointer_t response);

private:
    http_client_connection *        f_parent = nullptr;
    ed::communicator::pointer_t     f_communicator = ed::communicator::pointer_t();
    bool                            f_keep_alive = true;
    allowed_http_protocol_t         f_allowed_http_protocols = ALLOWED_HTTP_ALL;
    connection_count_t              f_maximum_connection_attempts = MAXIMUM_CONNECTION_ATTEMPTS_DEFAULT;
    std::size_t                     f_maximum_header_size = MAXIMUM_HEADER_SIZE;
    delay_t                         f_http_connection_timeout = DEFAULT_CONNECTION_TIMEOUT;
    snapdev::timespec_ex            f_http_idle_timeout = snapdev::timespec_ex();
    delay_t                         f_happy_eyeballs_delay = DEFAULT_HAPPY_EYEBALLS_DELAY;

    http_domain::map_t              f_domains = http_domain::map_t();
    http_udp_connection::pointer_t  f_udp_ipv4_connection = http_udp_connection::pointer_t();
    http_udp_connection::pointer_t  f_udp_ipv6_connection = http_udp_connection::pointer_t();
};



} // namespace detail
} // namespace edhttp
// vim: ts=4 sw=4 et
