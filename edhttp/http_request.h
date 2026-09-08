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

// eventdispatcher
//
#include    <eventdispatcher/tcp_bio_client.h>


// libaddr
//
#include    <libaddr/addr_range.h>


// C++
//
#include    <map>
#include    <vector>



namespace edhttp
{



// name / value pairs

// attachment buffer


class http_request
{
public:
    typedef std::shared_ptr<http_request>       pointer_t;

    addr::addr_range::vector_t  get_address_ranges() const;
    bool                        unique_host() const;
    std::string                 get_host() const;
    bool                        unique_port() const;
    int                         get_port() const;
    std::string                 get_agent_name() const;
    std::string                 get_method() const;
    std::string                 get_path() const;
    std::string                 get_header(std::string const & name) const;
    std::string                 get_post(std::string const & name) const;
    std::string                 get_body() const; // also returns data
    std::string                 get_request(bool keep_alive) const;

    void                        set_uri(std::string const & uri);
    void                        set_address_ranges(addr::addr_range::vector_t const & address_ranges);
    void                        set_host(std::string const & host);
    void                        set_port(int port);
    void                        set_agent_name(std::string const & agent_name);
    void                        set_method(std::string const & method);
    void                        set_path(std::string const & path);
    void                        set_header(std::string const & name, std::string const & value);
    void                        set_post(std::string const & name, std::string const & value);
    void                        set_basic_auth(std::string const & username, std::string const & secret);
    void                        set_data(std::string const & data);
    void                        set_body(std::string const & body);

private:
    typedef std::map<std::string, std::string>  header_t;
    typedef std::vector<char>                   attachment_t;

    //std::string                 f_host = std::string();
    //std::int32_t                f_port = -1;
    addr::addr_range::vector_t  f_address_ranges = addr::addr_range::vector_t();
    std::string                 f_agent_name = std::string("edhttp");
    std::string                 f_method = std::string();
    std::string                 f_path = std::string();
    header_t                    f_headers = header_t();
    header_t                    f_post = header_t();
    std::string                 f_body = std::string();
    std::vector<attachment_t>   f_attachments = std::vector<attachment_t>();  // not used yet (Also look in a way that allows us to avoid an extra copy)
    bool                        f_has_body = false;
    bool                        f_has_data = false;
    bool                        f_has_post = false;
    bool                        f_has_attachment = false; // not used yet
};



} // namespace edhttp
// vim: ts=4 sw=4 et
