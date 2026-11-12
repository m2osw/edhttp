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
#include    <edhttp/http/http_tcp_client_connection.h>

#include    <edhttp/http/http_tcp_client_async_connection.h>
#include    <edhttp/exception.h>
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
#include    <snapdev/safe_assert.h>
#include    <snapdev/hexadecimal_string.h>
#include    <snapdev/compare_switch_string.h>

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
namespace
{



// nghttp2 wants to send data back out
ssize_t send_callback(
      nghttp2_session * session
    , std::uint8_t const * data
    , std::size_t length
    , int flags
    , void * user_data)
{
    snapdev::NOT_USED(session, flags);
    http_tcp_client_connection * c(static_cast<http_tcp_client_connection *>(user_data));
    return c->write(data, length);
}


// Callback triggered when a complete header block/frame is received
int on_frame_recv_callback(
      nghttp2_session * session
    , nghttp2_frame const * frame
    , void * user_data)
{
    snapdev::NOT_USED(session);
    http_tcp_client_connection * c(static_cast<http_tcp_client_connection *>(user_data));

    switch(frame->hd.type)
    {
    case NGHTTP2_HEADERS:
        if(frame->headers.cat == NGHTTP2_HCAT_RESPONSE
        && (frame->hd.flags & NGHTTP2_FLAG_END_STREAM) != 0)
        {
            c->process_h2_response(frame);
        }
        break;

    case NGHTTP2_DATA:
        if((frame->hd.flags & NGHTTP2_FLAG_END_STREAM) != 0)
        {
            c->process_h2_response(frame); // frame->hd.stream_id
        }
        break;

    }

    return 0;
}


int on_header_callback(
      nghttp2_session * session
    , nghttp2_frame const * frame
    , std::uint8_t const * name
    , std::size_t name_length
    , std::uint8_t const * value
    , std::size_t value_length
    , std::uint8_t flags
    , void * user_data)
{
    snapdev::NOT_USED(session, flags);
    http_tcp_client_connection * c(static_cast<http_tcp_client_connection *>(user_data));

    //std::int32_t stream_id(frame->hd.stream_id);

    c->set_h2_header(
          frame
        , std::string(reinterpret_cast<char const *>(name), name_length)
        , std::string(reinterpret_cast<char const *>(value), value_length));

    return 0;
}


int on_stream_close_callback(
      nghttp2_session * session
    , std::int32_t stream_id
    , std::uint32_t error_code
    , void * user_data)
{
    snapdev::NOT_USED(session);
    http_tcp_client_connection * c(static_cast<http_tcp_client_connection *>(user_data));

    c->close_h2_stream(stream_id, error_code);

    return 0;
}



} // no name namespace




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

    if(f_is_h2)
    {
        create_h2_session();
    }
}


http_tcp_client_connection::~http_tcp_client_connection()
{
    if(f_h2_session != nullptr)
    {
        nghttp2_session_del(f_h2_session);
    }
}


void http_tcp_client_connection::create_h2_session()
{
    // setup callbacks
    //
    nghttp2_session_callbacks * callbacks(nullptr);
    nghttp2_session_callbacks_new(&callbacks);
    nghttp2_session_callbacks_set_send_callback(callbacks, send_callback);
    nghttp2_session_callbacks_set_on_frame_recv_callback(callbacks, on_frame_recv_callback);
    nghttp2_session_callbacks_set_on_header_callback(callbacks, on_header_callback);
    nghttp2_session_callbacks_set_on_stream_close_callback(callbacks, on_stream_close_callback);

    int const r(nghttp2_session_client_new(&f_h2_session, callbacks, this));

    // done with the callback list
    //
    nghttp2_session_callbacks_del(callbacks);

    if(r != 0)
    {
        snaplogger::message msg(snaplogger::severity_t::SEVERITY_FATAL);
        msg << "failed initializing an nghttp2 session: "
            << nghttp2_strerror(r);
        throw initialization_failure(msg.str());
    }

    // submit a connection settings frame
    //
    nghttp2_settings_entry settings[2] =
    {
        { NGHTTP2_SETTINGS_MAX_CONCURRENT_STREAMS,    100 },
        { NGHTTP2_SETTINGS_INITIAL_WINDOW_SIZE,     65535 },
    };

    nghttp2_submit_settings(f_h2_session, NGHTTP2_FLAG_NONE, settings, 2);
}


bool http_tcp_client_connection::has_output() const
{
    return !f_output.empty();
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


void http_tcp_client_connection::process_hup()
{
    close();
}


void http_tcp_client_connection::process_read()
{
    if(valid_socket())
    {
        int count_bytes(0);
        std::int32_t const time_limit(get_processing_time_limit());
        snapdev::timespec_ex const limit{ time_limit / 1'000'000, time_limit % 1'000'000 };
        snapdev::timespec_ex const date_limit(snapdev::now() + limit);
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
                    || snapdev::now() >= date_limit)
                    {
                        // we reach one or both limits, stop processing so
                        // the other events have a chance to run
                        //
                        if(f_input_position < f_input_size)
                        {
                            add_yield(ed::YIELD_READ);
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


ssize_t http_tcp_client_connection::process_h1_buffer()
{
    // for HTTP/1.1 we do the work here
    //
    ssize_t count_bytes(0);
    std::size_t count_lines(0);
    for(;;)
    {
        switch(f_h1_state)
        {
        case h1_state_t::H1_STATE_CONNECTING:
            throw not_implemented("connecting state not yet implemented.");

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
                    std::size_t const length(f_input_size - f_input_position);
                    count_bytes += length;
                    f_line += std::string(reinterpret_cast<char const *>(f_input.data()) + f_input_position, length);
                    f_input_size = 0;
                    f_input_position = 0;

                    if(f_header_size + f_line.length() > f_maximum_header_size)
                    {
                        // header too large, stop processing this request
                        //
                        SNAP_LOG_ERROR
                            << "total header size too large (1)."
                            << SNAP_LOG_SEND;

                        process_hup();
                    }
                    return count_bytes;
                }
                else
                {
                    std::size_t const length(it - f_input.begin() - f_input_position);
                    count_bytes += length;
                    f_line += std::string(reinterpret_cast<char const *>(f_input.data()) + f_input_position, length);
                }

                // each line must end with "\r\n"
                //
                if(f_line.empty())
                {
                    SNAP_LOG_ERROR
                        << "a line cannot be totally empty; the \\n\\r characters are missing."
                        << SNAP_LOG_SEND;

                    process_hup();
                    return count_bytes;
                }
                if(f_line.back() != '\r')
                {
                    SNAP_LOG_ERROR
                        << "end of header line missing the \\r character."
                        << SNAP_LOG_SEND;

                    process_hup();
                    return count_bytes;
                }
                f_line.pop_back();

                f_header_size += f_line.length() + 2;
                if(f_header_size > f_maximum_header_size)
                {
                    // header too large, stop processing this request
                    //
                    SNAP_LOG_ERROR
                        << "total header size too large (2)."
                        << SNAP_LOG_SEND;

                    process_hup();
                    return count_bytes;
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
                        SNAP_LOG_ERROR
                            << "unrecognized HTTP protocol or found obs-text characters."
                            << SNAP_LOG_SEND;

                        process_hup();
                        return count_bytes;
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
                            SNAP_LOG_ERROR
                                << "found an invalid header line (missing field name, the ':', invalid value)."
                                << SNAP_LOG_SEND;

                            process_hup();
                            return count_bytes;
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
                SNAP_LOG_ERROR
                    << "content-length and transfer-encoding headers are mutually exclusive."
                    << SNAP_LOG_SEND;

                process_hup();
                return count_bytes;
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
                    if(static_cast<std::make_signed_t<decltype(f_content_length)>>(f_content_length) < 0
                    || (end != nullptr && *end != '\0'))
                    {
                        SNAP_LOG_ERROR
                            << "content-length value is not a valid decimal."
                            << SNAP_LOG_SEND;

                        process_hup();
                        return count_bytes;
                    }
                }
                // else -- not specified, assume 0 / streaming mode

                http_response_code_t const code(f_h1_response->get_response_code());
                http_request::pointer_t request(f_h1_response->get_request());
                std::string const method(request->get_method());
                if((code >= 100 && code <= 199)
                || code == 204
                || code == 205
                || code == 304
                || method == "OPTIONS")
                {
                    if(f_content_length != 0)
                    {
                        SNAP_LOG_ERROR
                            << "content-length not zero when it ought to be."
                            << SNAP_LOG_SEND;

                        process_hup();
                        return count_bytes;
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
                            if(parameters[0] != "identity")
                            {
                                f_compressor = get_compressor(parameters[0]);
                                if(f_compressor == nullptr)
                                {
                                    SNAP_LOG_ERROR
                                        << "transfer-encoding \""
                                        << parameters[0]
                                        << "\" is not a recognized/supported compressor (1)."
                                        << SNAP_LOG_SEND;

                                    process_hup();
                                    return count_bytes;
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
                            SNAP_LOG_ERROR
                                << "transfer-encoding \""
                                << encoding
                                << "\" not currently supported."
                                << SNAP_LOG_SEND;

                            process_hup();
                            return count_bytes;
                        }
                        f_h1_state = h1_state_t::H1_STATE_CHUNK_HEADER;
                        f_header_size = 0;
                    }
                    else
                    {
                        std::string const & content_encoding(f_h1_response->get_header(g_name_edhttp_field_content_encoding));
                        if(!content_encoding.empty()
                        && content_encoding != "identity") // although we have an identity compressor, it would be a waste
                        {
                            if(!is_token(content_encoding))
                            {
                                SNAP_LOG_ERROR
                                    << "content-encoding must be exactly one valid token."
                                    << SNAP_LOG_SEND;

                                process_hup();
                                return count_bytes;
                            }
                            f_compressor = get_compressor(content_encoding);
                            if(f_compressor == nullptr)
                            {
                                SNAP_LOG_ERROR
                                    << "transfer-encoding \""
                                    << content_encoding
                                    << "\" is not a recognized/supported compressor (2)."
                                    << SNAP_LOG_SEND;

                                process_hup();
                                return count_bytes;
                            }

                            if(f_content_length > 0)
                            {
                                f_data.reserve(f_content_length);
                            }
                        }

                        f_got_streaming_body = content_length_str.empty();
                        f_h1_state = f_got_streaming_body
                                        ? h1_state_t::H1_STATE_STREAM
                                        : h1_state_t::H1_STATE_BODY;
                        if(f_compressor == nullptr)
                        {
                            f_h1_response->reserve_response(f_content_length);
                        }
                        // else -- we do not have the decompressed size at the moment
                    }
                }
            }
            break;

        case h1_state_t::H1_STATE_BODY:
            {
                std::size_t const length(std::min(static_cast<std::size_t>(f_input_size - f_input_position), f_content_length));
                count_bytes += length;
                if(f_compressor == nullptr)
                {
                    f_h1_response->append_response(std::string(
                              reinterpret_cast<char const *>(f_input.data()) + f_input_position
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
                    // got the entire body, if it were compressed, we need
                    // to decompress it now
                    //
                    if(f_compressor != nullptr)
                    {
                        buffer_t const decompressed_data(f_compressor->decompress(f_data, f_data.size()));
                        f_h1_response->set_response(
                                      std::string(reinterpret_cast<char const *>(decompressed_data.data())
                                    , decompressed_data.size()));
                    }
                    f_h1_state = h1_state_t::H1_STATE_RESPONSE_READY;
                }
            }
            break;

        case h1_state_t::H1_STATE_STREAM:
            {
                std::size_t const length(std::min(static_cast<std::size_t>(f_input_size - f_input_position), f_content_length));
                count_bytes += length;
                if(f_compressor == nullptr)
                {
                    f_h1_response->append_response(buffer_t(
                              f_input.begin() + f_input_position
                            , f_input.begin() + f_input_position + length));
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
                f_h1_response->set_response(buffer_t(
                          decompressed_data.data()
                        , decompressed_data.data() + decompressed_data.size()));
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
                    std::size_t const length(f_input_size - f_input_position);
                    count_bytes += length;
                    f_line += std::string(reinterpret_cast<char const *>(f_input.data()) + f_input_position, length);
                    f_input_size = 0;
                    f_input_position = 0;

                    if(f_header_size + f_line.length() > f_maximum_header_size)
                    {
                        // header too large, stop processing this request
                        //
                        process_hup();
                    }
                    return count_bytes;
                }
                else
                {
                    std::size_t const length(it - f_input.begin() - f_input_position);
                    count_bytes += length;
                    f_line += std::string(reinterpret_cast<char const *>(f_input.data()) + f_input_position, length);
                }

                // each line must end with "\r\n"
                //
                if(f_line.empty())
                {
                    process_hup();
                    return count_bytes;
                }
                if(f_line.back() != '\r')
                {
                    process_hup();
                    return count_bytes;
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
                count_bytes += length;
                if(f_compressor == nullptr)
                {
                    f_h1_response->append_response(buffer_t(
                              f_input.begin() + f_input_position
                            , f_input.begin() + f_input_position + length));
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
                        f_h1_response->append_response(decompressed_data);
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
                    SNAP_LOG_ERROR
                        << "a chunked encoded body must be followed by a \"\\r\\n\" sequence."
                        << SNAP_LOG_SEND;

                    process_hup();
                    return count_bytes;
                }
                count_bytes += 2;
                f_input_position += 2;
                if(f_input_position >= f_input_size)
                {
                    f_input_position = 0;
                    f_input_size = 0;
                }
                f_h1_state = h1_state_t::H1_STATE_CHUNK_HEADER;
            }
            else if(f_input_position == static_cast<ssize_t>(f_input.size() - 1))
            {
                // buffer is "full", we need to move the data because there
                // isn't room for at least 2 bytes
                //
                f_input[0] = f_input[f_input_position];
                f_input_position = 0;
                f_input_size = 1;
            }
            break;

        case h1_state_t::H1_STATE_RESPONSE_READY:
            if(!f_parent->process_response(f_h1_response))
            {
                f_h1_state = h1_state_t::H1_STATE_DONE;

                SNAP_LOG_ERROR
                    << "HTTP/1.x response not accepted by client."
                    << SNAP_LOG_SEND;

                process_hup();
                return count_bytes;
            }
            if(f_got_streaming_body)
            {
                f_h1_state = h1_state_t::H1_STATE_DONE;
            }
            else
            {
                f_h1_state = h1_state_t::H1_STATE_READY;
                f_parent->process_next_request();
            }
            break;

        case h1_state_t::H1_STATE_READY:
            // just sent another request to be sent
            //
            return count_bytes;

        case h1_state_t::H1_STATE_DONE:
            // done with this connection, it cannot be reused, server closed it
            //
            return count_bytes;

        }
    }
}


ssize_t http_tcp_client_connection::process_h2_buffer()
{
    return nghttp2_session_mem_recv(
            f_h2_session,
            f_input.data() + f_input_position,
            f_input_size - f_input_position);
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


ssize_t http_tcp_client_connection::write(void const * buf, std::size_t length)
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


bool http_tcp_client_connection::send_request(http_request::pointer_t request)
{
    http_response::pointer_t response(std::make_shared<http_response>());
    response->set_request(request);

    if(f_is_h2)
    {
        send_h2_request(request, response);

        // note: that means we may return true on an error
        //
        return f_h2_responses.size() >= f_h2_max_streams;
    }
    else
    {
        send_h1_request(request, response);
        return false; // cannot send more...
    }
}


void http_tcp_client_connection::send_h1_request(http_request::pointer_t request, http_response::pointer_t response)
{
    if(f_h1_response != nullptr)
    {
        throw logic_error("send_h1_request() called when an f_h1_response is still in place.");
    }

    f_h1_response = response;

    std::string const data(request->get_request(true));
    write(data.data(), data.length());
}


void http_tcp_client_connection::send_h2_request(
      http_request::pointer_t request
    , http_response::pointer_t response)
{
    // HTTP/2 uses pseudo-headers instead of a request line
    //
    //    :method
    //    :path
    //    :scheme (HTTP or HTTPS)
    //    :authority (equivalent to Host: ... in HTTP/1.1)
    //    :protocol (used with method CONNECT to convert to a "websocket")
    //
    header_t const & request_headers(request->get_headers());
    std::vector<nghttp2_nv> headers;
    headers.reserve(request_headers.size() + 4);

    std::string const method(request->get_method());
    headers.emplace_back(
          const_cast<std::uint8_t *>(reinterpret_cast<uint8_t const *>(g_name_edhttp_field_h2_method))
        , const_cast<std::uint8_t *>(reinterpret_cast<uint8_t const *>(method.c_str()))
        , g_name_edhttp_field_h2_method_length
        , method.length()
        , NGHTTP2_NV_FLAG_NO_COPY_NAME);

    std::string const path(request->get_path());
    headers.emplace_back(
          const_cast<std::uint8_t *>(reinterpret_cast<uint8_t const *>(g_name_edhttp_field_h2_path))
        , const_cast<std::uint8_t *>(reinterpret_cast<uint8_t const *>(path.c_str()))
        , g_name_edhttp_field_h2_path_length
        , path.length()
        , NGHTTP2_NV_FLAG_NO_COPY_NAME);

    char const * const scheme(is_secure()
                        ? g_name_edhttp_scheme_https
                        : g_name_edhttp_scheme_http);
    headers.emplace_back(
          const_cast<std::uint8_t *>(reinterpret_cast<uint8_t const *>(g_name_edhttp_field_h2_scheme))
        , const_cast<std::uint8_t *>(reinterpret_cast<uint8_t const *>(scheme))
        , g_name_edhttp_field_h2_scheme_length
        , strlen(scheme)
        , NGHTTP2_NV_FLAG_NO_COPY_NAME | NGHTTP2_NV_FLAG_NO_COPY_VALUE);

    // this is the replacement for the Host: ... in HTTP/1.1
    //
    std::string const authority(request->get_host());
    headers.emplace_back(
          const_cast<std::uint8_t *>(reinterpret_cast<uint8_t const *>(g_name_edhttp_field_h2_authority))
        , const_cast<std::uint8_t *>(reinterpret_cast<uint8_t const *>(authority.c_str()))
        , g_name_edhttp_field_h2_authority_length
        , authority.length()
        , NGHTTP2_NV_FLAG_NO_COPY_NAME);

    bool user_agent_found(false);
    for(auto const & h : request_headers)
    {
        if(h.first != g_name_edhttp_field_host)
        {
            if(h.first == g_name_edhttp_field_user_agent)
            {
                user_agent_found = true;
            }
            headers.emplace_back(
                  const_cast<std::uint8_t *>(reinterpret_cast<uint8_t const *>(h.first.c_str()))
                , const_cast<std::uint8_t *>(reinterpret_cast<uint8_t const *>(h.second.c_str()))
                , g_name_edhttp_field_h2_authority_length
                , authority.length()
                , NGHTTP2_NV_FLAG_NO_COPY_NAME);
        }
    }

    // in case we use it, we need the user_agent string to remain valid
    // until we send the data with nghttp2_submit_request()
    //
    std::string const user_agent(request->get_agent_name());
    if(!user_agent_found)
    {
        headers.emplace_back(
              const_cast<std::uint8_t *>(reinterpret_cast<uint8_t const *>(g_name_edhttp_field_user_agent))
            , const_cast<std::uint8_t *>(reinterpret_cast<uint8_t const *>(user_agent.c_str()))
            , g_name_edhttp_field_user_agent_length
            , user_agent.length()
            , NGHTTP2_NV_FLAG_NO_COPY_NAME);
    }

    // Submit the request
    //
    std::int32_t const stream_id(nghttp2_submit_request(
                  f_h2_session
                , nullptr
                , headers.data()
                , headers.size()
                , nullptr
                , this));
    
    if(stream_id < 0)
    {
        SNAP_LOG_ERROR
            << "could not submit request: "
            << nghttp2_strerror(stream_id)
            << SNAP_LOG_SEND;
        return;
    }

    f_h2_responses[stream_id] = response;

    nghttp2_session_send(f_h2_session);
}


void http_tcp_client_connection::process_h2_response(nghttp2_frame const * frame)
{
    auto it(f_h2_responses.find(frame->hd.stream_id));
    if(it == f_h2_responses.end())
    {
        throw logic_error(
              "response for stream id #"
            + std::to_string(frame->hd.stream_id)
            + " not found.");
    }

    if(!f_parent->process_response(it->second))
    {
        SNAP_LOG_ERROR
            << "HTTP/2 response not accepted by client."
            << SNAP_LOG_SEND;

        process_hup();
        return;
    }
}


void http_tcp_client_connection::set_h2_header(
      nghttp2_frame const * frame
    , std::string const & name
    , std::string const & value)
{
    if(name.empty())
    {
        throw logic_error("the name of an HTTP/2 header cannot be empty.");
    }

    auto it(f_h2_responses.find(frame->hd.stream_id));
    if(it == f_h2_responses.end())
    {
        throw logic_error(
              "response for stream id #"
            + std::to_string(frame->hd.stream_id)
            + " not found.");
    }

    http_response::pointer_t response(it->second);
    if(name[0] == ':')
    {
        // special names that actually represent the request status
        //
        if(snapdev::compare_exact_switch_string<":status:">(name))
        {
            // the status code
            //
            if(value.length() != 3
            || value[0] < '1' || value[0] > '5'
            || value[1] < '0' || value[1] > '9'
            || value[2] < '0' || value[2] > '9')
            {
                throw logic_error(":status pseudo-header is not exactly 3 decimal digits representing a number from 100 to 599.");
            }

            response->set_protocol(http_protocol_t::HTTP_PROTOCOL_2);
            http_response_code_t const code((value[0] - '0') * 100
                                          + (value[1] - '0') * 10
                                          + (value[2] - '0'));
            response->set_response_code(code);
            response->set_reason_phrase(get_reason_phrase_by_code(code));

            return;
        }

        // anything else generates a warning, we assume we can continue
        //
        SNAP_LOG_WARNING
            << "HTTP/2 client received unknown pseudo-header: \""
            << name
            << "\"."
            << SNAP_LOG_SEND;
        return;
    }

    response->add_header(name, value);
}


void http_tcp_client_connection::close_h2_stream(
      std::int32_t stream_id
    , std::uint32_t error_code)
{
    auto it(f_h2_responses.find(stream_id));
    if(it == f_h2_responses.end())
    {
        throw logic_error(
              "closing stream id #"
            + std::to_string(stream_id)
            + " not found.");
    }

    f_h2_responses.erase(it);

    if(error_code != NGHTTP2_NO_ERROR)
    {
        SNAP_LOG_ERROR
            << "closing stream id #"
            << stream_id
            << " with error "
            << error_code
            << ", "
            << nghttp2_http2_strerror(error_code)
            << SNAP_LOG_SEND;
    }

    while(f_parent->process_next_request());
}



} // namespace edhttp
} // namespace detail
// vim: ts=4 sw=4 et
