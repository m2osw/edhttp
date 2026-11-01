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

// self
//
#include    "edhttp/http/http_client_connection_impl.h"



// snaplogger
//
#include    <snaplogger/message.h>


// last include
//
#include    <snapdev/poison.h>





namespace edhttp
{
namespace detail
{




http_client_connection_impl::http_client_connection_impl(http_client_connection * parent)
    : timer(-1)
    , f_parent(parent)
{
}


http_client_connection_impl::~http_client_connection_impl()
{
    f_communicator->remove_connection(f_udp_ipv4_connection);
    f_communicator->remove_connection(f_udp_ipv6_connection);
}


bool http_client_connection_impl::get_keep_alive() const
{
    return f_keep_alive;
}


void http_client_connection_impl::set_keep_alive(bool keep_alive)
{
    f_keep_alive = keep_alive;
}


delay_t http_client_connection_impl::get_http_connection_timeout() const
{
    return f_http_connection_timeout;
}


void http_client_connection_impl::set_http_connection_timeout(delay_t timeout)
{
    f_http_connection_timeout = timeout;
}


snapdev::timespec_ex const & http_client_connection_impl::get_http_idle_timeout() const
{
    return f_http_idle_timeout;
}


void http_client_connection_impl::set_http_idle_timeout(snapdev::timespec_ex const & timeout)
{
    f_http_idle_timeout = timeout;
}


delay_t http_client_connection_impl::get_happy_eyeballs_delay() const
{
    return f_happy_eyeballs_delay;
}


void http_client_connection_impl::set_happy_eyeballs_delay(delay_t max)
{
    f_happy_eyeballs_delay = max;
}


connection_count_t http_client_connection_impl::get_maximum_connection_attempts() const
{
    return f_maximum_connection_attempts;
}


void http_client_connection_impl::set_maximum_connection_attempts(connection_count_t max)
{
    f_maximum_connection_attempts = max;
}


std::size_t http_client_connection_impl::get_maximum_header_size() const
{
    return f_maximum_header_size;
}


void http_client_connection_impl::set_maximum_header_size(std::size_t maximum_header_size)
{
    f_maximum_header_size = maximum_header_size;
}


http_udp_connection::pointer_t http_client_connection_impl::get_udp_ipv4_connection()
{
    if(f_udp_ipv4_connection == nullptr)
    {
        f_udp_ipv4_connection = std::make_shared<http_udp_connection>(this, addr::localhost_ipv4());
        if(!f_communicator->add_connection(f_udp_ipv4_connection))
        {
            SNAP_LOG_ERROR
                << "adding the health connection to the list of connections failed."
                << SNAP_LOG_SEND;
            return http_udp_connection::pointer_t();
        }
    }
    return f_udp_ipv4_connection;
}


http_udp_connection::pointer_t http_client_connection_impl::get_udp_ipv6_connection()
{
    if(f_udp_ipv6_connection == nullptr)
    {
        f_udp_ipv6_connection = std::make_shared<http_udp_connection>(this, addr::localhost_ipv6());
        if(!f_communicator->add_connection(f_udp_ipv6_connection))
        {
            SNAP_LOG_ERROR
                << "adding the health connection to the list of connections failed."
                << SNAP_LOG_SEND;
            return http_udp_connection::pointer_t();
        }
    }
    return f_udp_ipv6_connection;
}


void http_client_connection_impl::send_request(http_request::pointer_t request)
{
    std::string const host(request.get_host());

    // if there is already a connection to that host, use it
    //
    auto const it(f_connections.find(host));
    if(it != f_connections.end())
    {
        if(it->valid_socket())
        {
            it->send_request(request);
            return;
        }
        f_connections.erase(it);
    }

    http_permanent_tcp_connection::pointer_t c(std::make_shared<http_domain>(
          this
        , host
        , request.get_addresses()
        , request.get_uri().is_scheme_secure()));

    f_connections[host] = c;

    c->send_request(request);
}



} // namespace detail
} // namespace edhttp
// vim: ts=4 sw=4 et
