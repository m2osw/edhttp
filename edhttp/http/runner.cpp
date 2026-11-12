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
#include    "http/runner.h"

#include    "http/http_tcp_client_async_connection.h"


// eventdispatcher
//
#include    <eventdispatcher/exception.h>


// cppthread
//
#include    <cppthread/guard.h>


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



runner::runner(
	      http_tcp_client_async_connection * parent
	    , addr::addr const & address
        , std::string const & host
	    , ed::mode_t mode
        , bool allow_http2)
    : cppthread::runner("background http_tcp_connection for asynchronous connections")
    , f_parent(parent)
    , f_address(address)
    , f_host(host)
    , f_mode(mode)
    , f_allow_http2(allow_http2)
{
}



/** \brief This is the actual function run by the thread.
 *
 * This function calls the connect() function and then
 * tells the main thread we are done.
 */
void runner::run()
{
    connect();

    // tell the main thread that we are done
    //
    f_parent->trigger_thread_done();
}


/** \brief This function attempts to connect.
 *
 * This function attempts a connection to the specified address
 * and port with the specified mode (i.e. plain or encrypted.)
 *
 * The function may take a long time to succeed connecting with
 * the server. The main thread will be awaken whenever this
 * thread dies.
 *
 * If an error occurs, then the f_socket variable member will
 * be set to -1. Otherwise it represents the socket that we
 * just connected with.
 */
void runner::connect()
{
    char const * error_name(nullptr);
    try
    {
        // create a socket using the bio_client class,
        // but then just create a duplicate that we will
        // use in a server-client TCP object (because
        // we cannot directly create the right type of
        // object otherwise...)
        //
        ed::tcp_bio_options http_options;
        //http_options.set_sni(true); -- this is the default
        http_options.set_host(f_host);

        // only secure connections support the ALPN option
        //
        if(f_mode != ed::mode_t::MODE_PLAIN
        && f_allow_http2)
        {
            http_options.add_alpn("h2");
            http_options.add_alpn("http/1.1");
        }

        f_tcp_connection = std::make_shared<http_tcp_client_connection>(
                  f_parent
                , f_address
                , f_mode
                , http_options);
        return;
    }
    catch(ed::failed_connecting const & e)
    {
        error_name = "ed::failed_connecting";
        f_last_error = e.what();
    }
    catch(ed::initialization_error const & e)
    {
        error_name = "ed::initialization_error";
        f_last_error = e.what();
    }
    catch(ed::runtime_error const & e)
    {
        error_name = "ed::runtime_error";
        f_last_error = e.what();
    }
    catch(std::exception const & e)
    {
        error_name = "std::exception";
        f_last_error = e.what();
    }
    catch(...)
    {
        error_name = "a non-standard exception";
        f_last_error = "Unknown exception";
    }
    f_tcp_connection.reset();

    // connection failed... we will have to try again later
    //
    SNAP_LOG_ERROR
        << "connection to "
        << addr::setaddrmode(addr::STRING_IP_BRACKET_ADDRESS | addr::STRING_IP_PORT)
        << f_address
        << " failed with: "
        << f_last_error
        << " ("
        << error_name
        << ")."
        << SNAP_LOG_SEND;
}


/** \brief Retrieve the address to connect to.
 *
 * This function returns the address the runner is trying to connect
 * to or on success, the one it is connected to.
 *
 * \note
 * Since the variable is constant, it is likely to never change.
 * However, the c_str() function may change the buffer pointer.
 * Hence, to be 100% safe, you cannot call this function until
 * you make sure that the thread is fully stopped.
 *
 * \return The destination address.
 */
addr::addr const & runner::get_address() const
{
    return f_address;
}


/** \brief Retrieve the client allocated and connected by the thread.
 *
 * This function returns the TCP connection object resulting from
 * connection attempts of the background thread.
 *
 * If the pointer is null, then you may get the corresponding
 * error message using the get_last_error() function.
 *
 * You can get the client TCP connection pointer once. After that
 * you always get a null pointer.
 *
 * \note
 * This function is guarded so the pointer and the object it
 * points to will be valid in another thread that retrieves it.
 *
 * \return The connection pointer.
 */
http_tcp_client_connection::pointer_t runner::release_client()
{
    cppthread::guard g(f_mutex);
    http_tcp_client_connection::pointer_t tcp_connection;
    tcp_connection.swap(f_tcp_connection);
    return tcp_connection;
}


/** \brief Retrieve the last error message that happened.
 *
 * This function returns the last error message that was captured
 * when trying to connect to the socket. The message is the
 * e.what() message from the exception we captured.
 *
 * The message does not get cleared so the function can be called
 * any number of times. To know whether an error was generated
 * on the last attempt, make sure to first get the get_socket()
 * and if it returns -1, then this message is significant,
 * otherwise it is from a previous error.
 *
 * \warning
 * Remember that if the background thread was used the error will
 * NOT be available in the main thread until a full memory barrier
 * was executed. For that reason we make sure that the thread
 * was stopped when we detect an error.
 *
 * \return The last error message.
 */
std::string const & runner::get_last_error() const
{
    return f_last_error;
}


/** \brief Close the connection.
 *
 * This function closes the connection. Since the f_tcp_connection
 * holds the socket to the remote server, we have get this function
 * called in order to completely disconnect.
 *
 * \note
 * This function does not clear the f_last_error parameter so it
 * can be read later.
 */
void runner::close()
{
    f_tcp_connection.reset();
}



} // namespace detail
} // namespace edhttp
// vim: ts=4 sw=4 et
