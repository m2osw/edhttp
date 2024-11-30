// Copyright (c) 2011-2024  Made to Order Software Corp.  All Rights Reserved
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
#include    <edhttp/names.h>


// libaddr
//
#include    <libaddr/addr_range.h>


// advgetopt
//
#include    <advgetopt/utils.h>


// C++
//
#include    <map>



namespace edhttp
{



// Helper class to handle URIs
// http://tools.ietf.org/html/rfc3986
class uri
{
public:
    // types used by this class
    typedef std::map<std::string, std::string>
                                uri_options_t;

    // constructors/destructor
                                uri();
                                uri(std::string const & uri, bool accept_path = false);
                                ~uri();

    // URI handling
    bool                        set_uri(
                                      std::string const & uri
                                    , bool accept_path = false
                                    , bool accept_ip = false);
    std::string const &         get_original_uri() const;
    std::string                 get_uri(
                                      bool use_hash_bang = false
                                    , std::string const & redact = std::string()) const;
    std::string                 get_website_uri(bool include_port = false) const;
    std::string                 get_last_error_message() const;
    void                        clear_last_error_message();

    // get a part by name
    std::string                 get_part(std::string const & name, int part = -1) const;

    // user/password
    void                        set_username(std::string const & username);
    std::string const &         get_username() const;
    void                        set_password(std::string const & password);
    std::string const &         get_password() const;

    // scheme handling
    void                        set_scheme(std::string const & uri_scheme);
    std::string const &         scheme() const;

    // domain & sub-domains handling
    void                        set_domain(std::string const & full_domain_name);
    std::string                 full_domain() const;
    std::string const &         top_level_domain() const;
    std::string const &         domain() const;
    std::string                 sub_domains() const;
    int                         sub_domain_count() const;
    std::string                 sub_domain(int part) const;
    advgetopt::string_list_t const &
                                sub_domains_list() const;
    addr::addr_range::vector_t const &
                                address_ranges();

    // port handling
    void                        set_port(std::string const & port);
    void                        set_port(int port);
    int                         get_port() const;
    std::string                 get_str_port() const;

    // path handling
    bool                        is_unix() const;
    void                        set_path(std::string uri_path);
    std::string                 path(bool encoded = true) const;
    int                         path_count() const;
    std::string                 path_folder_name(int part) const;
    advgetopt::string_list_t const &
                                path_list() const;
    std::string                 hash_bang_path(bool encoded = true) const;

    // option handling
    void                        set_option(std::string const & name, std::string const & value);
    void                        unset_option(std::string const & name);
    std::string                 option(std::string const & name) const;
    int                         option_count() const;
    std::string                 option(int part, std::string & name) const;
    uri_options_t const &       options_list() const;

    // query string handling
    void                        set_query_option(std::string const & name, std::string const & value);
    void                        unset_query_option(std::string const & name);
    void                        set_query_string(std::string const & uri_query_string);
    std::string                 query_string() const;
    bool                        has_query_option(std::string const & name) const;
    void                        clear_query_options();
    std::string                 query_option(std::string const & name) const;
    int                         query_option_count() const;
    std::string                 query_option(int part, std::string & name) const;
    uri_options_t const &       query_string_list() const;

    // anchor handling (note: "#!" is not considered an anchor)
    void                        set_anchor(std::string const & uri_anchor);
    std::string const &         anchor() const;

    // operators
    bool                        operator == (uri const & rhs) const;
    bool                        operator != (uri const & rhs) const;
    bool                        operator <  (uri const & rhs) const;
    bool                        operator <= (uri const & rhs) const;
    bool                        operator >  (uri const & rhs) const;
    bool                        operator >= (uri const & rhs) const;

    static std::string          urlencode(std::string const & uri, char const * accepted = "");
    static std::string          urldecode(std::string const & uri, bool relax = false);
    static int                  scheme_to_port(std::string const & uri_scheme);

private:
    bool                        process_domain(
                                      std::string const & full_domain_name
                                    , advgetopt::string_list_t & sub_domain_names
                                    , std::string & domain_name
                                    , std::string & tld);
    bool                        clean_path(advgetopt::string_list_t & path_segments);

    // f_original is the unchanged source (from constructor or
    // last set_uri() call)
    std::string                 f_original = std::string();
    std::string                 f_scheme = std::string(g_name_edhttp_scheme_http);
    std::string                 f_username = std::string();
    std::string                 f_password = std::string();
    int                         f_port = 80;
    std::string                 f_domain = std::string();
    std::string                 f_top_level_domain = std::string();
    advgetopt::string_list_t    f_sub_domains = advgetopt::string_list_t();
    advgetopt::string_list_t    f_path = advgetopt::string_list_t();
    advgetopt::string_list_t    f_hash_bang_path = advgetopt::string_list_t();
    uri_options_t               f_options = uri_options_t();
    uri_options_t               f_query_strings = uri_options_t();
    std::string                 f_anchor = std::string();
    addr::addr_range::vector_t  f_address_ranges = addr::addr_range::vector_t();
    std::string                 f_last_error_message = std::string();
};



} // namespace edhttp
// vim: ts=4 sw=4 et
