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
#include    <edhttp/http_client_connection.h>

#include    "http/http_client_connection_impl.h"
//#include    "edhttp/names.h"
//#include    "edhttp/token.h"
//#include    "edhttp/uri.h"
//#include    "edhttp/version.h"


// snaplogger
//
#include    <snaplogger/message.h>


// eventdispatcher
//
#include    <eventdispatcher/exception.h>


// libaddr
//
//#include    <libaddr/addr_parser.h>


// snapdev
//
//#include    <snapdev/base64.h>
//#include    <snapdev/not_reached.h>


// C++
//
//#include    <algorithm>
//#include    <iostream>
//#include    <sstream>


// last include
//
#include    <snapdev/poison.h>



namespace edhttp
{
namespace
{























} // no name namespace



http_client_connection::http_client_connection()
    : timer(0)
    , f_impl(std::make_shared<detail::http_client_connection_impl>(this))
    , f_communicator(ed::communicator::instance())
{
    if(!f_communicator->add_connection(f_impl))
    {
        // this should not happen since we're 100% in control of this
        // pointer so no one else could have added it
        //
        SNAP_LOG_ERROR
            << "adding the client connection implementation to the list of connections failed."
            << SNAP_LOG_SEND;
    }
}


http_client_connection::~http_client_connection()
{
    f_communicator->remove_connection(f_impl);
}


bool http_client_connection::get_keep_alive() const
{
    return f_impl->get_keep_alive();
}


void http_client_connection::set_keep_alive(bool keep_alive)
{
    f_impl->set_keep_alive(keep_alive);
}


delay_t http_client_connection::get_http_connection_timeout() const
{
    return f_http_connection_timeout;
}


void http_client_connection::set_http_connection_timeout(delay_t timeout)
{
    f_http_connection_timeout = timeout;
}


snapdev::timespec_ex const & http_client_connection::get_http_idle_timeout() const
{
    return f_impl->get_http_idle_timeout();
}


void http_client_connection::set_http_idle_timeout(snapdev::timespec_ex const & timeout)
{
    f_impl->set_http_idle_timeout(timeout);
}


delay_t http_client_connection::get_happy_eyeballs_delay() const
{
    return f_impl->get_happy_eyeballs_delay();
}


/** \brief Set the delay of the Happy Eyeballs algorithm.
 *
 * When we try to connect, we do a new attempt every "happy eyeballs delay"
 * milliseconds.
 *
 * The delay is not used if the connection ends up being a single protocol
 * with a single IP address. Then it just tries that one IP and it's done.
 * No delay is necessary.
 *
 * If the domain we are trying to connect to has multiple IP addresses
 * and multiple protocols can be used (HTTP/1.1, HTTP/2, HTTP/3), then
 * the number of possibilities is the product of both (IPs x protocols).
 * Note, however, that we limit the number of attempts as per the
 * get_maximum_connection_attempts().
 *
 * \param[in] delay  Change the delay to this number of microseconds.
 *
 * \sa get_maximum_connection_attempts()
 * \sa get_happy_eyeballs_delay()
 */
void http_client_connection::set_happy_eyeballs_delay(delay_t max)
{
    f_impl->set_happy_eyeballs_delay(max);
}


connection_count_t http_client_connection::get_maximum_connection_attempts() const
{
    return f_impl->get_maximum_connection_attempts();
}


void http_client_connection::set_maximum_connection_attempts(connection_count_t max)
{
    f_impl->set_maximum_connection_attempts(max);
}


std::size_t http_client_connection::get_maximum_header_size() const
{
    return f_impl->get_maximum_header_size();
}


void http_client_connection::set_maximum_header_size(std::size_t max)
{
    f_impl->set_maximum_header_size(max);
}


/** \brief Send a request to an HTTP server.
 *
 * This functions sends an HTTP request to a server. The response will
 * be sent to you through a callback.
 *
 * The request can be sent using HTTP/1.1, HTTP/2, or HTTP/3.
 *
 * \param[in] request  The request to send to the HTTP server.
 */
void http_client_connection::send_request(http_request::pointer_t request)
{
    f_impl->send_request(request);
}



} // namespace edhttp
// vim: ts=4 sw=4 et
