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
#include    "http/thread_signal_handler.h"

#include    "http/http_tcp_client_async_connection.h"


// last include
//
#include    <snapdev/poison.h>



namespace edhttp
{
namespace detail
{



thread_signal_handler::thread_signal_handler(http_tcp_client_async_connection * parent)
    : f_parent(parent)
{
    set_name("edhttp::detail::thread_signal_handler");
}


/** \brief This signal was emitted.
 *
 * This function gets called whenever the thread is just about to
 * quit. Calling f_thread.is_running() may still return true when
 * you get in the 'thread_done()' callback. However, an
 * f_thread.stop() will return very quickly.
 */
void thread_signal_handler::process_read()
{
    thread_done_signal::process_read();

    f_parent->trigger_thread_done();
}



} // namespace detail
} // namespace edhttp
// vim: ts=4 sw=4 et
