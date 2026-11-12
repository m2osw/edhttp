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
#pragma once

// self
//
#include    <edhttp/uri.h>
#include    <edhttp/utils.h>
#include    <edhttp/version.h>


// eventdispatcher
//
#include    <eventdispatcher/utils.h>


// libaddr
//
#include    <libaddr/addr.h>


// snapdev
//
#include    <snapdev/callback_manager.h>


// C++
//
#include    <functional>
#include    <vector>



namespace edhttp
{



class http_response;


typedef std::map<std::string, std::string>      header_t;
typedef buffer_t                                attachment_t;


class http_request
{
public:
    typedef std::shared_ptr<http_request>           pointer_t;
    typedef std::list<pointer_t>                    list_t;
    typedef std::function<bool(std::shared_ptr<http_response> response)>
                                                    callback_t;
    typedef snapdev::callback_manager<callback_t>   callback_manager_t;


    ed::mode_t                  get_mode() const;
    uri const &                 get_uri() const;
    addr::addr::vector_t        get_addresses() const;
    std::string                 get_host() const;
    int                         get_port() const;
    std::string                 get_agent_name() const;
    std::string                 get_method() const;
    std::string                 get_path() const;
    std::string                 get_header(std::string const & name) const;
    header_t const &            get_headers() const;
    std::string                 get_post(std::string const & name) const;
    std::string                 get_body() const; // also returns data
    std::string                 get_request(bool keep_alive) const;

    void                        set_mode(ed::mode_t mode);
    void                        set_uri(std::string const & request_uri);
    void                        set_uri(uri const & request_uri);
    void                        set_host(std::string const & host);
    void                        set_port(int port);
    void                        set_agent_name(std::string const & agent_name);
    void                        set_method(std::string const & method);
    void                        set_path(std::string const & path);
    void                        set_header(std::string name, std::string const & value);
    void                        set_post(std::string const & name, std::string const & value);
    void                        set_query_option(std::string const & name, std::string const & value);
    void                        set_basic_auth(std::string const & username, std::string const & secret);
    void                        set_data(std::string const & data);
    void                        set_body(std::string const & body);

    callback_manager_t::callback_id_t
                                add_response_callback(callback_t f);
    bool                        remove_response_callback(callback_manager_t::callback_id_t id);
    bool                        process_response(std::shared_ptr<http_response> response);

private:
    // I replaced all of these parts with the f_uri instead
    //std::string                 f_host = std::string();
    //std::int32_t                f_port = -1;
    //std::string                 f_path = std::string();
    //addr::addr_range::vector_t  f_address_ranges = addr::addr_range::vector_t();
    uri                         f_uri = uri();

    ed::mode_t                  f_mode = ed::mode_t::MODE_PLAIN;
    std::string                 f_agent_name = std::string("edhttp/" EDHTTP_VERSION_STRING);
    std::string                 f_method = std::string();
    header_t                    f_headers = header_t();
    header_t                    f_post = header_t();
    std::string                 f_body = std::string();
    std::vector<attachment_t>   f_attachments = std::vector<attachment_t>();  // not used yet (Also look in a way that allows us to avoid an extra copy)
    bool                        f_has_body = false;
    bool                        f_has_data = false;
    bool                        f_has_post = false;
    bool                        f_has_attachment = false; // not used yet
    callback_manager_t          f_response_callbacks = callback_manager_t();
};



} // namespace edhttp
// vim: ts=4 sw=4 et
