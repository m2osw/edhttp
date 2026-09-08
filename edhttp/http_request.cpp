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
#include    "edhttp/http_request.h"

#include    "edhttp/exception.h"
#include    "edhttp/names.h"
#include    "edhttp/token.h"
#include    "edhttp/uri.h"
#include    "edhttp/version.h"


// snaplogger
//
#include    <snaplogger/message.h>


// eventdispatcher
//
#include    <eventdispatcher/exception.h>


// libaddr
//
#include    <libaddr/addr_parser.h>


// snapdev
//
#include    <snapdev/base64.h>
//#include    <snapdev/not_reached.h>


// C++
//
//#include    <algorithm>
//#include    <iostream>
#include    <sstream>


// last include
//
#include    <snapdev/poison.h>



namespace edhttp
{



addr::addr_range::vector_t http_request::get_address_ranges() const
{
    return f_address_ranges;
}


bool http_request::unique_host() const
{
    bool first(true);
    std::string hostname;
    for(auto const & r : f_address_ranges)
    {
        if(first)
        {
            first = false;
            hostname = r.get_from().get_hostname();
        }
        else if(hostname != r.get_from().get_hostname())
        {
            return false;
        }
    }

    return true;
}


/** \brief Retrieve the hostname.
 *
 * This function returns the host name attached to this request. The
 * hostname is expected to be a domain name although it may also be
 * an IP address.
 *
 * If no addresses were specified, this function returns an empty string.
 *
 * The IP address may be an IPv4 or an IPv6 address.
 *
 * \note
 * The http_request class supports any number of IP address which the
 * HTTP client can make use of to try to connect to the destination.
 * (i.e. if the first IP fails, then try the second one, etc.)
 *
 * \warning
 * The host and port are expected to be the same in all the addresses
 * which is why this function only returns the hostname found in the
 * first address.
 *
 * \return The host name or IP of the specified address.
 */
std::string http_request::get_host() const
{
    if(f_address_ranges.empty())
    {
        return std::string();
    }

    std::string hostname(f_address_ranges[0].get_from().get_hostname());
    if(hostname.empty())
    {
        return f_address_ranges[0].get_from().to_ipv4or6_string(addr::STRING_IP_ADDRESS);
    }

    return hostname;
}


bool http_request::unique_port() const
{
    int port(-1);
    for(auto const & r : f_address_ranges)
    {
        if(port == -1)
        {
            port = r.get_from().get_port();
        }
        else if(port != r.get_from().get_port())
        {
            return false;
        }
    }

    return true;
}


int http_request::get_port() const
{
    if(f_address_ranges.empty())
    {
        return -1;
    }

    return f_address_ranges[0].get_from().get_port();
}


std::string http_request::get_agent_name() const
{
    return f_agent_name;
}


void http_request::set_address_ranges(addr::addr_range::vector_t const & address_ranges)
{
    f_address_ranges = address_ranges;
}


std::string http_request::get_method() const
{
    return f_method;
}


std::string http_request::get_path() const
{
    return f_path;
}


std::string http_request::get_header(std::string const & name) const
{
    if(f_headers.find(name) == f_headers.end())
    {
        return "";
    }
    return f_headers.at(name);
}


std::string http_request::get_post(std::string const & name) const
{
    if(f_post.find(name) == f_post.end())
    {
        return "";
    }
    return f_post.at(name);
}


std::string http_request::get_body() const
{
    return f_body;
}


std::string http_request::get_request(bool keep_alive) const
{
    std::stringstream request;

    // first we generate the body, that way we define its size
    // and also the content type in case of a POST

    // we will get a copy of the body as required because this
    // function is constant and we do not want to modify f_body
    std::string body;
    std::string content_type;

    if(f_has_attachment)
    {
        request << (f_method.empty() ? g_name_edhttp_method_post : f_method.c_str())
                << ' ' << f_path << ' ' << g_name_edhttp_http_1_1 << "\r\n";

        throw logic_error("http_client_server.cpp:get_request(): attachments not supported yet");
    }
    else if(f_has_post)
    {
        // TODO: support the case where the post variables are passed using
        //       a GET and a query string
        //
        request << (f_method.empty() ? g_name_edhttp_method_post : f_method.c_str())
                << ' ' << f_path << ' ' << g_name_edhttp_http_1_1 << "\r\n";
        content_type = "application/x-www-form-urlencoded";

        body = "";
        for(auto it(f_post.begin()); it != f_post.end(); ++it)
        {
            if(!body.empty())
            {
                // separate parameters by ampersands
                body += "&";
            }
            // TODO: escape & and such
            //
            body += it->first + "=" + it->second;
        }
    }
    else if(f_has_data)
    {
        request << (f_method.empty() ? g_name_edhttp_method_post : f_method.c_str())
                << ' ' << f_path << ' ' << g_name_edhttp_http_1_1 << "\r\n";
        body = f_body;
    }
    else if(f_has_body)
    {
        request << (f_method.empty() ? g_name_edhttp_method_get : f_method.c_str())
                << ' ' << f_path << ' ' << g_name_edhttp_http_1_1 << "\r\n";
        body = f_body;
    }
    else
    {
        request << (f_method.empty() ? g_name_edhttp_method_get : f_method.c_str())
                << ' ' << f_path << ' ' << g_name_edhttp_http_1_1 << "\r\n";
        // body is empty by default
        //body = "";
    }

    // place Host first because some servers are that stupid
    request << g_name_edhttp_field_host << ": " << get_host() << "\r\n";

    bool found_user_agent(false);
    for(auto it(f_headers.begin()); it != f_headers.end(); ++it)
    {
        // make sure we do not output the following fields which are
        // managed by our code instead:
        //
        //      Content-Length
        //
        std::string name(it->first);
        std::transform(name.begin(), name.end(), name.begin(), ::tolower);
        if((content_type.empty() || name != g_name_edhttp_field_content_type_lowercase)
        && name != g_name_edhttp_field_content_length_lowercase
        && name != g_name_edhttp_field_host_lowercase
        && name != g_name_edhttp_field_connection_lowercase)
        {
            if(name == g_name_edhttp_field_user_agent_lowercase)
            {
                found_user_agent = true;
            }
            request << it->first
                    << ": "
                    << it->second
                    << "\r\n";
        }
    }

    // forcing the type? (generally doing so with POSTs)
    //
    if(!content_type.empty())
    {
        request
            << g_name_edhttp_field_content_type
            << ": "
            << content_type
            << "\r\n";
    }
    if(!found_user_agent)
    {
        request
            << g_name_edhttp_field_user_agent
            << ": "
            << f_agent_name
            << "/" EDHTTP_VERSION_STRING "\r\n";
    }

    // force the connection valid to what the programmer asked (keep-alive by
    // default though)
    //
    // WARNING: according to HTTP/1.1, servers only expect "close" and not
    //          "keep-alive"; however, it looks like many implementations
    //          understand both (there is also an "upgrade" which we do not
    //          support)
    request
        << g_name_edhttp_field_connection
        << ": "
        << (keep_alive ? g_name_edhttp_param_keep_alive
                       : g_name_edhttp_param_close)
        << "\r\n";

    // end the list with the fields we control:
    //
    // Content-Length is the size of the body
    request
        << g_name_edhttp_field_content_length
        << ": "
        << body.length()
        << "\r\n\r\n";

    // TBD: will this work if 'body' includes a '\0'?
    request << body;

    return request.str();
}


/** \brief Set the host, port, and path at once.
 *
 * HTTP accepts full URIs in the GET, POST, etc. line
 * so the following would be valid:
 *
 *    GET https://snapwebsites.org/some/path?a=view HTTP/1.1
 *
 * However, we break it down in a few separate parts instead, because
 * (a) we need the host to connect to the server, (b) we need the port
 * to connect to the server:
 *
 * 1. Remove protocol, this defines whether we use plain text (http)
 *    or encryption (https/ssl)
 * 2. Get the port, if not specified after the domain, use the default
 *    of the specified URI protocol
 * 3. Domain name is moved to the 'Host: ...' header
 * 4. Path and query string are kept as is
 *
 * So the example above changes to:
 *
 *    GET /some/path?a=view HTTP/1.1
 *    Host: snapwebsites.org
 *
 * We use a plain text connection (http:) and the port is the default
 * port for the HTTP protocol (80). That information does not appear
 * in the HTTP header.
 *
 * \param[in] request_uri  The URI to save in this HTTP request.
 */
void http_request::set_uri(std::string const & request_uri)
{
    uri u(request_uri);

    f_address_ranges = u.address_ranges();

    // use set_path() to make sure we get an absolute path
    // (which is not the case by default)
    set_path(u.path());

    // keep the query string parameters if any are defined
    std::string const q(u.query_string());
    if(!q.empty())
    {
        f_path += "?";
        f_path += q;
    }
}


void http_request::set_host(std::string const & host)
{
    int port(get_port());
    if(port == -1)
    {
        port = 80;
    }

    //f_host = host;
    addr::addr_parser p;
    p.set_default_port(port);
    p.set_protocol(IPPROTO_TCP);
    p.set_sort_order(addr::SORT_IPV6_FIRST | addr::SORT_NO_EMPTY);
    p.set_allow(addr::allow_t::ALLOW_REQUIRED_ADDRESS, true);
    p.set_allow(addr::allow_t::ALLOW_MULTI_ADDRESSES_COMMAS, true);
    p.set_allow(addr::allow_t::ALLOW_MULTI_ADDRESSES_SPACES, true);
    f_address_ranges = p.parse(host);
}


/** \brief Define the port.
 *
 * This function sets the port of all the addresses currently defined in
 * the HTTP request object.
 *
 * \warning
 * If no addresses are defined, then this function does nothing. Please
 * make sure to first add an address and then call this function to force
 * the port as required.
 *
 * \param[in] port  The new port to use to connect.
 */
void http_request::set_port(int port)
{
    for(auto & r : f_address_ranges)
    {
        if(r.has_from())
        {
            r.get_from().set_port(port);
        }
        if(r.has_to())
        {
            r.get_to().set_port(port);
        }
    }
}


void http_request::set_agent_name(std::string const & agent_name)
{
    if(!is_token(agent_name))
    {
        throw expected_token("the User-Agent name must be a valid HTTP token.");
    }

    f_agent_name = agent_name;
}


void http_request::set_method(std::string const & method)
{
    f_method = method;
}


void http_request::set_path(std::string const & path)
{
    // TODO: better verify path validity
    if(path.empty())
    {
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wrestrict"
        f_path = "/";
#pragma GCC diagnostic pop
    }
    else if(path[0] == '/')
    {
        f_path = path;
    }
    else
    {
        f_path = "/" + path;
    }
}


void http_request::set_header(std::string const & name, std::string const & value)
{
    // TODO: verify that the header name is compatible/valid
    // TODO: for known names, verify that the value is compatible/valid
    // TODO: verify the value in various other ways
    if(value.empty())
    {
        // remove headers if defined
        auto h(f_headers.find(value));
        if(h != f_headers.end())
        {
            f_headers.erase(h);
        }
    }
    else
    {
        // add header, overwrite if already defined
        f_headers[name] = value;
    }
}


void http_request::set_post(std::string const & name, std::string const & value)
{
    if(f_has_body || f_has_data)
    {
        throw logic_error("you cannot use set_body(), set_data(), and set_post() on the same http_request object");
    }

    // TODO: verify that the name is a valid name for a post variable
    f_post[name] = value;

    f_has_post = true;
}


void http_request::set_basic_auth(std::string const & username, std::string const & secret)
{
    std::string const authorization_token(username + ":" + secret);
    std::string base64;
    snapdev::base64::encode(authorization_token, base64);

    set_header(
          g_name_edhttp_field_authorization
        , g_name_edhttp_param_basic_authorization + (' ' + base64));
}


void http_request::set_data(std::string const & data)
{
    if(f_has_post || f_has_body)
    {
        throw logic_error("you cannot use set_post(), set_data(), and set_body() on the same http_request object");
    }

    f_body = data;

    f_has_data = true;
}


void http_request::set_body(std::string const & body)
{
    if(f_has_post || f_has_data)
    {
        throw logic_error("you cannot use set_post(), set_data(), and set_body() on the same http_request object");
    }

    f_body = body;

    f_has_body = true;
}



} // namespace edhttp
// vim: ts=4 sw=4 et
