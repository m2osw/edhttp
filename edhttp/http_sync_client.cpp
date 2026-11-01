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


/** \brief Send a small request to an HTTP server.
 *
 * This functions synchronously sends an HTTP request to a server,
 * waits for an answer, and save the answer in an http_response object.
 *
 * If the request has ed::mode_t::MODE_ALWAYS_SECURE as its mode, then
 * all connections will always be encrypted.
 *
 * \todo
 * Consider getting the list of addresses here instead of inside the
 * request. That way, reusing the same request over and over again can
 * be useful and safe. Right now the request determines the IP addresses
 * from the specified URI when those could change over time.
 *
 * \param[in] request  The request to send to the HTTP server.
 *
 * \return A pointer to an HTTP response.
 */
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
        //
        addr::addr::vector_t const & address_ranges(request.get_addresses());
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
                        r,
                        r.get_port() == 443
                            ? ed::mode_t::MODE_ALWAYS_SECURE
                            : request.get_mode());

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
    //
    std::string const data(request.get_request(f_keep_alive));
//std::cerr << "***\n*** request = [" << data << "]\n***\n";
    f_connection->write(data.c_str(), data.length());

    // create a response and read the server's answer in that object
    //
    http_response::pointer_t p(new http_response);
    read_response(p);

    // keep connection for further calls?
    //
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
            read_status_line();
            read_header();
            read_body();
        }

        int read_line(std::string& line)
        {
            int const r(f_connection->read_line(line));
            if(r < 1
            || *line.rbegin() != '\r')
            {
                snaplogger::message msg(snaplogger::severity_t::SEVERITY_ERROR);
                msg << "HTTP header line with missing '\\r' (carriage return) character missing.";
                throw client_io_error(msg.str());
            }

            // remove the '\r'
            //
            line.pop_back();

            if(line.length() > 8 * 1024)
            {
                snaplogger::message msg(snaplogger::severity_t::SEVERITY_ERROR);
                msg << "HTTP header line too long.";
                throw client_io_error(msg.str());
            }

            return r - 1;
        }

        void read_status_line()
        {
            // first check that the status line is HTTP and get the answer code
            //
SNAP_LOG_TRACE
<< "*** read the status line"
<< SNAP_LOG_SEND;
            std::string status_line;
            int const r(read_line(status_line));
            if(r < 0)
            {
                snaplogger::message msg(snaplogger::severity_t::SEVERITY_ERROR);
                msg << "read I/O error while reading HTTP status line in response.";
                throw client_io_error(msg.str());
            }
            if(status_line.size() < 13)
            {
                snaplogger::message msg(snaplogger::severity_t::SEVERITY_ERROR);
                msg << "invalid HTTP status line in response, needed at least 13 characters.";
                throw client_io_error(msg.str());
            }

SNAP_LOG_TRACE
<< "*** got status line: "
<< status_line
<< SNAP_LOG_SEND;
            f_response->parse_status_line(status_line);

#if 0
            char const * p(status_line.c_str());
            if(p[0] != 'H'
            || p[1] != 'T'
            || p[2] != 'T'
            || p[3] != 'P'
            || p[4] != '/'
            || p[5] != '1'
            || p[6] != '.'
            || (p[7] != '0' && p[7] != '1')
            || p[8] != ' '
            || p[9] < '1' || p[9] > '5'
            || p[10] < '0' || p[10] > '9'
            || p[11] < '0' || p[11] > '9'
            || p[12] != ' ')
            {
                snaplogger::message msg(snaplogger::severity_t::SEVERITY_ERROR);
                msg << "unknown protocol or invalid HTTP code in \""
                    << status_line
                    << "\", we only accept HTTP/1.0 and HTTP/1.1 at this time. Code must be between 100 and 599.";
                throw client_io_error(msg.str());
            }
            for(std::size_t idx(13); idx < status_line.length(); ++idx)
            {
                char const c(status_line[idx]);
                if((c < ' ' && c != '\t')
                || c >= 0x7F) // we refuse obs-text characters
                {
                    snaplogger::message msg(snaplogger::severity_t::SEVERITY_ERROR);
                    msg << "found invalid character (\\x"
                        << std::hex << std::setw(2) << c
                        << ") in reason phrase.";
                    throw client_io_error(msg.str());
                }
            }

            f_response->append_original_header(status_line);

            f_response->set_protocol(
                    p[7] == '0'
                        ? http_protocol_t::HTTP_PROTOCOL_1_0
                        : http_protocol_t::HTTP_PROTOCOL_1_1);

            f_response->set_response_code(
                  (p[9] - '0') * 100
                + (p[10] - '0') * 10
                + p[11] - '0');
SNAP_LOG_TRACE
<< "***   +---> code: "
<< f_response->get_response_code()
<< SNAP_LOG_SEND;

            f_response->set_reason_phrase(p + 13);
SNAP_LOG_TRACE
<< "***   +---> msg: ["
<< p + 13
<< "]"
<< SNAP_LOG_SEND;
#endif
        }

        void read_header()
        {
            for(;;)
            {
                std::string field;
                int const r(read_line(field));
                if(r < 0)
                {
                    snaplogger::message msg(snaplogger::severity_t::SEVERITY_ERROR);
                    msg << "read I/O error while reading header.";
                    throw client_io_error(msg.str());
                }
                if(r == 0)
                {
                    // found the empty line after the header
                    // we are done reading the header then
                    //
                    break;
                }
                f_response->parse_header(field);

//                f_response->append_original_header(field);
//
//SNAP_LOG_TRACE
//<< "got a header field: "
//<< field
//<< SNAP_LOG_SEND;
//                char const * f(field.c_str());
//                char const * e(f);
//                for(; *e != ':' && *e != '\0'; ++e);
//                if(*e != ':')
//                {
//                    // TODO: add support for long fields that continue on
//                    //       the following line
//                    //
//                    snaplogger::message msg(snaplogger::severity_t::SEVERITY_ERROR);
//                    SNAP_LOG_ERROR
//                        << "invalid header, field definition does not include a colon."
//                        << SNAP_LOG_SEND;
//                    throw client_io_error("invalid header, field definition does not include a colon.");
//                }
//
//                // get the name and make it lowercase so we can search for
//                // it with ease (HTTP field names are case insensitive)
//                //
//                std::string name(f, e - f);
//                std::transform(name.begin(), name.end(), name.begin(), ::tolower);
//
//                // skip the ':' and then trimming of spaces
//                //
//                for(++e; isspace(*e); ++e);
//                char const * end(f + field.length());
//                for(; end > e && isspace(end[-1]); --end);
//                std::string const value(e, end - e);
//
//                f_response->add_header(name, value);
            }
        }

        void read_body()
        {
            if(f_response->has_header(g_name_edhttp_field_content_length))
            {
                // server sent a content-length parameter, make use of
                // it and do one "large" read
                //
                std::string const length(f_response->get_header(g_name_edhttp_field_content_length));
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
