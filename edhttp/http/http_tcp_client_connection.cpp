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
#include    "http/http_tcp_client_connection.h"

#include    "http/http_tcp_client_async_connection.h"
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
namespace detail
{



http_tcp_client_connection::http_tcp_client_connection(
          http_tcp_client_async_connection * parent
        , addr::addr const & address
        , ed::mode_t const mode
        , ed::tcp_bio_options const & opt)
    : tcp_client_connection(address, mode, opt)
    , f_parent(parent)
    , f_input(16 * 1024)
{
    f_is_h2 = tcp_bio_client::get_selected_alpn() == "h2";
    f_maximum_header_size = f_parent->get_maximum_header_size();

    // the day we can make the connection asynchronous using a non-blocking
    // socket, we instead would need H1_STATE_CONNECTING as the state
    //
    f_h1_state = h1_state_t::H1_STATE_READY;

    set_event_limit(16);
}


bool http_tcp_client_connection::is_h2() const
{
    return f_is_h2;
}


/** \brief An HTTP/1.1 socket was converted to a websocket.
 *
 * With HTTP/1.1, we can upgrade a standard HTTP/1.1 connection to
 * a websocket. Once that upgrade takes hold, we are in a websocket
 * mode and need to handle the frames ourselves. Also the socket
 * cannot be reused for additional HTTP requests such as a GET.
 *
 * \return true if the socket was used with HTTP/1.1 and was converted
 *         to a websocket (i.e. bi-directional stream).
 */
bool http_tcp_client_connection::is_websocket() const
{
    return f_is_websocket;
}


/** \brief The connection is a writer when the output buffer is not empty.
 *
 * This function returns true as long as the output buffer of this
 * client connection is not empty and the socket is still valid.
 *
 * \return true if the output buffer is not empty, false otherwise.
 */
bool http_tcp_client_connection::is_writer() const
{
    return valid_socket() && has_output();
}


void http_tcp_client_connection::process_read()
{
    if(valid_socket())
    {
        int count_bytes(0);
        std::int64_t const date_limit(get_current_date() + get_processing_time_limit());
        bool more(true);
        do
        {
            errno = 0;
            if(f_input_size == 0)
            {
                f_input_size = read(f_input.data(), f_input.size());
            }
            if(f_input_size > 0)
            {
                while(f_input_position < f_input_size)
                {
                    ssize_t const processed_bytes(is_h2()
                                ? process_h2_buffer()
                                : process_h1_buffer());
                    if(processed_bytes <= 0)
                    {
                        if(processed_bytes < 0)
                        {
                            process_error();
                        }
                        return;
                    }
                    f_input_position += processed_bytes;
                    count_bytes += processed_bytes;

                    if(count_bytes >= get_event_limit() * 1024
                    || get_current_date() >= date_limit)
                    {
                        // we reach one or both limits, stop processing so
                        // the other events have a chance to run
                        //
                        if(f_input_position < f_input_size)
                        {
                            add_yield(YIELD_READ);
                        }
                        more = false;
                        break;
                    }
                }
            }
            else if(f_input_size == 0 || errno == 0 || errno == EAGAIN || errno == EWOULDBLOCK)
            {
                // no more data available at this time
                //
                if(f_input_size == 0
                && errno == 0)
                {
                    // explicitly mark the socket as closed
                    //
                    close();

                    // properly terminate a stream
                    //
                    if(f_h1_state == h1_state_t::H1_STATE_STREAM
                    && !is_h2())
                    {
                        f_h1_state = h1_state_t::H1_STATE_STREAM_CLOSED;
                        process_h1_buffer();
                        snapdev::SAFE_ASSERT(f_h1_state == h1_state_t::H1_STATE_DONE, "f_h1_state is expected to be marked DONE after closing a stream.");
                    }
                }
                break;
            }
            else //if(f_input_size < 0)
            {
                // TODO: do something about the error
                //
                int const e(errno);
                SNAP_LOG_ERROR
                    << "an error occurred while reading from socket (errno: "
                    << e
                    << " -- "
                    << strerror(e)
                    << ")."
                    << SNAP_LOG_SEND;
                process_error();
                return;
            }
        }
        while(more);
    }

    tcp_client_connection::process_read();
}


void http_tcp_client_connection::process_h1_buffer()
{
    // for HTTP/1.1 we do the work here
    //
    for(;;)
    {
        switch(f_h1_state)
        {
        case h1_state_t::H1_STATE_WAITING:
            f_h1_state = h1_state_t::H1_STATE_STATUS_LINE;
            [[fallthrough]];
        case h1_state_t::H1_STATE_STATUS_LINE:
        case h1_state_t::H1_STATE_HEADER:
            {
                buffer_t::const_iterator it(std::find(
                      f_input.begin() + f_input_position
                    , f_input.begin() + f_input_size
                    , '\n'));
                if(it == f_input.begin() + f_input_size)
                {
                    // not enough data, return and wait for more
                    //
                    f_line += std::string(f_input.data() + f_input_position, f_input_size - f_input_position);
                    f_input_size = 0;
                    f_input_position = 0;

                    if(f_header_size + f_line.length() > f_maximum_header_size)
                    {
                        // header too large, stop processing this request
                        //
                        process_hup();
                    }
                    return;
                }
                f_line += std::string(f_input.data() + f_input_position, it - f_input.begin() - f_input_position);

                // each line must end with "\r\n"
                //
                if(f_line.empty())
                {
                    process_hup();
                    return;
                }
                if(f_line.back() != '\r')
                {
                    process_hup();
                    return;
                }
                f_line.pop_back();

                f_header_size += f_line.length() + 2;
                if(f_header_size > f_maximum_header_size)
                {
                    // header too large, stop processing this request
                    //
                    process_hup();
                    return;
                }

                if(f_h1_state == h1_state_t::H1_STATE_STATUS_LINE)
                {
                    // the very first line must be the protocol, HTTP code,
                    // and a status; something like:
                    //
                    //     HTTP/1.1 200 OK
                    //
                    if(!f_h1_response->parse_status_line(f_line))
                    {
                        process_hup();
                        return;
                    }
                    f_h1_state = h1_state_t::H1_STATE_HEADER;
                }
                else
                {
                    if(f_line.empty())
                    {
                        // we found the end of the header
                        //
                        f_h1_state = h1_state_t::H1_STATE_CHECK_HEADER;
                    }
                    else
                    {
                        if(!f_h1_response->parse_header(f_line))
                        {
                            process_hup();
                            return;
                        }
                    }
                }
                f_line.clear();

                ++count_lines;
            }
            break;

        case h1_state_t::H1_STATE_CHECK_HEADER:
            // verify that values match / are not in conflict, etc.
            //
            if(f_h1_response->has_header(g_name_edhttp_field_content_length)
            && f_h1_response->has_header(g_name_edhttp_field_transfer_encoding))
            {
                // conflict
                //
                process_hup();
                return;
            }

            {
                // get the content-length since we use it for nearly all cases
                //
                f_content_length = 0;
                std::string const & content_length_str(f_h1_response->get_header(g_name_edhttp_field_content_length));
                if(!content_length_str.empty())
                {
                    char * end;
                    f_content_length = strtoll(content_length_str.c_str(), &end, 10);
                    if(static_cast<std::make_signed<decltype(f_content_length)>>(f_content_length) < 0
                    || (end != nullptr && *end != '\0'))
                    {
                        process_hup();
                        return;
                    }
                }
                // else -- not specified, assume 0 / streaming mode

                http_response_code_t const code(get_response_code());
                std::string const method(f_request.get_method());
                if((code >= 100 && code <= 199)
                || code == 204
                || code == 205
                || code == 304
                || method == "OPTIONS")
                {
                    if(content_length != 0)
                    {
                        process_hup();
                        return;
                    }
                    f_h1_state = h1_state_t::H1_STATE_RESPONSE_READY;
                }
                else if(method == "HEAD")
                {
                    f_h1_state = h1_state_t::H1_STATE_RESPONSE_READY;
                }
                else
                {
                    // TODO: add support for chunked output
                    //       Transfer-Encoding: [compression,] chunked
                    //
                    std::string const & transfer_encoding(f_h1_response->get_header(g_name_edhttp_field_transfer_encoding));
                    if(!transfer_encoding.empty())
                    {
                        // need to split the string and see whether we support
                        // the specified compression or other feature...
                        //
                        std::string encoding;
                        advgetopt::string_list_t parameters;
                        advgetopt::split_string(transfer_encoding, parameters, {","});
                        if(parameters.size() == 2)
                        {
                            // right now, we assume [0] is a compressor name
                            //
                            f(parameters[0] != "identity")
                            {
                                f_compressor = get_compressor(parameters[0]);
                                if(f_compressor == nullptr)
                                {
                                    throw not_supported("transfer-encoding unrecognized compressor.");
                                }
                            }

                            encoding = parameters[1];
                        }
                        else
                        {
                            encoding = parameters[0];
                        }
                        if(encoding != "chunked")
                        {
                            throw unknown_encoding("specified transfer-encoding not currently supported.");
                        }
                        f_h1_state = h1_state_t::H1_STATE_CHUNK_HEADER;
                        f_header_size = 0;
                    }
                    else
                    {
                        std::string const & content_length_str(f_h1_response->get_header(g_name_edhttp_field_content_length));
                        if(!content_encoding.empty()
                        && content_encoding != "identity") // although we have an identity compressor, it would be a waste
                        {
                            if(!is_token(content_encoding))
                            {
                                throw invalid_token("content-encoding must be exactly one valid token.");
                            }
                            f_compressor = get_compressor(content_encoding);
                            if(f_compressor == nullptr)
                            {
                                throw not_supported("transfer-encoding unrecognized compressor.");
                            }

                            if(content_length > 0)
                            {
                                f_data.reserve(content_length);
                            }
                        }

                        f_got_streaming_body = content_length_str.empty();
                        f_h1_state = f_got_streaming_body
                                        ? h1_state_t::H1_STATE_STREAM
                                        : h1_state_t::H1_STATE_BODY;
                        f_response.reserve_response(content_length);
                    }
                }
            }
            break;

        case h1_state_t::H1_STATE_BODY:
            {
                std::size_t const length(std::min(f_input_size - f_input_position, f_content_length));
                if(f_compressor == nullptr)
                {
                    f_h1_response->append_response(std::string(
                              f_input.begin() + f_input_position
                            , length));
                }
                else
                {
                    f_data.insert(
                              f_data.end()
                            , f_input.begin() + f_input_position
                            , f_input.begin() + f_input_position + length);
                }
                f_input_position += length;
                f_content_length -= length;
                if(f_input_position >= f_input_size)
                {
                    f_input_position = 0;
                    f_input_size = 0;
                }
                if(f_content_length == 0)
                {
                    // got the entire body, if it was compressed, we need
                    // to decompress it now
                    //
                    if(f_compressor != nullptr)
                    {
                        buffer_t const decompressed_data(f_compressor->decompress(f_data, f_data.size()));
                        f_h1_response->set_response(std::string(decompressed_data.data(), decompressed_data.size()));
                    }
                    f_h1_state = h1_state_t::H1_STATE_RESPONSE_READY;
                }
            }
            break;

        case h1_state_t::H1_STATE_STREAM:
            {
                std::size_t const length(std::min(f_input_size - f_input_position, f_content_length));
                if(f_compressor == nullptr)
                {
                    f_h1_response->append_response(std::string(
                              f_input.begin() + f_input_position
                            , length));
                }
                else
                {
                    f_data.insert(
                              f_data.end()
                            , f_input.begin() + f_input_position
                            , f_input.begin() + f_input_position + length);
                }
                f_input_position += length;
                if(f_input_position >= f_input_size)
                {
                    f_input_position = 0;
                    f_input_size = 0;
                }
            }
            break;

        case h1_state_t::H1_STATE_STREAM_CLOSED:
            // got the entire body, if it was compressed, we need
            // to decompress it now
            //
            if(f_compressor != nullptr)
            {
                buffer_t const decompressed_data(f_compressor->decompress(f_data, f_data.size()));
                f_h1_response->set_response(std::string(decompressed_data.data(), decompressed_data.size()));
            }
            f_h1_state = h1_state_t::H1_STATE_RESPONSE_READY;
            break;

        case h1_state_t::H1_STATE_CHUNK_HEADER:
            {
                buffer_t::const_iterator it(std::find(
                      f_input.begin() + f_input_position
                    , f_input.begin() + f_input_size
                    , '\n'));
                if(it == f_input.begin() + f_input_size)
                {
                    // not enough data, return and wait for more
                    //
                    f_line += std::string(f_input.data() + f_input_position, f_input_size - f_input_position);
                    f_input_size = 0;
                    f_input_position = 0;

                    if(f_header_size + f_line.length() > f_maximum_header_size)
                    {
                        // header too large, stop processing this request
                        //
                        process_hup();
                    }
                    return;
                }
                f_line += std::string(f_input.data() + f_input_position, it - f_input.begin() - f_input_position);

                // each line must end with "\r\n"
                //
                if(f_line.empty())
                {
                    process_hup();
                    return;
                }
                if(f_line.back() != '\r')
                {
                    process_hup();
                    return;
                }
                f_line.pop_back();

                // the data in these header is `<hex>[; <option>]*`
                // so we search for a ';' and if we find it, drop anything
                // after that (for now, we do not support any such extension)
                //
                std::string::size_type const pos(f_line.find(';'));
                if(pos != std::string::npos)
                {
                    f_line = f_line.substr(0, pos);
                }

                f_content_length = snapdev::hex_to_int<std::size_t>(f_line);
                if(f_content_length == 0)
                {
                    f_h1_state = h1_state_t::H1_STATE_RESPONSE_READY;
                }
                else
                {
                    f_h1_state = h1_state_t::H1_STATE_CHUNK_DATA;

                    if(f_compressor != nullptr)
                    {
                        f_data.reserve(f_content_length);
                    }
                }
            }
            break;

        case h1_state_t::H1_STATE_CHUNK_DATA:
            // read up to f_content_length bytes and save them to the response
            //
            {
                std::size_t length(f_input_size - f_input_position);
                length = std::min(length, f_content_length);
                if(f_compressor == nullptr)
                {
                    f_h1_response->append_response(std::string(
                              f_input.begin() + f_input_position
                            , length));
                }
                else
                {
                    f_data.insert(
                              f_data.end()
                            , f_input.begin() + f_input_position
                            , f_input.begin() + f_input_position + length);
                }
                f_input_position += length;
                f_content_length -= length;
                if(f_input_position >= f_input_size)
                {
                    f_input_position = 0;
                    f_input_size = 0;
                }
                if(f_content_length == 0)
                {
                    // got the entire chunk, if it was compressed, we need
                    // to decompress it now
                    //
                    if(f_compressor != nullptr)
                    {
                        buffer_t decompressed_data(f_compressor->decompress(f_data, f_data.size()));
                        f_h1_response->append_response(std::string(decompressed_data.data(), decompressed_data.size()));
                    }
                    f_h1_state = h1_state_t::H1_STATE_CHUNK_NL;
                }
            }
            break;

        case h1_state_t::H1_STATE_CHUNK_NL:
            if(f_input_size - f_input_position >= 2)
            {
                if(f_input[f_input_position] != '\r'
                || f_input[f_input_position + 1] != '\n')
                {
                    process_hup();
                    return;
                }
                f_input_position += 2;
                if(f_input_position >= f_input_size)
                {
                    f_input_position = 0;
                    f_input_size = 0;
                }
                f_h1_state = h1_state_t::H1_STATE_CHUNK_HEADER;
            }
            else if(f_input_position == f_input.size() - 1)
            {
                // buffer is "full", we need to move the data because there
                // isn't room for at least 2 bytes
                //
                f_input[0] = f_input[f_input_position];
                f_input_position = 0;
                f_input_size = 1;
            }
            break;

        case h1_state_t::H1_STATE_RESPONSE_READY;
            if(!f_request->process_response(f_h1_response))
            {
                f_h1_state = h1_state_t::H1_STATE_DONE;
                process_hup();
                return;
            }
            if(f_got_streaming_body)
            {
                f_h1_state = h1_state_t::H1_STATE_DONE;
            }
            else
            {
                f_h1_state = h1_state_t::H1_STATE_READY;
            }
            break;

        }
    }
}


void http_tcp_client_connection::process_h2_buffer()
{
    return nghttp2_session_mem_recv(
            session,
            reinterpret_cast<std::uint8_t const *>(f_buffer.data()) + f_buffer_position,
            f_buffer_size - f_buffer_position);
}


void http_tcp_client_connection::process_write()
{
    if(valid_socket())
    {
        errno = 0;
        ssize_t const r(tcp_client_connection::write(f_output.data() + f_output_position, f_output.size() - f_output_position));
        if(r > 0)
        {
            // some data was written
            //
            f_output_position += r;
            if(f_output_position >= f_output.size())
            {
                f_output.clear();
                f_output_position = 0;
                process_empty_buffer();
            }
        }
        else if(r < 0 && errno != 0 && errno != EAGAIN && errno != EWOULDBLOCK)
        {
            // connection is considered bad, generate an error
            //
            int const e(errno);
            SNAP_LOG_ERROR
                << "an error occurred while writing to socket of \""
                << get_name()
                << "\" (errno: "
                << e
                << " -- "
                << strerror(e)
                << ")."
                << SNAP_LOG_SEND;
            process_error();
            return;
        }
    }

    tcp_client_connection::process_write();
}


void http_tcp_client_connection::write(void const * buf, std::size_t length)
{
    if(!valid_socket())
    {
        errno = EBADF;
        return -1;
    }

    if(buf != nullptr && length > 0)
    {
        char const * d(reinterpret_cast<char const *>(buf));
        std::size_t l(length);

        if(f_output.empty()
        && is_non_blocking())
        {
            // it is non-blocking so we can attempt an immediate write()
            // to the socket, this way we may be able to avoid caching
            // anything
            //
            errno = 0;
            ssize_t const r(tcp_client_connection::write(d, l));
            if(r > 0)
            {
                l -= r;
                if(l == 0)
                {
                    // no buffer needed!
                    //
                    //process_empty_buffer();
                    return length;
                }

                // could not write the entire buffer, cache the rest
                //
                d += r;
            }
            // TODO: handle error cases -- the process_write() will do that
            //       but we're going to cache the data, etc. which is a waste
        }

        f_output.insert(f_output.end(), d, d + l);
        return length;
    }

    //if(f_output.empty())
    //{
    //    process_empty_buffer();
    //}

    return 0;
}



} // namespace edhttp
} // namespace detail
// vim: ts=4 sw=4 et
