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
#include    "http/http_domain.h"

#include    "http/http_client_connection_impl.h"
#include    "utils.h"


// last include
//
#include    <snapdev/poison.h>



namespace edhttp
{
namespace detail
{



/** \brief Initialize the HTTP domain object.
 *
 * This constructor initializes the HTTP domain object which will run
 * a battery of connection attempts as per the Happy Eyeballs algorithm.
 * This means trying to connect with HTTP/3 if that domain has that
 * feature turned on. Also try to connect using HTTP/2 or HTTP/1.1 if
 * HTTP/3 does not reply in time.
 *
 * The total number of attempts is defined by the maximum set in the
 * parent through the http_client_connection.
 *
 * \param[in] parent  The parent (http_client_connection_impl).
 * \param[in] host  The name of the host to connect to.
 */
http_domain::http_domain(
          http_client_connection_impl * parent
        , std::string const & host
        , addr::addr::vector_t const & addresses
        , ed::mode_t mode)
    : timer(parent->get_happy_eyeballs_delay() * 1000)
    , f_parent(parent)
    , f_communicator(ed::communicator::instance())
    , f_host(host)
    , f_mode(mode)
{
    connection_count_t const max_attempts(f_parent->get_maximum_connection_attempts());
    bool const h3(accept_h3(f_host));
    for(auto const & a : addresses)
    {
        if(h3)
        {
            f_attempts.emplace_back(a, nullptr, true);
            if(f_attempts.size() >= max_attempts)
            {
                break;
            }
        }

        f_attempts.emplace_back(a, nullptr, false);
        if(f_attempts.size() >= max_attempts)
        {
            break;
        }
    }

    start_attempt();
}


http_domain::~http_domain()
{
    close_attempts();
    close();
}


void http_domain::close_attempts()
{
    for(auto & a : f_attempts)
    {
        // UDP connections are managed by our parent, make sure not to
        // remove them from the communicator
        //
        http_udp_connection::pointer_t udp(std::dynamic_pointer_cast<http_udp_connection>(a.f_connection));
        if(udp == nullptr)
        {
            f_communicator->remove_connection(a.f_connection);
            a.f_connection.reset();
        }
    }
}


void http_domain::close()
{
    // UDP connections are managed by our parent, make sure not to
    // remove them from the communicator
    //
    http_udp_connection::pointer_t udp(std::dynamic_pointer_cast<http_udp_connection>(f_child_connection));
    if(udp == nullptr)
    {
        f_communicator->remove_connection(f_child_connection);
        f_child_connection.reset();
    }
}


void http_domain::send_request(http_request::pointer_t request)
{
    // if we are dealing with an HTTP/1.x version then we need to make sure
    // to send one request at a time; this is handled by the connection but
    // we receive requests even before the connection is established so
    // we need to cache them until we do have a fully valid connection
    //
}


std::size_t http_domain::get_maximum_header_size() const
{
    return f_parent->get_maximum_header_size();
}


void http_domain::process_timeout()
{
    if(f_current_attempt >= f_attempts.size())
    {
        // could not connect, the whole thing timed out
        //
        set_enable(false);
        process_error();
    }
    else
    {
        start_attempt();
    }
}


void http_domain::process_error()
{
    close_attempts();
    close();
}


void http_domain::process_hup()
{
    close_attempts();
    close();
}


/** \brief Start the next attempt.
 *
 * This function starts the next attempt. If a connection attempt succeeds,
 * then this process is stopped by disabling the connection which prevents
 * further calls tot he process_timeout() and thus this very function. If
 * all the attempts were used up and none returned, then the whole process
 * ends in a "could not connect" error.
 */
void http_domain::start_attempt()
{
    if(f_current_attempt >= f_attempts.size())
    {
        // this should not happen, but just in case
        //
        set_enable(false);
        return;
    }

    if(f_attempts[f_current_attempt].f_http3)
    {
        if(f_attempts[f_current_attempt].f_address.is_ipv4())
        {
            f_attempts[f_current_attempt].f_connection = f_parent->get_udp_ipv4_connection();
        }
        else
        {
            f_attempts[f_current_attempt].f_connection = f_parent->get_udp_ipv6_connection();
        }
    }
    else
    {
        http_tcp_client_async_connection::pointer_t c(std::make_shared<http_tcp_client_async_connection>(
                  this
                , f_attempt[f_current_attempt].f_address
                , f_mode));
        if(c->start())
        {
            f_attempt[f_current_attempt].f_connection = c;
            if(!f_communicator->add_connection(c))
            {
                SNAP_LOG_ERROR
                    << "adding the health connection to the list of connections failed."
                    << SNAP_LOG_SEND;
                return false;
           }
        }
    }

    ++f_current_attempt;
    if(f_current_attempt >= f_attempts.size())
    {
        delay_t const timeout(get_http_connection_timeout());
        set_timeout_delay(timeout * 1'000);
    }
}



} // namespace detail
} // namespace edhttp
// vim: ts=4 sw=4 et
