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
#include    <edhttp/http/http_client_connection_impl.h>

#include    <edhttp/http_cache.h>
#include    <edhttp/parametrized_value_list.h>


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
    , f_communicator(ed::communicator::instance())
{
    set_name("edhttp::detail::http_client_connection_impl");
}


http_client_connection_impl::~http_client_connection_impl()
{
    f_communicator->remove_connection(f_udp_ipv4_connection);
    f_communicator->remove_connection(f_udp_ipv6_connection);

    for(auto & d : f_domains)
    {
        f_communicator->remove_connection(d.second);
    }
}


bool http_client_connection_impl::get_keep_alive() const
{
    return f_keep_alive;
}


void http_client_connection_impl::set_keep_alive(bool keep_alive)
{
    f_keep_alive = keep_alive;
}


allowed_http_protocol_t http_client_connection_impl::get_allowed_http_protocols() const
{
    return f_allowed_http_protocols;
}


void http_client_connection_impl::set_allowed_http_protocols(allowed_http_protocol_t allowed)
{
    f_allowed_http_protocols = allowed;
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


bool http_client_connection_impl::get_use_cache() const
{
    return f_use_cache;
}


void http_client_connection_impl::set_use_cache(bool use_cache)
{
    f_use_cache = use_cache;
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


void http_client_connection_impl::process_connected(ed::connection::pointer_t c)
{
    f_parent->process_connected(c);
}


void http_client_connection_impl::process_connection_failed(ed::connection::pointer_t c)
{
    f_parent->process_connection_failed(c);
}


bool http_client_connection_impl::send_request(http_request::pointer_t request)
{
    http_response::pointer_t response;
    if(f_use_cache)
    {
        response = cache::get(request);
        if(response != nullptr)
        {
            if(!response->need_revalidation(request, false))
            {
                process_response(response);
                return true; // in this case you can always send more!
            }
            request->set_response_to_revalidate(response);
        }
    }

    std::string const host(request->get_host());

    http_domain::pointer_t d{};

    // if there is already a valid connection to that host, use it
    {
        auto const it(f_domains.find(host));
        if(it != f_domains.end())
        {
            if(it->second->get_status() == http_domain_status_t::HTTP_DOMAIN_STATUS_DISCONNECTED)
            {
                f_domains.erase(it);
            }
            else
            {
                d = it->second;
            }
        }
    }

    if(d == nullptr)
    {
        d = std::make_shared<http_domain>(
              this
            , host
            , request->get_addresses()
            , request->get_uri().is_scheme_secure()
                    ? ed::mode_t::MODE_ALWAYS_SECURE
                    : ed::mode_t::MODE_PLAIN
            , f_allowed_http_protocols);
        if(!f_communicator->add_connection(d))
        {
            SNAP_LOG_ERROR
                << "adding domain connection \""
                << host
                << "\" to the list of connections failed."
                << SNAP_LOG_SEND;
            return false;
        }

        f_domains[host] = d;
    }

    d->send_request(request);

    return true;
}


bool http_client_connection_impl::process_response(http_response::pointer_t response)
{
    http_request::pointer_t request(response->get_request());

    bool const response_is_from_cache(response->is_from_cache());
    if(!response_is_from_cache)
    {
        std::string const alt_svc(response->get_header(g_name_edhttp_field_alt_svc));
        if(!alt_svc.empty())
        {
            parametrized_value_list ws;
            if(ws.parse(alt_svc))
            {
                // parsing worked, see if there are h2/h3 definitions
                //
                // for now, we completely ignore the "persist=1" possible
                // parameter since we are not monitoring the network
                // connection
                //
                for(int v(2); v <= 3; ++v)
                {
                    std::string protocol("h");
                    protocol += std::to_string(v);
                    std::string const key(protocol + ':' + request->get_host());
                    parametrized_value const * p(ws.get_part(key));
                    if(p != nullptr)
                    {
                        std::int64_t ttl(86400);
                        if(p->has_parameter("ma"))
                        {
                            std::int64_t ma(-1);
                            if(p->get_parameter_as_integer("ma", ma))
                            {
                                ttl = std::clamp<std::int64_t>(ma, 0, 31536000);
                            }
                        }
                        accepted_protocol(key, ttl);
                    }
                }
            }
        }
    }

    bool result(request->process_response(response));
    result = result && f_parent->process_response(response);

    if(!response_is_from_cache)
    {
        // this may have the side effect of invalidating if the method
        // is not GET or HEAD
        //
        cache::set(response);
    }

    return result;
}



} // namespace detail
} // namespace edhttp
// vim: ts=4 sw=4 et
