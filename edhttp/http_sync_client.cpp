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
#include    "edhttp/http_sync_client.h"

#include    "edhttp/exception.h"
#include    "edhttp/names.h"
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



http_sync_client::http_sync_client()
{
}


bool http_sync_client::get_keep_alive() const
{
    return f_keep_alive;
}


void http_sync_client::set_keep_alive(bool keep_alive)
{
    f_keep_alive = keep_alive;
}


http_response::pointer_t http_sync_client::send_request(http_request const & request)
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
    read_response(p);

    // keep connection for further calls?
    if(!f_keep_alive
    || p->get_header("connection") == "close")
    {
        f_connection.reset();
    }

    return p;
}


void http_sync_client::read_response(http_response::pointer_t response)
{
    struct reader
    {
        reader(http_response::pointer_t response, ed::tcp_bio_client::pointer_t connection)
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
                    << "read I/O error while reading HTTP protocol in response."
                    << SNAP_LOG_SEND;
                throw client_io_error("read I/O error while reading HTTP protocol in response.");
            }
            f_response->append_original_header(protocol);

SNAP_LOG_TRACE
<< "*** got protocol: "
<< protocol
<< SNAP_LOG_SEND;
            char const *p(protocol.c_str());
            if(strncmp(p, "HTTP/1.0 ", 9) == 0)
            {
                f_response->set_protocol(http_response::protocol_t::HTTP_1_0);
                p += 9; // skip protocol
            }
            else if(strncmp(p, "HTTP/1.1 ", 9) == 0)
            {
                f_response->set_protocol(http_response::protocol_t::HTTP_1_1);
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
                throw client_io_error("read I/O error while reading HTTP protocol in response.");
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
                throw client_io_error("unknown response code, expected exactly three digits.");
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
                        << "read I/O error while reading header."
                        << SNAP_LOG_SEND;
                    throw client_io_error("read I/O error while reading header.");
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
                        << "invalid header, field definition does not include a colon."
                        << SNAP_LOG_SEND;
                    throw client_io_error("invalid header, field definition does not include a colon.");
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
                            << "\", which includes invalid characters."
                            << SNAP_LOG_SEND;
                        throw client_io_error("server returned an HTTP Content-Length which includes invalid characters.");
                    }
                    content_length = content_length * 10 + (*l - '0');
                    if(content_length > 0xFFFFFFFF) // TBD: should we have a much lower limit?
                    {
                        SNAP_LOG_ERROR
                            << "server returned an HTTP Content-Length of "
                            << length
                            << ", which is too large."
                            << SNAP_LOG_SEND;
                        throw client_io_error("server return an HTTP Content-Length which is too large.");
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
                            << "read I/O error while reading response body."
                            << SNAP_LOG_SEND;
                        throw client_io_error("read I/O error while reading response body.");
                    }
                    if(r != content_length)
                    {
                        SNAP_LOG_ERROR
                            << "read returned before the entire content buffer was read."
                            << SNAP_LOG_SEND;
                        throw client_io_error("read returned before the entire content buffer was read.");
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
                            << "read I/O error while reading response body."
                            << SNAP_LOG_SEND;
                        throw client_io_error("read I/O error while reading response body.");
                    }
                    response += std::string(buffer, r);
                }
                f_response->set_response(response);
            }
        }

        http_response::pointer_t         f_response = http_response::pointer_t();
        ed::tcp_bio_client::pointer_t    f_connection = ed::tcp_bio_client::pointer_t();
    } r(response, f_connection);

    r.process();
}



} // namespace edhttp
// vim: ts=4 sw=4 et
