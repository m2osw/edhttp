// Copyright (c) 2011-2022  Made to Order Software Corp.  All Rights Reserved
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
#include    "edhttp/http_client_server.h"

#include    "edhttp/names.h"
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
#include    <snapdev/not_reached.h>


// C++
//
#include    <algorithm>
#include    <iostream>
#include    <sstream>


// last include
//
#include    <snapdev/poison.h>



namespace edhttp
{


namespace
{

char const g_base64[] =
{
    // 8x8 characters
    'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H',
    'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P',
    'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X',
    'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f',
    'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n',
    'o', 'p', 'q', 'r', 's', 't', 'u', 'v',
    'w', 'x', 'y', 'z', '0', '1', '2', '3',
    '4', '5', '6', '7', '8', '9', '+', '/'
};

}
// no name namespace



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
        return f_address_ranges[0].get_from().to_ipv4or6_string(addr::addr::string_ip_t::STRING_IP_ONLY);
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


std::string http_request::get_command() const
{
    return f_command;
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
        request << (f_command.empty() ? g_name_edhttp_method_post : f_command.c_str())
                << ' ' << f_path << ' ' << g_name_edhttp_http_1_1 << "\r\n";

        throw edhttp_client_server_logic_error("http_client_server.cpp:get_request(): attachments not supported yet");
    }
    else if(f_has_post)
    {
        // TODO: support the case where the post variables are passed using
        //       a GET and a query string
        request << (f_command.empty() ? g_name_edhttp_method_post : f_command.c_str())
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
            body += it->first + "=" + it->second;
        }
    }
    else if(f_has_data)
    {
        request << (f_command.empty() ? g_name_edhttp_method_post : f_command.c_str())
                << ' ' << f_path << ' ' << g_name_edhttp_http_1_1 << "\r\n";
        body = f_body;
    }
    else if(f_has_body)
    {
        request << (f_command.empty() ? g_name_edhttp_method_get : f_command.c_str())
                << ' ' << f_path << ' ' << g_name_edhttp_http_1_1 << "\r\n";
        body = f_body;
    }
    else
    {
        request << (f_command.empty() ? g_name_edhttp_method_get : f_command.c_str())
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


void http_request::set_agent_name(std::string const & name)
{
    f_agent_name = name;
}


void http_request::set_command(std::string const & command)
{
    f_command = command;
}


void http_request::set_path(std::string const & path)
{
    // TODO: better verify path validity
    if(path.empty())
    {
        f_path = "/";
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
        throw edhttp_client_server_logic_error("you cannot use set_body(), set_data(), and set_post() on the same http_request object");
    }

    // TODO: verify that the name is a valid name for a post variable
    f_post[name] = value;

    f_has_post = true;
}


void http_request::set_basic_auth(std::string const & username, std::string const & secret)
{
    auto encode = [](std::string const & in, std::string & out)
    {
        // reset output (just in case)
        out.clear();

        // WARNING: following algorithm does NOT take any line length
        //          in account; and it is deadly well optimized
        unsigned char const *s(reinterpret_cast<unsigned char const *>(in.c_str()));
        while(*s != '\0')
        {
            // get 1 to 3 characters of input
            out += g_base64[s[0] >> 2]; // & 0x3F not required
            ++s;
            if(s[0] != '\0')
            {
                out += g_base64[((s[-1] << 4) & 0x30) | ((s[0] >> 4) & 0x0F)];
                ++s;
                if(s[0] != '\0')
                {
                    // 24 bits of input uses 4 base64 characters
                    out += g_base64[((s[-1] << 2) & 0x3C) | ((s[0] >> 6) & 0x03)];
                    out += g_base64[s[0] & 0x3F];
                    s++;
                }
                else
                {
                    // 16 bits of input uses 3 base64 characters + 1 pad
                    out += g_base64[(s[-1] << 2) & 0x3C];
                    out += '=';
                    break;
                }
            }
            else
            {
                // 8 bits of input uses 2 base64 characters + 2 pads
                out += g_base64[(s[-1] << 4) & 0x30];
                out += "==";
                break;
            }
        }
    };

    std::string const authorization_token(username + ":" + secret);
    std::string base64;
    encode(authorization_token, base64);

    set_header(
          g_name_edhttp_field_authorization
        , g_name_edhttp_param_basic_authorization + (' ' + base64));
}


void http_request::set_data(std::string const & data)
{
    if(f_has_post || f_has_body)
    {
        throw edhttp_client_server_logic_error("you cannot use set_post(), set_data(), and set_body() on the same http_request object");
    }

    f_body = data;

    f_has_data = true;
}


void http_request::set_body(std::string const & body)
{
    if(f_has_post || f_has_data)
    {
        throw edhttp_client_server_logic_error("you cannot use set_post(), set_data(), and set_body() on the same http_request object");
    }

    f_body = body;

    f_has_body = true;
}


std::string http_response::get_original_header() const
{
    return f_original_header;
}


http_response::protocol_t http_response::get_protocol() const
{
    return f_protocol;
}


int http_response::get_response_code() const
{
    return f_response_code;
}


std::string http_response::get_http_message() const
{
    return f_http_message;
}


bool http_response::has_header(std::string const & name) const
{
    return f_header.find(name) != f_header.end();
}


std::string http_response::get_header(std::string const & name) const
{
    return f_header.at(name);
}


std::string http_response::get_response() const
{
    return f_response;
}


void http_response::append_original_header(std::string const & header)
{
    f_original_header += header;
    f_original_header += "\r\n";
}


void http_response::set_protocol(protocol_t protocol)
{
    f_protocol = protocol;
}


void http_response::set_response_code(int code)
{
    f_response_code = code;
}


void http_response::set_http_message(std::string const & message)
{
    f_http_message = message;
}


void http_response::set_header(std::string const& name, std::string const & value)
{
    f_header[name] = value;
}


void http_response::set_response(std::string const & response)
{
    f_response = response;
}


void http_response::read_response(ed::tcp_bio_client::pointer_t connection)
{
    struct reader
    {
        reader(http_response * response, ed::tcp_bio_client::pointer_t connection)
            : f_response(response)
            , f_connection(connection)
        {
        }

        reader(reader const & rhs) = delete;
        reader & operator = (reader const & rhs) = delete;

        void process()
        {
            read_protocol();
            read_header();
            read_body();
        }

        int read_line(std::string& line)
        {
            int r(f_connection->read_line(line));
            if(r >= 1)
            {
                if(*line.rbegin() == '\r')
                {
                    // remove the '\r' if present (should be)
                    line.erase(line.end() - 1);
                    --r;
                }
            }
            return r;
        }

        void read_protocol()
        {
            // first check that the protocol is HTTP and get the answer code
SNAP_LOG_TRACE
<< "*** read the protocol line"
<< SNAP_LOG_SEND;
            std::string protocol;
            int const r(read_line(protocol));
            if(r < 0)
            {
                SNAP_LOG_ERROR
                    << "read I/O error while reading HTTP protocol in response"
                    << SNAP_LOG_SEND;
                throw client_io_error("read I/O error while reading HTTP protocol in response");
            }
            f_response->append_original_header(protocol);

SNAP_LOG_TRACE
<< "*** got protocol: "
<< protocol
<< SNAP_LOG_SEND;
            char const *p(protocol.c_str());
            if(strncmp(p, "HTTP/1.0 ", 9) == 0)
            {
                f_response->set_protocol(protocol_t::HTTP_1_0);
                p += 9; // skip protocol
            }
            else if(strncmp(p, "HTTP/1.1 ", 9) == 0)
            {
                f_response->set_protocol(protocol_t::HTTP_1_1);
                p += 9; // skip protocol
            }
            else
            {
                // HTTP/2 is in the making, but it does not seem to
                // be officially out yet...
                SNAP_LOG_ERROR
                    << "unknown protocol \""
                    << protocol
                    << "\", we only accept HTTP/1.0 and HTTP/1.1 at this time."
                    << SNAP_LOG_SEND;
                throw client_io_error("read I/O error while reading HTTP protocol in response");
            }
            // skip any extra spaces (should be none)
            for(; isspace(*p); ++p);
            int count(0);
            int response_code(0);
            for(; *p >= '0' && *p <= '9'; ++p, ++count)
            {
                // no overflow check necessary, we expect from 1 to 3 digits
                response_code = response_code * 10 + (*p - '0');
            }
            if(count != 3)
            {
                SNAP_LOG_ERROR
                    << "unknown response code \""
                    << protocol
                    << "\", all response code are expected to be three digits (i.e. 200, 401, or 500)."
                    << SNAP_LOG_SEND;
                throw client_io_error("unknown response code, expected exactly three digits");
            }
            f_response->set_response_code(response_code);
SNAP_LOG_TRACE
<< "***   +---> code: "
<< response_code
<< SNAP_LOG_SEND;
            // skip any spaces after the code
            for(; isspace(*p); ++p);
            f_response->set_http_message(p);
SNAP_LOG_TRACE
<< "***   +---> msg: "
<< p
<< SNAP_LOG_SEND;
        }

        void read_header()
        {
            for(;;)
            {
                std::string field;
                int const r(read_line(field));
                if(r < 0)
                {
                    SNAP_LOG_ERROR
                        << "read I/O error while reading header"
                        << SNAP_LOG_SEND;
                    throw client_io_error("read I/O error while reading header");
                }
                if(r == 0)
                {
                    // found the empty line after the header
                    // we are done reading the header then
                    break;
                }
                f_response->append_original_header(field);

SNAP_LOG_TRACE
<< "got a header field: "
<< field
<< SNAP_LOG_SEND;
                char const * f(field.c_str());
                char const * e(f);
                for(; *e != ':' && *e != '\0'; ++e);
                if(*e != ':')
                {
                    // TODO: add support for long fields that continue on
                    //       the following line
                    SNAP_LOG_ERROR
                        << "invalid header, field definition does not include a colon"
                        << SNAP_LOG_SEND;
                    throw client_io_error("invalid header, field definition does not include a colon");
                }
                // get the name and make it lowercase so we can search for
                // it with ease (HTTP field names are case insensitive)
                std::string name(f, e - f);
                std::transform(name.begin(), name.end(), name.begin(), ::tolower);

                // skip the ':' and then left trimming of spaces
                for(++e; isspace(*e); ++e);
                char const * end(f + field.length());
                for(; end > e && isspace(end[-1]); --end);
                std::string const value(e, end - e);

                f_response->set_header(name, value);
            }
        }

        void read_body()
        {
            if(f_response->has_header(g_name_edhttp_field_content_length_lowercase))
            {
                // server sent a content-length parameter, make use of
                // it and do one "large" read
                std::string const length(f_response->get_header(g_name_edhttp_field_content_length_lowercase));
                int64_t content_length(0);
                for(char const * l(length.c_str()); *l != '\0'; ++l)
                {
                    if(*l < '0' || *l > '9')
                    {
                        SNAP_LOG_ERROR
                            << "server returned HTTP Content-Length \""
                            << length
                            << "\", which includes invalid characters"
                            << SNAP_LOG_SEND;
                        throw client_io_error("server returned an HTTP Content-Length which includes invalid characters");
                    }
                    content_length = content_length * 10 + (*l - '0');
                    if(content_length > 0xFFFFFFFF) // TBD: should we have a much lower limit?
                    {
                        SNAP_LOG_ERROR
                            << "server returned an HTTP Content-Length of "
                            << length
                            << ", which is too large"
                            << SNAP_LOG_SEND;
                        throw client_io_error("server return an HTTP Content-Length which is too large");
                    }
                }
                // if content-length is zero, the body response is empty
                if(content_length > 0)
                {
                    std::vector<char> buffer;
                    buffer.resize(content_length);
SNAP_LOG_TRACE
<< "reading "
<< content_length
<< " bytes..."
<< SNAP_LOG_SEND;
                    int const r(f_connection->read(&buffer[0], content_length));
                    if(r < 0)
                    {
                        SNAP_LOG_ERROR
                            << "read I/O error while reading response body"
                            << SNAP_LOG_SEND;
                        throw client_io_error("read I/O error while reading response body");
                    }
                    if(r != content_length)
                    {
                        SNAP_LOG_ERROR
                            << "read returned before the entire content buffer was read"
                            << SNAP_LOG_SEND;
                        throw client_io_error("read returned before the entire content buffer was read");
                    }
                    f_response->set_response(std::string(&buffer[0], content_length));
SNAP_LOG_TRACE
<< "body ["
<< f_response->get_response()
<< "]..."
<< SNAP_LOG_SEND;
                }
            }
            else
            {
                // server did not specify the content-length, this means
                // the request ends when the connection gets closed
                char buffer[BUFSIZ];
                std::string response;
                for(;;)
                {
                    int const r(f_connection->read(buffer, BUFSIZ));
                    if(r < 0)
                    {
                        SNAP_LOG_ERROR
                            << "read I/O error while reading response body"
                            << SNAP_LOG_SEND;
                        throw client_io_error("read I/O error while reading response body");
                    }
                    response += std::string(buffer, r);
                }
                f_response->set_response(response);
            }
        }

        http_response *                  f_response = nullptr;
        ed::tcp_bio_client::pointer_t    f_connection = ed::tcp_bio_client::pointer_t();
    } r(this, connection);

    r.process();
}


bool http_client::get_keep_alive() const
{
    return f_keep_alive;
}


void http_client::set_keep_alive(bool keep_alive)
{
    f_keep_alive = keep_alive;
}


http_response::pointer_t http_client::send_request(http_request const & request)
{
    // we can keep a connection alive, but the host and port cannot
    // change between calls... if you need to make such changes, you
    // may want to consider using another http_client object, otherwise
    // we disconnect the previous connection and reconnect with a new one
    //
    // TBD: test cache with the addresses instead?
    //
    int const port(request.get_port());
    std::string const host(request.get_host());
    if(f_connection != nullptr
    && (f_host != host || f_port != port))
    {
        f_connection.reset();
    }

    // if we have no connection, create a new one
    if(f_connection == nullptr)
    {
        // TODO: allow user to specify the security instead of using the port
        addr::addr_range::vector_t address_ranges(request.get_address_ranges());
        if(address_ranges.empty())
        {
            SNAP_LOG_ERROR
                << "no addresses available for client to connect."
                << SNAP_LOG_SEND;
            throw client_no_addresses("no addresses available for client to connect.");
        }

        // TODO: attempt connecting to any of the offered addresses
        //
        for(auto & r : address_ranges)
        {
            try
            {
                f_connection = std::make_shared<ed::tcp_bio_client>(
                        r.get_from(),
                        r.get_from().get_port() == 443
                            ? ed::mode_t::MODE_ALWAYS_SECURE
                            : ed::mode_t::MODE_PLAIN);

                // we successfully connected, so exit the loop
                break;
            }
            catch(ed::failed_connecting const & e)
            {
                // try again on a connection error
            }
        }

        f_host = host;
        f_port = port;
    }

    // build and send the request to the server
    std::string const data(request.get_request(f_keep_alive));
//std::cerr << "***\n*** request = [" << data << "]\n***\n";
    f_connection->write(data.c_str(), data.length());

    // create a response and read the server's answer in that object
    http_response::pointer_t p(new http_response);
    p->read_response(f_connection);

    // keep connection for further calls?
    if(!f_keep_alive
    || p->get_header("connection") == "close")
    {
        f_connection.reset();
    }

    return p;
}



} // namespace edhttp
// vim: ts=4 sw=4 et
