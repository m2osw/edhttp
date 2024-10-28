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

/** \file
 * \brief Verify the uri class.
 *
 * This file implements tests to verify that the uri class functions.
 */

// edhttp
//
#include    <edhttp/uri.h>

#include    <edhttp/exception.h>


// self
//
#include    "catch_main.h"


// snapdev
//
#include    <snapdev/tokenize_string.h>


// snaplogger
//
#include    <snaplogger/message.h>


// C
//
#include    <string.h>



CATCH_TEST_CASE("uri", "[domain]")
{
    CATCH_START_SECTION("uri: verify already canonicalized URI")
    {
        edhttp::uri uri("http://snap.website/");

        CATCH_REQUIRE(uri.domain() == "snap");
        CATCH_REQUIRE(uri.top_level_domain() == ".website");
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("uri: verify URI without '/' after domain name")
    {
        edhttp::uri uri("http://snap.website");

        CATCH_REQUIRE(uri.domain() == "snap");
        CATCH_REQUIRE(uri.top_level_domain() == ".website");
        CATCH_REQUIRE(uri.get_original_uri() == "http://snap.website");
        CATCH_REQUIRE(uri.get_uri() == "http://snap.website/");
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("uri: verify URI with two slashes")
    {
        edhttp::uri uri("http://snap.website//");

        CATCH_REQUIRE(uri.domain() == "snap");
        CATCH_REQUIRE(uri.top_level_domain() == ".website");
        CATCH_REQUIRE(uri.get_original_uri() == "http://snap.website//");
        CATCH_REQUIRE(uri.get_uri() == "http://snap.website/");
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("uri: verify URI with multiple slashes and a path")
    {
        edhttp::uri uri("http://snap.website///and/a/path");

        CATCH_REQUIRE(uri.domain() == "snap");
        CATCH_REQUIRE(uri.top_level_domain() == ".website");
        CATCH_REQUIRE(uri.path() == "and/a/path");
        CATCH_REQUIRE(uri.get_original_uri() == "http://snap.website///and/a/path");
        CATCH_REQUIRE(uri.get_uri() == "http://snap.website/and/a/path");
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("uri: verify credentials")
    {
        char const * ptr = nullptr;

        {
            edhttp::uri uri("http://explicit:credentials@snapwebsites.org:8888/and-port");

            CATCH_REQUIRE(uri.domain() == "snapwebsites");
            CATCH_REQUIRE(uri.top_level_domain() == ".org");
            CATCH_REQUIRE(uri.path() == "and-port");
            CATCH_REQUIRE(uri.get_username() == "explicit");
            CATCH_REQUIRE(uri.get_password() == "credentials");

            // check the string buffer is still saying "credentials
            //
            std::string const & p(uri.get_password());
            ptr = reinterpret_cast<char const *>(p.data());
            CATCH_REQUIRE(strncmp(ptr, "credentials", 11) == 0);
        }

        // after destruction, the buffer should not have the password anymore
        //
        // avoid that test when the sanitizer is active, it will detect that
        // we're accessing memory that was deallocated
        //
#ifndef __SANITIZE_ADDRESS__
        CATCH_REQUIRE(strncmp(ptr, "credentials", 11) != 0);
#endif
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("uri: use set_uri() to test valid URIs")
    {
        struct uri_test
        {
            char const *    f_original_uri = nullptr;           // set_uri() with original
            char const *    f_uri = nullptr;                    // canonicalized
            char const *    f_redacted_uri = nullptr;           // canonicalized & password redacted
            char const *    f_hash_bang_uri = nullptr;          // canonicalized & keep hash bang if defined
            char const *    f_website_uri = nullptr;            // scheme + full domain
            char const *    f_website_uri_with_port = nullptr;  // scheme + full domain + port
            char const *    f_domain = nullptr;                 // domain name only (no sub-domain or TLD)
            char const *    f_full_domain = nullptr;            // full domain
            char const *    f_top_level_domain = nullptr;       // the TLD of the domain
            char const *    f_sub_domains = nullptr;            // complete list of sub-domains
            char const *    f_username = nullptr;               // the user name
            char const *    f_password = nullptr;               // the password
            char const *    f_scheme = nullptr;                 // scheme name
            char const *    f_path = nullptr;                   // complete path
            int             f_path_count = 0;                   // number of segments in the complete path
            int             f_port = 0;                         // the expected port
            bool            f_is_unix = false;                  // whether the input URI is a Unix path
            char const *    f_query_string = nullptr;           // the query string
            char const *    f_anchor = nullptr;                 // the string after the '#"
            char const *    f_hash_bang_path = nullptr;         // path defined after '#!'
        };

        uri_test uris[] =
        {
            {
                .f_original_uri          = "https://me:p1@test.this.domain.net///with///a///path?var=value&other_var=more%20data#hello",
                .f_uri                   = "https://me:p1@test.this.domain.net/with/a/path?other_var=more%20data&var=value#hello",
                .f_redacted_uri          = "https://me:%2A%2A%2A@test.this.domain.net/with/a/path?other_var=more%20data&var=value#hello",
                .f_hash_bang_uri         = nullptr,
                .f_website_uri           = "https://test.this.domain.net/",
                .f_website_uri_with_port = "https://test.this.domain.net/",
                .f_domain                = "domain",
                .f_full_domain           = "test.this.domain.net",
                .f_top_level_domain      = ".net",
                .f_sub_domains           = "test.this",
                .f_username              = "me",
                .f_password              = "p1",
                .f_scheme                = "https",
                .f_path                  = "with/a/path",
                .f_path_count            = 3,
                .f_port                  = 443,
                .f_is_unix               = false,
                .f_query_string          = "other_var=more%20data&var=value",
                .f_anchor                = "hello",
                .f_hash_bang_path        = "",
            },
            {
                .f_original_uri          = "http://you:p2%5D@sub.test.this.domain.cloud///with///a///path?var=value&other_var=more%20data#!/extra/path",
                .f_uri                   = "http://you:p2%5D@sub.test.this.domain.cloud/with/a/path?other_var=more%20data&var=value",
                .f_redacted_uri          = "http://you:%2A%2A%2A@sub.test.this.domain.cloud/with/a/path?other_var=more%20data&var=value",
                .f_hash_bang_uri         = "http://you:p2%5D@sub.test.this.domain.cloud/with/a/path?other_var=more%20data&var=value#!/extra/path",
                .f_website_uri           = "http://sub.test.this.domain.cloud/",
                .f_website_uri_with_port = "http://sub.test.this.domain.cloud/",
                .f_domain                = "domain",
                .f_full_domain           = "sub.test.this.domain.cloud",
                .f_top_level_domain      = ".cloud",
                .f_sub_domains           = "sub.test.this",
                .f_username              = "you",
                .f_password              = "p2]",
                .f_scheme                = "http",
                .f_path                  = "with/a/path",
                .f_path_count            = 3,
                .f_port                  = 80,
                .f_is_unix               = false,
                .f_query_string          = "other_var=more%20data&var=value",
                .f_anchor                = "",
                .f_hash_bang_path        = "extra/path",
            },
            {
                .f_original_uri          = "ftp://you:p2%5B@sub.test.this.domain.cloud///with///a///path?var=value&other_var=more%20data#hello",
                .f_uri                   = "ftp://you:p2%5B@sub.test.this.domain.cloud/with/a/path?other_var=more%20data&var=value#hello",
                .f_redacted_uri          = "ftp://you:%2A%2A%2A@sub.test.this.domain.cloud/with/a/path?other_var=more%20data&var=value#hello",
                .f_hash_bang_uri         = nullptr,
                .f_website_uri           = "ftp://sub.test.this.domain.cloud/",
                .f_website_uri_with_port = "ftp://sub.test.this.domain.cloud/",
                .f_domain                = "domain",
                .f_full_domain           = "sub.test.this.domain.cloud",
                .f_top_level_domain      = ".cloud",
                .f_sub_domains           = "sub.test.this",
                .f_username              = "you",
                .f_password              = "p2[",
                .f_scheme                = "ftp",
                .f_path                  = "with/a/path",
                .f_path_count            = 3,
                .f_port                  = 21,
                .f_is_unix               = false,
                .f_query_string          = "other_var=more%20data&var=value",
                .f_anchor                = "hello",
                .f_hash_bang_path        = "",
            },
            {
                .f_original_uri          = "ssh://agent:secret@console.example.website///packages?PATH=/usr/bin",
                .f_uri                   = "ssh://agent:secret@console.example.website/packages?PATH=%2Fusr%2Fbin",
                .f_redacted_uri          = "ssh://agent:%2A%2A%2A@console.example.website/packages?PATH=%2Fusr%2Fbin",
                .f_hash_bang_uri         = nullptr,
                .f_website_uri           = "ssh://console.example.website/",
                .f_website_uri_with_port = "ssh://console.example.website/",
                .f_domain                = "example",
                .f_full_domain           = "console.example.website",
                .f_top_level_domain      = ".website",
                .f_sub_domains           = "console",
                .f_username              = "agent",
                .f_password              = "secret",
                .f_scheme                = "ssh",
                .f_path                  = "packages",
                .f_path_count            = 1,
                .f_port                  = 22,
                .f_is_unix               = false,
                .f_query_string          = "PATH=/usr/bin",
                .f_anchor                = "",
                .f_hash_bang_path        = "",
            },
            {
                .f_original_uri          = "telnet://user:password1@shell.example.org///packages?PATH=/usr/bin%3A/usr/sbin&=no-name",
                .f_uri                   = "telnet://user:password1@shell.example.org/packages?%2A=no-name&PATH=%2Fusr%2Fbin%3A%2Fusr%2Fsbin",
                .f_redacted_uri          = "telnet://user:%2A%2A%2A@shell.example.org/packages?%2A=no-name&PATH=%2Fusr%2Fbin%3A%2Fusr%2Fsbin",
                .f_hash_bang_uri         = nullptr,
                .f_website_uri           = "telnet://shell.example.org/",
                .f_website_uri_with_port = "telnet://shell.example.org/",
                .f_domain                = "example",
                .f_full_domain           = "shell.example.org",
                .f_top_level_domain      = ".org",
                .f_sub_domains           = "shell",
                .f_username              = "user",
                .f_password              = "password1",
                .f_scheme                = "telnet",
                .f_path                  = "packages",
                .f_path_count            = 1,
                .f_port                  = 23,
                .f_is_unix               = false,
                .f_query_string          = "%2A=no-name&PATH=/usr/bin:/usr/sbin",
                .f_anchor                = "",
                .f_hash_bang_path        = "",
            },
            {
                .f_original_uri          = "smtp://user:password1@mail.abc123.info///var/mail-boxes/user?PATH=/usr/bin%3A/usr/sbin#latest",
                .f_uri                   = "smtp://user:password1@mail.abc123.info/var/mail-boxes/user?PATH=%2Fusr%2Fbin%3A%2Fusr%2Fsbin#latest",
                .f_redacted_uri          = "smtp://user:%2A%2A%2A@mail.abc123.info/var/mail-boxes/user?PATH=%2Fusr%2Fbin%3A%2Fusr%2Fsbin#latest",
                .f_hash_bang_uri         = nullptr,
                .f_website_uri           = "smtp://mail.abc123.info/",
                .f_website_uri_with_port = "smtp://mail.abc123.info/",
                .f_domain                = "abc123",
                .f_full_domain           = "mail.abc123.info",
                .f_top_level_domain      = ".info",
                .f_sub_domains           = "mail",
                .f_username              = "user",
                .f_password              = "password1",
                .f_scheme                = "smtp",
                .f_path                  = "var/mail-boxes/user",
                .f_path_count            = 3,
                .f_port                  = 25,
                .f_is_unix               = false,
                .f_query_string          = "PATH=/usr/bin:/usr/sbin",
                .f_anchor                = "latest",
                .f_hash_bang_path        = "",
            },
            {
                .f_original_uri          = "gopher://gofer:yes%3Ano@gopher.wall.alexis.museum///gopher",
                .f_uri                   = "gopher://gofer:yes%3Ano@gopher.wall.alexis.museum/gopher",
                .f_redacted_uri          = "gopher://gofer:%2A%2A%2A@gopher.wall.alexis.museum/gopher",
                .f_hash_bang_uri         = nullptr,
                .f_website_uri           = "gopher://gopher.wall.alexis.museum/",
                .f_website_uri_with_port = "gopher://gopher.wall.alexis.museum/",
                .f_domain                = "alexis",
                .f_full_domain           = "gopher.wall.alexis.museum",
                .f_top_level_domain      = ".museum",
                .f_sub_domains           = "gopher.wall",
                .f_username              = "gofer",
                .f_password              = "yes:no",
                .f_scheme                = "gopher",
                .f_path                  = "gopher",
                .f_path_count            = 1,
                .f_port                  = 70,
                .f_is_unix               = false,
                .f_query_string          = "",
                .f_anchor                = "",
                .f_hash_bang_path        = "",
            },
            {
                .f_original_uri          = "time://realtime.atomic.cl/utc?leap-seconds=separate",
                .f_uri                   = "time://realtime.atomic.cl/utc?leap-seconds=separate",
                .f_redacted_uri          = "time://realtime.atomic.cl/utc?leap-seconds=separate",
                .f_hash_bang_uri         = nullptr,
                .f_website_uri           = "time://realtime.atomic.cl/",
                .f_website_uri_with_port = "time://realtime.atomic.cl/",
                .f_domain                = "atomic",
                .f_full_domain           = "realtime.atomic.cl",
                .f_top_level_domain      = ".cl",
                .f_sub_domains           = "realtime",
                .f_username              = "",
                .f_password              = "",
                .f_scheme                = "time",
                .f_path                  = "utc",
                .f_path_count            = 1,
                .f_port                  = 37,
                .f_is_unix               = false,
                .f_query_string          = "leap-second=separate",
                .f_anchor                = "",
                .f_hash_bang_path        = "",
            },
            {
                .f_original_uri          = "snapwebsites://@parlement.co.uk/////folder/electric/bill?line%5B3%5D=129.07#quantity",
                .f_uri                   = "snapwebsites://parlement.co.uk:80/folder/electric/bill?line%5B3%5D=129.07#quantity",
                .f_redacted_uri          = "snapwebsites://parlement.co.uk:80/folder/electric/bill?line%5B3%5D=129.07#quantity",
                .f_hash_bang_uri         = nullptr,
                .f_website_uri           = "snapwebsites://parlement.co.uk/",
                .f_website_uri_with_port = "snapwebsites://parlement.co.uk:80/",
                .f_domain                = "parlement",
                .f_full_domain           = "parlement.co.uk",
                .f_top_level_domain      = ".co.uk",
                .f_sub_domains           = "",
                .f_username              = "",
                .f_password              = "",
                .f_scheme                = "snapwebsites",
                .f_path                  = "folder/electric/bill",
                .f_path_count            = 3,
                .f_port                  = 80,
                .f_is_unix               = false,
                .f_query_string          = "line[3]=129.07",
                .f_anchor                = "quantity",
                .f_hash_bang_path        = "",
            },
        };

    // query string handling
    //void                        set_query_option(std::string const & name, std::string const & value);
    //void                        unset_query_option(std::string const & name);
    //void                        set_query_string(std::string const & uri_query_string);
    //std::string                 query_string() const;
    //bool                        has_query_option(std::string const & name) const;
    //void                        clear_query_options();
    //std::string                 query_option(std::string const & name) const;
    //int                         query_option_count() const;
    //std::string                 query_option(int part, std::string & name) const;
    //uri_options_t const &       query_string_list() const;

        int part(0);
        for(auto t : uris)
        {
            edhttp::uri uri(t.f_original_uri);

            // sub-domains
            //
            std::list<std::string> sub_domains;
            snapdev::tokenize_string(sub_domains, t.f_sub_domains, {"."}, true);

            // path segments
            //
            std::list<std::string> segments;
            snapdev::tokenize_string(segments, t.f_path, {"/"}, true);

            // verify each value for each original
            //
            CATCH_REQUIRE(uri.get_original_uri() == t.f_original_uri);
            CATCH_REQUIRE(uri.get_uri() == t.f_uri);
            CATCH_REQUIRE(uri.get_uri(false, "***") == t.f_redacted_uri);
            if(t.f_hash_bang_uri != nullptr)
            {
                CATCH_REQUIRE(uri.get_uri(true) == t.f_hash_bang_uri);
            }
            CATCH_REQUIRE(uri.get_website_uri() == t.f_website_uri);
            CATCH_REQUIRE(uri.get_website_uri(true) == t.f_website_uri_with_port);
            CATCH_REQUIRE(uri.domain() == t.f_domain);
            CATCH_REQUIRE(uri.sub_domains() == t.f_sub_domains);
            CATCH_REQUIRE(uri.sub_domain_count() == static_cast<int>(sub_domains.size()));
            CATCH_REQUIRE(uri.full_domain() == t.f_full_domain);
            CATCH_REQUIRE(uri.top_level_domain() == t.f_top_level_domain);
            CATCH_REQUIRE(uri.get_username() == t.f_username);
            CATCH_REQUIRE(uri.get_password() == t.f_password);
            CATCH_REQUIRE(uri.scheme() == t.f_scheme);
            CATCH_REQUIRE(uri.path() == t.f_path);
            CATCH_REQUIRE(uri.path_count() == t.f_path_count);
            CATCH_REQUIRE(uri.get_port() == t.f_port);
            CATCH_REQUIRE(uri.is_unix() == t.f_is_unix);
            CATCH_REQUIRE(uri.get_str_port() == std::to_string(t.f_port));
            CATCH_REQUIRE(uri.anchor() == t.f_anchor);
            CATCH_REQUIRE(uri.hash_bang_path() == t.f_hash_bang_path);

            advgetopt::string_list_t const & sub_domain_list(uri.sub_domains_list());
            CATCH_REQUIRE(sub_domain_list.size() == sub_domains.size());
            auto sub_domain_it(sub_domain_list.begin());
            part = 0;
            for(auto const & s : sub_domains)
            {
                CATCH_REQUIRE(uri.sub_domain(part) == s);
                CATCH_REQUIRE(*sub_domain_it == s);
                CATCH_REQUIRE(uri.get_part("sub-domain", part) == s);
                ++sub_domain_it;
                ++part;
            }

            advgetopt::string_list_t const & path_list(uri.path_list());
            CATCH_REQUIRE(path_list.size() == segments.size());
            auto path_it(path_list.begin());
            part = 0;
            for(auto const & s : segments)
            {
                CATCH_REQUIRE(uri.path_folder_name(part) == s);
                CATCH_REQUIRE(*path_it == s);
                CATCH_REQUIRE(uri.get_part("path", part) == s);
                ++path_it;
                ++part;
            }

            // try again with the get_part() function
            //
            CATCH_REQUIRE(uri.get_part("anchor") == t.f_anchor);
            CATCH_REQUIRE(uri.get_part("domain") == t.f_domain);
            CATCH_REQUIRE(uri.get_part("full-domain") == t.f_full_domain);
            CATCH_REQUIRE(uri.get_part("is-unix") == (t.f_is_unix ? "unix" : "inet"));
            CATCH_REQUIRE(uri.get_part("original") == t.f_original_uri);
            CATCH_REQUIRE(uri.get_part("password") == t.f_password);
            CATCH_REQUIRE(uri.get_part("path-count") == std::to_string(t.f_path_count));
            CATCH_REQUIRE(uri.get_part("port") == std::to_string(t.f_port));
            CATCH_REQUIRE(uri.get_part("scheme") == t.f_scheme);
            CATCH_REQUIRE(uri.get_part("tld") == t.f_top_level_domain);
            CATCH_REQUIRE(uri.get_part("top-level-domain") == t.f_top_level_domain);
            CATCH_REQUIRE(uri.get_part("uri") == t.f_uri);
            CATCH_REQUIRE(uri.get_part("username") == t.f_username);

            // unknown parts are accepted and the function returns an empty string
            //
            CATCH_REQUIRE(uri.get_part("anything-else-is-empty") == std::string());
        }
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("uri: use set_uri() to test invalid URIs")
    {
        struct uri_test
        {
            char const *    f_uri = nullptr;                    // some invalid URI
            char const *    f_error_message = nullptr;          // the expected error message
        };

        uri_test uris[] =
        {
            {
                .f_uri           = "https",
                .f_error_message = "scheme not followed by \"://\".",
            },
            {
                .f_uri           = "https://@m2osw.com:80:80/",
                .f_error_message = "more than one ':' in the domain name segment (after the '@') [1].",
            },
            {
                .f_uri           = "https:///this.domain.net///with///a///path?var=value&other_var=more%20data#hello",
                .f_error_message = "a domain name is required.",
            },
            {
                .f_uri           = "https://top:secret:password@m2osw.com:80:80/",
                .f_error_message = "more than one ':' in the login info segment (before the '@').",
            },
            {
                .f_uri           = "https://top:secret@password@m2osw.com:80:80/",
                .f_error_message = "more than one '@' character found.",
            },
            {
                .f_uri           = "https://my:password@m2osw.com:80:80/",
                .f_error_message = "more than one ':' in the domain name segment (after the '@') [2].",
            },
            {
                .f_uri           = "https://empty:port@m2osw.com:/",
                .f_error_message = "port cannot be an empty string.",
            },
            {
                .f_uri           = "https://empty:port@m2osw.com:http/",
                .f_error_message = "port must be a valid decimal number ('http' unexpected).",
            },
            {
                .f_uri           = "https://big:port@m2osw.com:65536/",
                .f_error_message = "port must be between 0 and 65536.",
            },
            {
                .f_uri           = "https://no:domain@:65535/no/domain",
                .f_error_message = "a domain name is required.",
            },
            {
                .f_uri           = "https://empty:@password.m2osw.com:1001/",
                .f_error_message = "username and password must both be defined (or define neither).",
            },
            {
                .f_uri           = "https://:empty@password.m2osw.com:1001/",
                .f_error_message = "username and password must both be defined (or define neither).",
            },
            {
                .f_uri           = "https://utc.m2osw.clock/",
                .f_error_message = "could not verify domain name \"utc.m2osw.clock\".",
            },
            {
                .f_uri           = "https://utc.m2osw.co/?a=1&a=3",
                .f_error_message = "query string \"a\" found more than once.",
            },
            {
                .f_uri           = "https://parent.m2osw.co/..",
                .f_error_message = "found \"..\" at the beginning of your path.",
            },
            {
                .f_uri           = "https://parent.m2osw.co/../none",
                .f_error_message = "found \"..\" at the beginning of your path.",
            },
            {
                .f_uri           = "https://parent.m2osw.co/./../sub-domain",
                .f_error_message = "found \"..\" at the beginning of your path.",
            },
        };

        for(auto t : uris)
        {
            edhttp::uri uri;

            CATCH_REQUIRE_FALSE(uri.set_uri(t.f_uri));
            CATCH_REQUIRE(uri.get_last_error_message() == t.f_error_message);
            uri.clear_last_error_message();
            CATCH_REQUIRE(uri.get_last_error_message().empty());
        }
    }
    CATCH_END_SECTION()
}


CATCH_TEST_CASE("uri_error", "[domain][error]")
{
    CATCH_START_SECTION("uri_error: create uri with an invalid string")
    {
        CATCH_REQUIRE_THROWS_MATCHES(
                  edhttp::uri("bad_URI")
                , edhttp::invalid_uri
                , Catch::Matchers::ExceptionMessage(
                          "edhttp_exception: URI \"bad_URI\" is considered invalid."));
    }
    CATCH_END_SECTION()
}


// vim: ts=4 sw=4 et
