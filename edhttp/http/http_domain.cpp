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

#include    "exception.h"
#include    "http/http_client_connection_impl.h"
#include    "http/http_tcp_client_async_connection.h"
#include    "http_cache.h"
#include    "utils.h"


// snaplogger
//
#include    <snaplogger/message.h>


// snapdev
//
#include    <snapdev/safe_assert.h>


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
        , ed::mode_t mode
        , allowed_http_protocol_t allowed_protocols)
    : timer(parent->get_happy_eyeballs_delay() * 1000)
    , f_parent(parent)
    , f_communicator(ed::communicator::instance())
    , f_host(host)
    , f_mode(mode)
{
    set_name("edhttp::detail::http_domain[" + host + "]");

    connection_count_t const max_attempts(f_parent->get_maximum_connection_attempts());
    bool const h3(
           (allowed_protocols & ALLOWED_HTTP_PROTOCOL_3) != 0
        && accepted_protocol("h3:" + f_host));
    bool const h2(
           (allowed_protocols & ALLOWED_HTTP_PROTOCOL_2) != 0);
    for(auto const & a : addresses)
    {
        if(h3)
        {
            f_attempts.emplace_back(a, nullptr, true, false);
            if(f_attempts.size() >= max_attempts)
            {
                break;
            }
        }

        f_attempts.emplace_back(a, nullptr, false, h2);
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
        if(!a.f_http3)
        {
#ifdef _DEBUG
            http_udp_connection::pointer_t udp(std::dynamic_pointer_cast<http_udp_connection>(a.f_connection));
            snapdev::SAFE_ASSERT(udp == nullptr, "the connect is not an HTTP3, so it cannot be a UDP connection.");
#endif
            f_communicator->remove_connection(a.f_connection);
            a.f_connection.reset();
        }
    }
    f_attempts.clear();
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
    }
    f_child_connection.reset();
    f_request_list.clear();
}


http_domain_status_t http_domain::get_status() const
{
    if(f_child_connection != nullptr)
    {
        return http_domain_status_t::HTTP_DOMAIN_STATUS_CONNECTED;
    }

    if(f_attempts.empty())
    {
        return http_domain_status_t::HTTP_DOMAIN_STATUS_DISCONNECTED;
    }

    return http_domain_status_t::HTTP_DOMAIN_STATUS_CONNECTING;
}


std::string const & http_domain::get_host() const
{
    return f_host;
}


/** \brief Send a request to the HTTP server.
 *
 * This function sends a request to the HTTP server.
 *
 * Note that if the connection is not yet established, the function
 * simply caches the request. Eventually, your callback is called
 * with the response. This means you cannot reuse the same request
 * over and over again. You must create new requests each time you
 * send one or wait for the callback to be called before you reuse
 * it.
 *
 * \param[in] request  The request to send to the server.
 */
bool http_domain::send_request(http_request::pointer_t request)
{
    http_response::pointer_t response(cache::get(request));
    if(response != nullptr)
    {
        return true;
    }

    // if we are dealing with an HTTP/1.x version then we need to make sure
    // to send one request at a time; this is handled by the connection but
    // we receive requests even before the connection is established so
    // we need to cache them until we do have a fully valid connection
    //
    if(f_child_connection != nullptr
    && f_request_list.empty())
    {
        return apply_send_request(request);
    }
    else
    {
        // caching
        //
        f_request_list.push_back(request);
        return false;
    }
}


/** \brief Process the next cached request.
 *
 * Until a connection is established, requests do not get sent anywhere.
 * Instead, they get cached by the http_domain object. The request is
 * processed only once the connection is established.
 *
 * Further, many HTTP/1.x connections do not really work well went sent
 * multiple requests in a raw without first accepting the entire reply.
 * So for HTTP/1.x, this happens each time a request was processed and
 * another is ready to be sent to the server.
 *
 * \warning
 * This function is considered private. Do not call it directly. It is
 * an internal signal. You need to use the send_request() function instead.
 *
 * \return true if a request was just sent.
 *
 * \sa send_request()
 */
bool http_domain::process_next_request()
{
    if(f_request_list.empty())
    {
        return false;
    }

    http_request::pointer_t request(f_request_list.front());
    f_request_list.pop_front();
    return apply_send_request(request);
}


bool http_domain::apply_send_request(http_request::pointer_t request)
{
    return std::dynamic_pointer_cast<http_connection>(f_child_connection)->send_request(request);
}


std::size_t http_domain::get_maximum_header_size() const
{
    return f_parent->get_maximum_header_size();
}


void http_domain::process_connected(ed::connection::pointer_t c)
{
    f_child_connection = c;

    // make sure to stop the other attempts; we have a valid connection now
    //
    auto it(std::find_if(
          f_attempts.begin()
        , f_attempts.end()
        , [&c](auto const & a)
        {
            return a.f_connection == c;
        }));
    if(it == f_attempts.end())
    {
        throw logic_error("somehow a successful connection is not part of our attempts."); // LCOV_EXCL_LINE
    }
    f_attempts.erase(it);
    close_attempts();

    // if we already received requests, forward them to the child
    //
    if(std::dynamic_pointer_cast<http_connection>(c)->is_h2())
    {
        // with H2 we can send all the requests at once
        // (albeit, it may not always be wise!?)
        //
        while(process_next_request());
    }
    else
    {
        process_next_request();
    }

    f_parent->process_connected(c);
}


void http_domain::process_connection_failed(ed::connection::pointer_t c)
{
    f_parent->process_connection_failed(c);
}


bool http_domain::process_response(http_response::pointer_t response)
{
    response->mark_as_received();
    return f_parent->process_response(response);
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
std::cerr << "--- start tcp attempt #" << static_cast<int>(f_current_attempt) << " for address: " << f_attempts[f_current_attempt].f_address << "... " << snapdev::now() << "\n";
        http_tcp_client_async_connection::pointer_t c(std::make_shared<http_tcp_client_async_connection>(
                  this
                , f_attempts[f_current_attempt].f_address
                , f_host
                , f_mode
                , f_attempts[f_current_attempt].f_allow_http2));
        if(c->start())
        {
            f_attempts[f_current_attempt].f_connection = c;
            if(!f_communicator->add_connection(c))
            {
                SNAP_LOG_ERROR
                    << "adding the health connection to the list of connections failed."
                    << SNAP_LOG_SEND;
                return;
           }
        }
    }

    ++f_current_attempt;
    if(f_current_attempt >= f_attempts.size())
    {
        delay_t const timeout(f_parent->get_http_connection_timeout());
        set_timeout_delay(timeout * 1'000);
    }
}



} // namespace detail
} // namespace edhttp
// vim: ts=4 sw=4 et
