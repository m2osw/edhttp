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
#include    "edhttp/http_response.h"
#include    "edhttp/token.h"
#include    "edhttp/uri.h"


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



ed::mode_t http_request::get_mode() const
{
    return f_mode;
}


uri const & http_request::get_uri() const
{
    return f_uri;
}


/** \brief Transform the domain name to a set of addresses.
 *
 * The function transforms the URI domain name to a set of addresses using
 * the corresponding DNS. The function sorts the addresses interleaving
 * the IPv6 and IPv4 addresses, which is the way you are supposed to use
 * them. First an IPv6 then an IPv4, repeat for all the IPs.
 *
 * The addresses remain in the order the DNS sent them to us outside of
 * the fact that different types get interleaved as defined above.
 * That means the Happy Eyeballs algorithm can be implemented as is
 * from the list returned by this function.
 *
 * \return A vector of addresses.
 *
 * \sa uri::get_addresses()
 */
addr::addr::vector_t http_request::get_addresses() const
{
    return f_uri.addresses();
}


//bool http_request::unique_host() const
//{
//    bool first(true);
//    std::string hostname;
//    for(auto const & r : f_address_ranges)
//    {
//        if(first)
//        {
//            first = false;
//            hostname = r.get_from().get_hostname();
//        }
//        else if(hostname != r.get_from().get_hostname())
//        {
//            return false;
//        }
//    }
//
//    return true;
//}


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
 * \todo
 * Verify that IP addresses work here. The old version would convert
 * the IP to a string if the host was not defined.
 *
 * \return The host name or IP of the specified address.
 */
std::string http_request::get_host() const
{
    return f_uri.full_domain();

    //if(f_address_ranges.empty())
    //{
    //    return std::string();
    //}

    //std::string hostname(f_address_ranges[0].get_from().get_hostname());
    //if(hostname.empty())
    //{
    //    return f_address_ranges[0].get_from().to_ipv4or6_string(addr::STRING_IP_ADDRESS);
    //}

    //return hostname;
}


//bool http_request::unique_port() const
//{
//    int port(-1);
//    for(auto const & r : f_address_ranges)
//    {
//        if(port == -1)
//        {
//            port = r.get_from().get_port();
//        }
//        else if(port != r.get_from().get_port())
//        {
//            return false;
//        }
//    }
//
//    return true;
//}


int http_request::get_port() const
{
    return f_uri.get_port();

    //if(f_address_ranges.empty())
    //{
    //    return -1;
    //}

    //return f_address_ranges[0].get_from().get_port();
}


std::string http_request::get_agent_name() const
{
    return f_agent_name;
}


// if we ever need this, consider adding it, but at this time, the
// f_uri holds all the necessary info
//
//void http_request::set_address_ranges(addr::addr_range::vector_t const & address_ranges)
//{
//    f_address_ranges = address_ranges;
//}


std::string http_request::get_method() const
{
    return f_method;
}


std::string http_request::get_path() const
{
    std::string path("/");
    path += f_uri.path();

    std::string qs(f_uri.query_string());
    if(!qs.empty())
    {
        path += '?';
        path += qs;
    }

    return path;
}


std::string http_request::get_header(std::string const & name) const
{
    if(f_headers.find(name) == f_headers.end())
    {
        return std::string();
    }
    return f_headers.at(name);
}


header_t const & http_request::get_headers() const
{
    return f_headers;
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


/** \brief Generate a string buffer with the complete HTTP/1.1 request.
 *
 * This function generates a string with all the necessary fields
 * defined in the HTTP request. This includes the method at the top,
 * headers, an empty line, and a body if any was defined (only for
 * POST, PUT, and similar methods).
 *
 * \param[in] keep_alive  Whether to keep the connection alive or
 * immediately close it.
 *
 * \return A string of the entire request. It may include binary data.
 */
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
                << ' ' << get_path() << ' ' << g_name_edhttp_http_1_1 << "\r\n";

        throw logic_error("http_client_server.cpp:get_request(): attachments not supported yet");
    }
    else if(f_has_post)
    {
        // TODO: support the case where the post variables are passed using
        //       a GET and a query string
        //
        request << (f_method.empty() ? g_name_edhttp_method_post : f_method.c_str())
                << ' ' << get_path() << ' ' << g_name_edhttp_http_1_1 << "\r\n";
        content_type = "application/x-www-form-urlencoded";

        for(auto it(f_post.begin()); it != f_post.end(); ++it)
        {
            if(!body.empty())
            {
                // separate parameters by ampersands
                //
                body += "&";
            }
            body += urlencode(it->first) + "=" + urlencode(it->second);
        }
    }
    else if(f_has_data)
    {
        request << (f_method.empty() ? g_name_edhttp_method_post : f_method.c_str())
                << ' ' << get_path() << ' ' << g_name_edhttp_http_1_1 << "\r\n";
        body = f_body;
    }
    else if(f_has_body)
    {
        request << (f_method.empty() ? g_name_edhttp_method_get : f_method.c_str())
                << ' ' << get_path() << ' ' << g_name_edhttp_http_1_1 << "\r\n";
        body = f_body;
    }
    else
    {
        request << (f_method.empty() ? g_name_edhttp_method_get : f_method.c_str())
                << ' ' << get_path() << ' ' << g_name_edhttp_http_1_1 << "\r\n";
        // body is empty by default
        //body = "";
    }

    // place Host first because some servers are that stupid
    //
    request << g_name_edhttp_field_host << ": " << get_host() << "\r\n";

    bool found_user_agent(false);
    for(auto it(f_headers.begin()); it != f_headers.end(); ++it)
    {
        // make sure we do not output the following fields which are
        // managed by our code instead:
        //
        //      Connection
        //      Content-Length
        //      Content-Type
        //      Host
        //
        std::string name(it->first);
        std::transform(name.begin(), name.end(), name.begin(), ::tolower);
        if((content_type.empty() || name != g_name_edhttp_field_content_type)
        && name != g_name_edhttp_field_content_length
        && name != g_name_edhttp_field_host
        && name != g_name_edhttp_field_connection)
        {
            if(name == g_name_edhttp_field_user_agent)
            {
                found_user_agent = true;
            }
            request << it->first
                    << ": "
                    << it->second
                    << "\r\n";
        }
    }

    // end the list of headers with the fields we control:

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
            << "\r\n";
    }

    // force the Connection header to what the programmer asked
    //
    // WARNING: according to HTTP/1.1, servers only expect "close" and not
    //          "keep-alive"; however, it looks like many implementations
    //          understand both (there is also an "upgrade" which we do not
    //          support)
    //
    request
        << g_name_edhttp_field_connection
        << ": "
        << (keep_alive ? g_name_edhttp_param_keep_alive
                       : g_name_edhttp_param_close)
        << "\r\n";

    // Content-Length is the size of the body
    //
    request
        << g_name_edhttp_field_content_length
        << ": "
        << body.length()
        << "\r\n\r\n";

    // TBD: will this work if 'body' includes a '\0'?
    //
    request << body;

    return request.str();
}


/** \brief Set the mode to use by default.
 *
 * When a request is sent to one of the clients, it uses this mode as the
 * default mode. The mode may be forced to secure if the port is recognized
 * as a secure port (i.e. 443).
 *
 * The default is a plain connection which works just fine within your
 * network.
 *
 * \param[in] mode  The connection mode to use for this request.
 *
 * \sa get_mode()
 */
void http_request::set_mode(ed::mode_t mode)
{
    f_mode = mode;
}


/** \brief Set the host, port, path, query string, etc.
 *
 * Set the URI of this request as a string.
 *
 * The HTTP protocol expects a method, a path, a query string, and a host:
 *
 *     GET /some/path?a=view HTTP/1.1
 *     Host: snapwebsites.org:<port>
 *     Authenticate: <username>:<password>
 *
 * However, to send the request, we need to break it down because:
 *
 * 1. The path and query string need to be separated from the rest
 *    (scheme, domain, anchor)
 * 2. The anchor is ignored in a request, it's just a point within the
 *    result that browsers scroll to
 * 3. The domain is moved to the Host: ... header; this can be an IP address
 * 4. The port is necessary as a separate number to connect to the server;
 *    if not specified, then it gets derived from the scheme
 * 5. If a username and password are specified, we need a separate
 *    authentication header
 *
 * Note that the scheme does not appear in the HTTP request. That's something
 * used to know how to connect to the server.
 *
 * \note
 * If you use port 443, the system will automatically switch to HTTPS.
 * You can also force the use of TLS encryption by changing the mode
 * from the default (ed::mode_t::MODE_PLAIN) to the secure one
 * (ed::mode_t::MODE_ALWAYS_SECURE).
 *
 * \param[in] request_uri  The URI to save in this HTTP request.
 *
 * \sa set_mode()
 */
void http_request::set_uri(std::string const & request_uri)
{
    uri u;
    u.set_uri(request_uri, true, true);
    set_uri(u);
}


void http_request::set_uri(uri const & request_uri)
{
    f_uri = request_uri;
}


void http_request::set_host(std::string const & host)
{
    f_uri.set_domain(host);

    //int port(get_port());
    //if(port == -1)
    //{
    //    port = 80;
    //}

    ////f_host = host;
    //addr::addr_parser p;
    //p.set_default_port(port);
    //p.set_protocol(IPPROTO_TCP);
    //p.set_sort_order(addr::SORT_IPV6_FIRST | addr::SORT_NO_EMPTY);
    //p.set_allow(addr::allow_t::ALLOW_REQUIRED_ADDRESS, true);
    //p.set_allow(addr::allow_t::ALLOW_MULTI_ADDRESSES_COMMAS, true);
    //p.set_allow(addr::allow_t::ALLOW_MULTI_ADDRESSES_SPACES, true);
    //f_address_ranges = p.parse(host);
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
    f_uri.set_port(port);

    //for(auto & r : f_address_ranges)
    //{
    //    if(r.has_from())
    //    {
    //        r.get_from().set_port(port);
    //    }
    //    if(r.has_to())
    //    {
    //        r.get_to().set_port(port);
    //    }
    //}
}


void http_request::set_agent_name(std::string const & agent_name)
{
    // we expect a possibly full agent string with versions, etc.
    // the very first word must be a valid HTTP token
    //
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
    // force a set by making the path an absolute path
    // (a double "/" is safe here)
    //
    f_uri.set_path("/" + path);
}


/** \brief Set a header.
 *
 * This function sets the header \p name to \p value.
 *
 * The \p name must be a valid HTTP token. See is_token() for details.
 *
 * The function force the name to lowercase. This way, "Content-Type"
 * and "content-type" get saved as the same field. The lowercase characters
 * are used to send the header to the server. You may want to use the
 * names instead of entering your own string. These are already lowercase
 * and you avoid potential misspelling.
 *
 * \note
 * Setting a header to the empty string (\p value set to std::string())
 * actually removes that header from the list.
 *
 * \note
 * Whether you set certain headers or not, the library may still send
 * them with a default value (i.e. User-Agent is sent with "edhttp/<version>"
 * if you do not define your own agent).
 *
 * Some headers are not defined using this function. For example, cookies
 * are defined in a different way, especially because they require one
 * header per cookie (`Set-Cookie: <name>=<value>; ...`).
 *
 * \param[in] name  The name of the header to set.
 * \param[in] value  The value of the header.
 */
void http_request::set_header(std::string name, std::string const & value)
{
    // TODO: for known names, verify that the value is compatible/valid
    // TODO: verify the value in various other ways

    for(auto & c : name)
    {
        if(c >= 'A' && c <= 'Z')
        {
            c |= 0x20; // make lowercase
        }
    }
    if(!is_token(name))
    {
        throw expected_token("the name of an HTTP header must be a valid HTTP token.");
    }

    if(value.empty())
    {
        // remove header if defined
        //
        auto h(f_headers.find(name));
        if(h != f_headers.end())
        {
            f_headers.erase(h);
        }
    }
    else
    {
        // save header value
        //
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


/** \brief Add a response callback.
 *
 * Instead of deriving and reimplementing the process_response() function,
 * you can instead use this function to add a response callback. This
 * function gets called any time you receive a response.
 *
 * Any number of response callbacks can be added.
 *
 * If you keep note of the callback identifier (returned by the function,)
 * then you can remove that callback later.
 *
 * \param[in] f  The function to add.
 *
 * \return An identifier which can be used to remove the callback later.
 */
http_request::callback_manager_t::callback_id_t http_request::add_response_callback(callback_t f)
{
    return f_response_callbacks.add_callback(f);
}


/** \brief Remove a response callback.
 *
 * You may add any number of response callback using the add_response_callback()
 * function. That function returns an identifier that can be used to later
 * remove that callback.
 *
 * \param[in] id  The identifier of the callback to remove.
 *
 * \return true if the callback was found and removed.
 *
 * \sa add_response_callback()
 */
bool http_request::remove_response_callback(callback_manager_t::callback_id_t id)
{
    return f_response_callbacks.remove_callback(id);
}


bool http_request::process_response(http_response::pointer_t response)
{
    return f_response_callbacks.call(response);
}



} // namespace edhttp
// vim: ts=4 sw=4 et
