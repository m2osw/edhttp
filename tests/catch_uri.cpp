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



CATCH_TEST_CASE("uri", "[uri][domain]")
{
    CATCH_START_SECTION("uri: verify already canonicalized URI")
    {
        edhttp::uri u("http://snap.website/");

        CATCH_REQUIRE(u.domain() == "snap");
        CATCH_REQUIRE(u.top_level_domain() == ".website");
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("uri: verify URI without '/' after domain name")
    {
        edhttp::uri u("http://snap.website");

        CATCH_REQUIRE(u.domain() == "snap");
        CATCH_REQUIRE(u.top_level_domain() == ".website");
        CATCH_REQUIRE(u.get_original_uri() == "http://snap.website");
        CATCH_REQUIRE(u.get_uri() == "http://snap.website/");
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("uri: verify URI with two slashes")
    {
        edhttp::uri u("http://snap.website//");

        CATCH_REQUIRE(u.domain() == "snap");
        CATCH_REQUIRE(u.top_level_domain() == ".website");
        CATCH_REQUIRE(u.get_original_uri() == "http://snap.website//");
        CATCH_REQUIRE(u.get_uri() == "http://snap.website/");
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("uri: verify URI with multiple slashes and a path")
    {
        edhttp::uri u("http://snap.website///and/a/path");

        CATCH_REQUIRE(u.domain() == "snap");
        CATCH_REQUIRE(u.top_level_domain() == ".website");
        CATCH_REQUIRE(u.path() == "and/a/path");
        CATCH_REQUIRE(u.get_original_uri() == "http://snap.website///and/a/path");
        CATCH_REQUIRE(u.get_uri() == "http://snap.website/and/a/path");

        u.set_path("//this//new//path//");
        CATCH_REQUIRE(u.path() == "this/new/path");

        u.set_path("append///this//too");
        CATCH_REQUIRE(u.path() == "this/new/path/append/this/too");

        u.set_path("//dots//like//.//are//ignored//.");
        CATCH_REQUIRE(u.path() == "dots/like/are/ignored");

        u.set_path("//double-dots//like//..//are//removed//poof//..");
        CATCH_REQUIRE(u.path() == "double-dots/are/removed");
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("uri: verify anchor")
    {
        edhttp::uri u("http://snapwebsites.org:8585/check/anchor#this-anchor");

        CATCH_REQUIRE(u.domain() == "snapwebsites");
        CATCH_REQUIRE(u.top_level_domain() == ".org");
        CATCH_REQUIRE(u.get_port() == 8585);
        CATCH_REQUIRE(u.path() == "check/anchor");
        CATCH_REQUIRE(u.anchor() == "this-anchor");
        u.set_anchor("change-it");
        CATCH_REQUIRE(u.anchor() == "change-it");
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("uri: query string list")
    {
        edhttp::uri u("http://snapwebsites.org:8484/?a=123&b=999&c-is-valid-too");

        CATCH_REQUIRE(u.domain() == "snapwebsites");
        CATCH_REQUIRE(u.top_level_domain() == ".org");
        CATCH_REQUIRE(u.get_port() == 8484);
        CATCH_REQUIRE(u.path().empty());
        CATCH_REQUIRE(u.anchor().empty());
        CATCH_REQUIRE(u.query_option_count() == 3);
        auto const & list(u.query_string_list());
        CATCH_REQUIRE(list.size() == 3);
        CATCH_REQUIRE(list.contains("a"));
        CATCH_REQUIRE(list.contains("b"));
        CATCH_REQUIRE(list.contains("c-is-valid-too"));
        CATCH_REQUIRE(list.at("a") == "123");
        CATCH_REQUIRE(list.at("b") == "999");
        CATCH_REQUIRE(list.at("c-is-valid-too") == "");
        CATCH_REQUIRE(u.query_option("a") == "123");
        CATCH_REQUIRE(u.query_option("b") == "999");
        CATCH_REQUIRE(u.query_option("c-is-valid-too") == "");
        CATCH_REQUIRE(u.query_option("undefined") == "");
        CATCH_REQUIRE(u.has_query_option("a"));
        CATCH_REQUIRE(u.has_query_option("b"));
        CATCH_REQUIRE(u.has_query_option("c-is-valid-too"));
        CATCH_REQUIRE_FALSE(u.has_query_option("undefined"));

        std::string name;
        CATCH_REQUIRE(u.query_option(0, name) == "123");
        CATCH_REQUIRE(name == "a");
        CATCH_REQUIRE(u.query_option(1, name) == "999");
        CATCH_REQUIRE(name == "b");
        CATCH_REQUIRE(u.query_option(2, name) == "");
        CATCH_REQUIRE(name == "c-is-valid-too");

        CATCH_REQUIRE(u.query_string() == "a=123&b=999&c-is-valid-too");

        u.set_query_option("d", "*6#7(x+y)");
        CATCH_REQUIRE(u.query_option_count() == 4);
        CATCH_REQUIRE(u.query_string() == "a=123&b=999&c-is-valid-too&d=%2A6%237%28x%2By%29");
        u.unset_query_option("d");
        CATCH_REQUIRE(u.query_string() == "a=123&b=999&c-is-valid-too");

        u.add_query_option("q", "magic");
        CATCH_REQUIRE(u.query_option("q") == "magic");
        u.add_query_option("q", "hall");
        CATCH_REQUIRE(u.query_option("q") == "magic,hall");

        u.clear_query_options();
        CATCH_REQUIRE(u.query_option_count() == 0);
        CATCH_REQUIRE(u.query_string() == std::string());

        u.set_query_string("a=%2A&b=%23&c=good%11progress");
        CATCH_REQUIRE(u.query_option_count() == 3);
        CATCH_REQUIRE(list.at("a") == "*");
        CATCH_REQUIRE(list.at("b") == "#");
        CATCH_REQUIRE(list.at("c") == "good\x11progress");

        u.set_query_option("", "ignored");
        CATCH_REQUIRE(u.query_option_count() == 3);
        CATCH_REQUIRE(u.query_option("*") == "");

        u.set_query_string("d=&e%23t=%25XY&=no%20name&no-equal");
        CATCH_REQUIRE(u.query_option_count() == 7);
        CATCH_REQUIRE(list.at("a") == "*");
        CATCH_REQUIRE(list.at("b") == "#");
        CATCH_REQUIRE(list.at("c") == "good\x11progress");
        CATCH_REQUIRE(u.has_query_option("d"));
        CATCH_REQUIRE(list.at("d") == "");
        CATCH_REQUIRE(list.at("e#t") == "%XY");
        CATCH_REQUIRE(list.at("*") == "no name");
        CATCH_REQUIRE(u.has_query_option("no-equal"));
        CATCH_REQUIRE(list.at("no-equal") == "");
        CATCH_REQUIRE_FALSE(u.has_query_option(std::string()));
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("uri: verify port without any '@' characters")
    {
        edhttp::uri u("http://snapwebsites.org:8585/check/port");

        CATCH_REQUIRE(u.domain() == "snapwebsites");
        CATCH_REQUIRE(u.top_level_domain() == ".org");
        CATCH_REQUIRE(u.get_port() == 8585);
        CATCH_REQUIRE(u.path() == "check/port");
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("uri: compares just like strings, but canonicalized")
    {
        edhttp::uri u1("http://snapwebsites.org:8585/check/port");
        edhttp::uri u2("http://snapwebsites.org:8585///check///port");
        CATCH_REQUIRE(u1 == u2);

        edhttp::uri u3("http://snapwebsites.org:5858///check///port");
        CATCH_REQUIRE(u1 > u3);
        CATCH_REQUIRE(u2 >= u3);
        CATCH_REQUIRE(u3 < u2);
        CATCH_REQUIRE(u3 <= u1);
        CATCH_REQUIRE(u3 != u1);
        CATCH_REQUIRE(u2 != u3);
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("uri: verify a bare IP")
    {
        edhttp::uri u;

        CATCH_REQUIRE(u.set_uri("ftp://127.0.0.1:9191/check/ipv4", true, true));
        CATCH_REQUIRE(u.scheme() == "ftp");
        CATCH_REQUIRE(u.domain() == "127.0.0.1");
        CATCH_REQUIRE(u.top_level_domain() == "");
        CATCH_REQUIRE(u.get_port() == 9191);
        CATCH_REQUIRE(u.path() == "check/ipv4");

        CATCH_REQUIRE(u.set_uri("http://[0123:4567:89ab:cdef::1]:1919/check/ipv6", true, true));
        CATCH_REQUIRE(u.scheme() == "http");
        CATCH_REQUIRE(u.domain() == "[123:4567:89ab:cdef::1]");
        CATCH_REQUIRE(u.top_level_domain() == "");
        CATCH_REQUIRE(u.get_port() == 1919);
        CATCH_REQUIRE(u.path() == "check/ipv6");

        CATCH_REQUIRE(u.set_uri("https://[::ffff:127.0.0.6]:2323/check/ipv4/in/ipv6", true, true));
        CATCH_REQUIRE(u.scheme() == "https");
        CATCH_REQUIRE(u.domain() == "127.0.0.6");
        CATCH_REQUIRE(u.top_level_domain() == "");
        CATCH_REQUIRE(u.get_port() == 2323);
        CATCH_REQUIRE(u.path() == "check/ipv4/in/ipv6");
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("uri: verify credentials")
    {
        char const * ptr = nullptr;

        {
            edhttp::uri u("http://explicit:credentials@snapwebsites.org:8888/and-port");

            CATCH_REQUIRE(u.domain() == "snapwebsites");
            CATCH_REQUIRE(u.top_level_domain() == ".org");
            CATCH_REQUIRE(u.get_port() == 8888);
            CATCH_REQUIRE(u.path() == "and-port");
            CATCH_REQUIRE(u.get_username() == "explicit");
            CATCH_REQUIRE(u.get_password() == "credentials");

            // check the string buffer is still saying "credentials
            //
            std::string const & p(u.get_password());
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
            char const *    f_website_uri = nullptr;            // scheme + full domain
            char const *    f_website_uri_with_port = nullptr;  // scheme + full domain + port
            char const *    f_domain = nullptr;                 // domain name only (no sub-domain or TLD)
            char const *    f_full_domain = nullptr;            // full domain
            char const *    f_top_level_domain = nullptr;       // the TLD of the domain
            char const *    f_sub_domains = nullptr;            // complete list of sub-domains
            char const *    f_username = nullptr;               // the user name
            char const *    f_password = nullptr;               // the password
            char const *    f_scheme = nullptr;                 // scheme name
            bool            f_path_encoded = true;              // whether to encode the path or not
            char const *    f_path = nullptr;                   // complete path
            int             f_path_count = 0;                   // number of segments in the complete path
            int             f_port = 0;                         // the expected port
            bool            f_is_port_secure = false;           // whether the port is considered secure
            bool            f_is_unix = false;                  // whether the input URI is a Unix path
            char const *    f_query_string = nullptr;           // the query string
            char const *    f_anchor = nullptr;                 // the string after the '#"
        };

        uri_test uris[] =
        {
            {
                .f_original_uri          = "https://me:p1@test.this.domain.net///with///a///path?var=value&other_var=more%20data#hello",
                .f_uri                   = "https://me:p1@test.this.domain.net/with/a/path?other_var=more%20data&var=value#hello",
                .f_redacted_uri          = "https://me:%2A%2A%2A@test.this.domain.net/with/a/path?other_var=more%20data&var=value#hello",
                .f_website_uri           = "https://test.this.domain.net/",
                .f_website_uri_with_port = "https://test.this.domain.net/",
                .f_domain                = "domain",
                .f_full_domain           = "test.this.domain.net",
                .f_top_level_domain      = ".net",
                .f_sub_domains           = "test.this",
                .f_username              = "me",
                .f_password              = "p1",
                .f_scheme                = "https",
                .f_path_encoded          = true,
                .f_path                  = "with/a/path",
                .f_path_count            = 3,
                .f_port                  = 443,
                .f_is_port_secure        = true,
                .f_is_unix               = false,
                .f_query_string          = "other_var=more%20data&var=value",
                .f_anchor                = "hello",
            },
            {
                .f_original_uri          = "http://you:p2%5D@sub.test.this.domain.cloud///%25with///a///path?var=value&other_var=more%20data#!/extra/path",
                .f_uri                   = "http://you:p2%5D@sub.test.this.domain.cloud/%25with/a/path?other_var=more%20data&var=value#!/extra/path",
                .f_redacted_uri          = "http://you:%2A%2A%2A@sub.test.this.domain.cloud/%25with/a/path?other_var=more%20data&var=value#!/extra/path",
                .f_website_uri           = "http://sub.test.this.domain.cloud/",
                .f_website_uri_with_port = "http://sub.test.this.domain.cloud/",
                .f_domain                = "domain",
                .f_full_domain           = "sub.test.this.domain.cloud",
                .f_top_level_domain      = ".cloud",
                .f_sub_domains           = "sub.test.this",
                .f_username              = "you",
                .f_password              = "p2]",
                .f_scheme                = "http",
                .f_path_encoded          = true,
                .f_path                  = "%25with/a/path",
                .f_path_count            = 3,
                .f_port                  = 80,
                .f_is_port_secure        = false,
                .f_is_unix               = false,
                .f_query_string          = "other_var=more%20data&var=value",
                .f_anchor                = "!/extra/path",
            },
            {
                .f_original_uri          = "http://you:p2%5D@sub.test.this.domain.cloud///%25with///a///path?var=value&other_var=more%20data#!~alexis/extra/path",
                .f_uri                   = "http://you:p2%5D@sub.test.this.domain.cloud/%25with/a/path?other_var=more%20data&var=value#!~alexis/extra/path",
                .f_redacted_uri          = "http://you:%2A%2A%2A@sub.test.this.domain.cloud/%25with/a/path?other_var=more%20data&var=value#!~alexis/extra/path",
                .f_website_uri           = "http://sub.test.this.domain.cloud/",
                .f_website_uri_with_port = "http://sub.test.this.domain.cloud/",
                .f_domain                = "domain",
                .f_full_domain           = "sub.test.this.domain.cloud",
                .f_top_level_domain      = ".cloud",
                .f_sub_domains           = "sub.test.this",
                .f_username              = "you",
                .f_password              = "p2]",
                .f_scheme                = "http",
                .f_path_encoded          = false,
                .f_path                  = "%with/a/path",
                .f_path_count            = 3,
                .f_port                  = 80,
                .f_is_port_secure        = false,
                .f_is_unix               = false,
                .f_query_string          = "other_var=more%20data&var=value",
                .f_anchor                = "!~alexis/extra/path",
            },
            {
                .f_original_uri          = "http://you:p2%5D@sub.test.this.domain.cloud///with///a///path?var=value&other_var=more%20data#!/special/path/%23%25%20",
                .f_uri                   = "http://you:p2%5D@sub.test.this.domain.cloud/with/a/path?other_var=more%20data&var=value#!/special/path/%23%25%20",
                .f_redacted_uri          = "http://you:%2A%2A%2A@sub.test.this.domain.cloud/with/a/path?other_var=more%20data&var=value#!/special/path/%23%25%20",
                .f_website_uri           = "http://sub.test.this.domain.cloud/",
                .f_website_uri_with_port = "http://sub.test.this.domain.cloud/",
                .f_domain                = "domain",
                .f_full_domain           = "sub.test.this.domain.cloud",
                .f_top_level_domain      = ".cloud",
                .f_sub_domains           = "sub.test.this",
                .f_username              = "you",
                .f_password              = "p2]",
                .f_scheme                = "http",
                .f_path_encoded          = true,
                .f_path                  = "with/a/path",
                .f_path_count            = 3,
                .f_port                  = 80,
                .f_is_port_secure        = false,
                .f_is_unix               = false,
                .f_query_string          = "other_var=more%20data&var=value",
                .f_anchor                = "!/special/path/#% ",
            },
            {
                .f_original_uri          = "ftp://you:p2%5B@sub.test.this.domain.cloud///with///a///path?var=value&other_var=more%20data#hello",
                .f_uri                   = "ftp://you:p2%5B@sub.test.this.domain.cloud/with/a/path?other_var=more%20data&var=value#hello",
                .f_redacted_uri          = "ftp://you:%2A%2A%2A@sub.test.this.domain.cloud/with/a/path?other_var=more%20data&var=value#hello",
                .f_website_uri           = "ftp://sub.test.this.domain.cloud/",
                .f_website_uri_with_port = "ftp://sub.test.this.domain.cloud/",
                .f_domain                = "domain",
                .f_full_domain           = "sub.test.this.domain.cloud",
                .f_top_level_domain      = ".cloud",
                .f_sub_domains           = "sub.test.this",
                .f_username              = "you",
                .f_password              = "p2[",
                .f_scheme                = "ftp",
                .f_path_encoded          = true,
                .f_path                  = "with/a/path",
                .f_path_count            = 3,
                .f_port                  = 21,
                .f_is_port_secure        = false,
                .f_is_unix               = false,
                .f_query_string          = "other_var=more%20data&var=value",
                .f_anchor                = "hello",
            },
            {
                .f_original_uri          = "ssh://agent:secret@console.example.website///packages?PATH=/usr/bin",
                .f_uri                   = "ssh://agent:secret@console.example.website/packages?PATH=%2Fusr%2Fbin",
                .f_redacted_uri          = "ssh://agent:%2A%2A%2A@console.example.website/packages?PATH=%2Fusr%2Fbin",
                .f_website_uri           = "ssh://console.example.website/",
                .f_website_uri_with_port = "ssh://console.example.website/",
                .f_domain                = "example",
                .f_full_domain           = "console.example.website",
                .f_top_level_domain      = ".website",
                .f_sub_domains           = "console",
                .f_username              = "agent",
                .f_password              = "secret",
                .f_scheme                = "ssh",
                .f_path_encoded          = true,
                .f_path                  = "packages",
                .f_path_count            = 1,
                .f_port                  = 22,
                .f_is_port_secure        = true,
                .f_is_unix               = false,
                .f_query_string          = "PATH=/usr/bin",
                .f_anchor                = "",
            },
            {
                .f_original_uri          = "telnet://user:password1@shell.example.org///packages?PATH=/usr/bin%3A/usr/sbin&=no-name",
                .f_uri                   = "telnet://user:password1@shell.example.org/packages?%2A=no-name&PATH=%2Fusr%2Fbin%3A%2Fusr%2Fsbin",
                .f_redacted_uri          = "telnet://user:%2A%2A%2A@shell.example.org/packages?%2A=no-name&PATH=%2Fusr%2Fbin%3A%2Fusr%2Fsbin",
                .f_website_uri           = "telnet://shell.example.org/",
                .f_website_uri_with_port = "telnet://shell.example.org/",
                .f_domain                = "example",
                .f_full_domain           = "shell.example.org",
                .f_top_level_domain      = ".org",
                .f_sub_domains           = "shell",
                .f_username              = "user",
                .f_password              = "password1",
                .f_scheme                = "telnet",
                .f_path_encoded          = true,
                .f_path                  = "packages",
                .f_path_count            = 1,
                .f_port                  = 23,
                .f_is_port_secure        = false,
                .f_is_unix               = false,
                .f_query_string          = "%2A=no-name&PATH=/usr/bin:/usr/sbin",
                .f_anchor                = "",
            },
            {
                .f_original_uri          = "smtp://user:password1@mail.abc123.info///var/mail-boxes/user?PATH=/usr/bin%3A/usr/sbin#latest",
                .f_uri                   = "smtp://user:password1@mail.abc123.info/var/mail-boxes/user?PATH=%2Fusr%2Fbin%3A%2Fusr%2Fsbin#latest",
                .f_redacted_uri          = "smtp://user:%2A%2A%2A@mail.abc123.info/var/mail-boxes/user?PATH=%2Fusr%2Fbin%3A%2Fusr%2Fsbin#latest",
                .f_website_uri           = "smtp://mail.abc123.info/",
                .f_website_uri_with_port = "smtp://mail.abc123.info/",
                .f_domain                = "abc123",
                .f_full_domain           = "mail.abc123.info",
                .f_top_level_domain      = ".info",
                .f_sub_domains           = "mail",
                .f_username              = "user",
                .f_password              = "password1",
                .f_scheme                = "smtp",
                .f_path_encoded          = true,
                .f_path                  = "var/mail-boxes/user",
                .f_path_count            = 3,
                .f_port                  = 25,
                .f_is_port_secure        = false,
                .f_is_unix               = false,
                .f_query_string          = "PATH=/usr/bin:/usr/sbin",
                .f_anchor                = "latest",
            },
            {
                .f_original_uri          = "gopher://gofer:yes%3Ano@gopher.wall.alexis.museum:///gopher",
                .f_uri                   = "gopher://gofer:yes%3Ano@gopher.wall.alexis.museum/gopher",
                .f_redacted_uri          = "gopher://gofer:%2A%2A%2A@gopher.wall.alexis.museum/gopher",
                .f_website_uri           = "gopher://gopher.wall.alexis.museum/",
                .f_website_uri_with_port = "gopher://gopher.wall.alexis.museum/",
                .f_domain                = "alexis",
                .f_full_domain           = "gopher.wall.alexis.museum",
                .f_top_level_domain      = ".museum",
                .f_sub_domains           = "gopher.wall",
                .f_username              = "gofer",
                .f_password              = "yes:no",
                .f_scheme                = "gopher",
                .f_path_encoded          = true,
                .f_path                  = "gopher",
                .f_path_count            = 1,
                .f_port                  = 70,
                .f_is_port_secure        = false,
                .f_is_unix               = false,
                .f_query_string          = "",
                .f_anchor                = "",
            },
            {
                .f_original_uri          = "time://realtime.atomic.cl:/utc?leap-seconds=separate",
                .f_uri                   = "time://realtime.atomic.cl/utc?leap-seconds=separate",
                .f_redacted_uri          = "time://realtime.atomic.cl/utc?leap-seconds=separate",
                .f_website_uri           = "time://realtime.atomic.cl/",
                .f_website_uri_with_port = "time://realtime.atomic.cl/",
                .f_domain                = "atomic",
                .f_full_domain           = "realtime.atomic.cl",
                .f_top_level_domain      = ".cl",
                .f_sub_domains           = "realtime",
                .f_username              = "",
                .f_password              = "",
                .f_scheme                = "time",
                .f_path_encoded          = true,
                .f_path                  = "utc",
                .f_path_count            = 1,
                .f_port                  = 37,
                .f_is_port_secure        = false,
                .f_is_unix               = false,
                .f_query_string          = "leap-second=separate",
                .f_anchor                = "",
            },
            {
                .f_original_uri          = "snapwebsites://@parlement.co.uk/////folder/electric/bill?line%5B3%5D=129.07#quantity",
                .f_uri                   = "snapwebsites://parlement.co.uk:80/folder/electric/bill?line%5B3%5D=129.07#quantity",
                .f_redacted_uri          = "snapwebsites://parlement.co.uk:80/folder/electric/bill?line%5B3%5D=129.07#quantity",
                .f_website_uri           = "snapwebsites://parlement.co.uk/",
                .f_website_uri_with_port = "snapwebsites://parlement.co.uk:80/",
                .f_domain                = "parlement",
                .f_full_domain           = "parlement.co.uk",
                .f_top_level_domain      = ".co.uk",
                .f_sub_domains           = "",
                .f_username              = "",
                .f_password              = "",
                .f_scheme                = "snapwebsites",
                .f_path_encoded          = true,
                .f_path                  = "folder/electric/bill",
                .f_path_count            = 3,
                .f_port                  = 80,
                .f_is_port_secure        = false,
                .f_is_unix               = false,
                .f_query_string          = "line[3]=129.07",
                .f_anchor                = "quantity",
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
std::cerr << "--- working on [" << t.f_original_uri << "]\n";
            edhttp::uri u(t.f_original_uri);

            // sub-domains
            //
            std::list<std::string> sub_domains;
            snapdev::tokenize_string(sub_domains, t.f_sub_domains, {"."}, true);

            // verify each value for each original
            //
            CATCH_REQUIRE(u.get_original_uri() == t.f_original_uri);
            CATCH_REQUIRE(u.get_uri() == t.f_uri);
            CATCH_REQUIRE(u.get_uri("***") == t.f_redacted_uri);
            CATCH_REQUIRE(u.get_website_uri() == t.f_website_uri);
            CATCH_REQUIRE(u.get_website_uri(true) == t.f_website_uri_with_port);
            CATCH_REQUIRE(u.domain() == t.f_domain);
            CATCH_REQUIRE(u.sub_domains() == t.f_sub_domains);
            CATCH_REQUIRE(u.sub_domain_count() == static_cast<int>(sub_domains.size()));
            CATCH_REQUIRE(u.full_domain() == t.f_full_domain);
            CATCH_REQUIRE(u.top_level_domain() == t.f_top_level_domain);
            CATCH_REQUIRE(u.get_username() == t.f_username);
            CATCH_REQUIRE(u.get_password() == t.f_password);
            CATCH_REQUIRE(u.scheme() == t.f_scheme);
            CATCH_REQUIRE(u.path(t.f_path_encoded) == t.f_path);
            CATCH_REQUIRE(u.path_count() == t.f_path_count);
            CATCH_REQUIRE(u.get_port() == t.f_port);
            CATCH_REQUIRE(u.is_port_secure() == t.f_is_port_secure);
            CATCH_REQUIRE(u.is_unix() == t.f_is_unix);
            CATCH_REQUIRE(u.get_str_port() == std::to_string(t.f_port));
            CATCH_REQUIRE(u.anchor() == t.f_anchor);

            advgetopt::string_list_t const & sub_domain_list(u.sub_domains_list());
            CATCH_REQUIRE(sub_domain_list.size() == sub_domains.size());
            auto sub_domain_it(sub_domain_list.begin());
            part = 0;
            for(auto const & s : sub_domains)
            {
                CATCH_REQUIRE(u.sub_domain(part) == s);
                CATCH_REQUIRE(*sub_domain_it == s);
                CATCH_REQUIRE(u.get_part("sub-domain", part) == s);
                ++sub_domain_it;
                ++part;
            }

            // path segments
            //
            std::list<std::string> segments;
            snapdev::tokenize_string(segments, t.f_path, {"/"}, true);
            advgetopt::string_list_t const & path_list(u.path_list());
            CATCH_REQUIRE(path_list.size() == segments.size());
            auto path_it(path_list.begin());
            part = 0;
            for(auto const & s : segments)
            {
                if(t.f_path_encoded)
                {
                    CATCH_REQUIRE(edhttp::urlencode(u.path_segment(part)) == s);
                    CATCH_REQUIRE(edhttp::urlencode(*path_it) == s);
                    CATCH_REQUIRE(edhttp::urlencode(u.get_part("path", part)) == s);
                }
                else
                {
                    CATCH_REQUIRE(u.path_segment(part) == s);
                    CATCH_REQUIRE(*path_it == s);
                    CATCH_REQUIRE(u.get_part("path", part) == s);
                }
                ++path_it;
                ++part;
            }

            // try again with the get_part() function
            //
            CATCH_REQUIRE(u.get_part("anchor") == t.f_anchor);
            CATCH_REQUIRE(u.get_part("domain") == t.f_domain);
            CATCH_REQUIRE(u.get_part("full-domain") == t.f_full_domain);
            CATCH_REQUIRE(u.get_part("is-unix") == (t.f_is_unix ? "unix" : "inet"));
            CATCH_REQUIRE(u.get_part("original") == t.f_original_uri);
            CATCH_REQUIRE(u.get_part("password") == t.f_password);
            CATCH_REQUIRE(u.get_part("path-count") == std::to_string(t.f_path_count));
            CATCH_REQUIRE(u.get_part("port") == std::to_string(t.f_port));
            CATCH_REQUIRE(u.get_part("scheme") == t.f_scheme);
            CATCH_REQUIRE(u.get_part("tld") == t.f_top_level_domain);
            CATCH_REQUIRE(u.get_part("top-level-domain") == t.f_top_level_domain);
            CATCH_REQUIRE(u.get_part("uri") == t.f_uri);
            CATCH_REQUIRE(u.get_part("username") == t.f_username);

            // unknown parts are accepted and the function returns an empty string
            //
            CATCH_REQUIRE(u.get_part("anything-else-is-empty") == std::string());
        }
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("uri: use set_uri() to test invalid URIs")
    {
        struct uri_test
        {
            char const *    f_uri = nullptr;                    // some invalid URI
            char const *    f_error_message = nullptr;          // the expected error message
            bool            f_accept_path = false;
            bool            f_accept_ip = false;
        };

        uri_test uris[] =
        {
            {
                .f_uri           = "https",
                .f_error_message = "scheme not followed by \"://\".",
                .f_accept_path   = false,
                .f_accept_ip     = false,
            },
            {
                .f_uri           = "https://@m2osw.com:80:80/",
                .f_error_message = "more than one ':' in the domain name segment (after the '@') [1].",
                .f_accept_path   = false,
                .f_accept_ip     = false,
            },
            {
                .f_uri           = "https:///this.domain.net///with///a///path?var=value&other_var=more%20data#hello",
                .f_error_message = "a domain name is required.",
                .f_accept_path   = false,
                .f_accept_ip     = false,
            },
            {
                .f_uri           = "https://top:secret:password@m2osw.com:80:80/",
                .f_error_message = "more than one ':' in the login info segment (before the '@').",
                .f_accept_path   = false,
                .f_accept_ip     = false,
            },
            {
                .f_uri           = "https://top:secret@password@m2osw.com:80:80/",
                .f_error_message = "more than one '@' character found.",
                .f_accept_path   = false,
                .f_accept_ip     = false,
            },
            {
                .f_uri           = "https://my:password@m2osw.com:80:80/",
                .f_error_message = "more than one ':' in the domain name segment (after the '@') [2].",
                .f_accept_path   = false,
                .f_accept_ip     = false,
            },
            //{ -- as per the RFC, this is valid
            //    .f_uri           = "https://empty:port@m2osw.com:/",
            //    .f_error_message = "port cannot be an empty string.",
            //    .f_accept_path   = false,
            //    .f_accept_ip     = false,
            //},
            {
                .f_uri           = "https://empty:port@m2osw.com:http/",
                .f_error_message = "port must be a valid decimal number ('http' was unexpected).",
                .f_accept_path   = false,
                .f_accept_ip     = false,
            },
            {
                .f_uri           = "https://big:port@m2osw.com:65536/",
                .f_error_message = "port must be between 0 and 65536.",
                .f_accept_path   = false,
                .f_accept_ip     = false,
            },
            {
                .f_uri           = "https://no:domain@:65535/no/domain",
                .f_error_message = "a domain name is required.",
                .f_accept_path   = false,
                .f_accept_ip     = false,
            },
            {
                .f_uri           = "https://empty:@password.m2osw.com:1001/",
                .f_error_message = "username and password must both be defined (or define neither).",
                .f_accept_path   = false,
                .f_accept_ip     = false,
            },
            {
                .f_uri           = "https://:empty@password.m2osw.com:1001/",
                .f_error_message = "username and password must both be defined (or define neither).",
                .f_accept_path   = false,
                .f_accept_ip     = false,
            },
            {
                .f_uri           = "https://utc.m2osw.clock/",
                .f_error_message = "could not verify domain name \"utc.m2osw.clock\".",
                .f_accept_path   = false,
                .f_accept_ip     = false,
            },
            {
                .f_uri           = "https://utc.m2osw.co/?a=1&a=3",
                .f_error_message = "query string \"a\" found more than once.",
                .f_accept_path   = false,
                .f_accept_ip     = false,
            },
            {
                .f_uri           = "https://parent.m2osw.co/..",
                .f_error_message = "found \"..\" at the beginning of your path.",
                .f_accept_path   = false,
                .f_accept_ip     = false,
            },
            {
                .f_uri           = "https://parent.m2osw.co/../none",
                .f_error_message = "found \"..\" at the beginning of your path.",
                .f_accept_path   = false,
                .f_accept_ip     = false,
            },
            {
                .f_uri           = "https://parent.m2osw.co/./../sub-domain",
                .f_error_message = "found \"..\" at the beginning of your path.",
                .f_accept_path   = false,
                .f_accept_ip     = false,
            },
            {
                .f_uri           = "https://127.300.300.1/invalid/ipv4",
                .f_error_message = "could not parse \"127.300.300.1\" as a domain name or an IP address.",
                .f_accept_path   = false,
                .f_accept_ip     = true,
            },
            { // ranges are not allowed, same error as above
                .f_uri           = "https://127.0.0.1-127.0.0.255/invalid/ipv4",
                .f_error_message = "could not parse \"127.0.0.1-127.0.0.255\" as a domain name or an IP address.",
                .f_accept_path   = false,
                .f_accept_ip     = true,
            },
            { // ranges are not allowed, same error as above
                .f_uri           = "https://-127.0.0.1/invalid/ipv4",
                .f_error_message = "could not parse \"-127.0.0.1\" as a domain name or an IP address.",
                .f_accept_path   = false,
                .f_accept_ip     = true,
            },
            {
                .f_uri           = "https://[::z]/invalid/ipv4",
                .f_error_message = "an IPv6 must only include colons and hexadecimal digits.",
                .f_accept_path   = false,
                .f_accept_ip     = true,
            },
        };

        for(auto t : uris)
        {
            edhttp::uri u;

std::cerr << "--- testing URI [" << t.f_uri << "]\n";
            CATCH_REQUIRE_FALSE(u.set_uri(t.f_uri, t.f_accept_path, t.f_accept_ip));
            CATCH_REQUIRE(u.get_last_error_message() == t.f_error_message);
            u.clear_last_error_message();
            CATCH_REQUIRE(u.get_last_error_message().empty());
        }
    }
    CATCH_END_SECTION()
}


CATCH_TEST_CASE("uri_options", "[uri][options]")
{
    CATCH_START_SECTION("uri_options: verify options")
    {
        edhttp::uri u;

        CATCH_REQUIRE(u.option_count() == 0);
        CATCH_REQUIRE(u.option("unknown").empty());
        u.set_option("name", "value");
        CATCH_REQUIRE(u.option_count() == 1);
        CATCH_REQUIRE(u.option("name") == "value");
        u.set_option("name", "");
        CATCH_REQUIRE(u.option_count() == 0);

        edhttp::uri::uri_options_t const & list(u.options_list());
        CATCH_REQUIRE(list.find("name") == list.end());
        CATCH_REQUIRE(u.option("name") == "");
        u.set_option("name", ""); // a second time works too
        CATCH_REQUIRE(list.find("name") == list.end());
        CATCH_REQUIRE(u.option("name") == "");

        u.set_option("other", "special value");
        CATCH_REQUIRE(list.size() == 1);
        CATCH_REQUIRE(u.option_count() == 1);
        CATCH_REQUIRE(u.option("other") == "special value");
        u.unset_option("other");
        CATCH_REQUIRE(list.empty());
        CATCH_REQUIRE(u.option("other") == "");

        u.set_option("opt1", "val1");
        u.set_option("opt2", "val2");
        u.set_option("opt3", "val3");
        CATCH_REQUIRE(list.size() == 3);
        CATCH_REQUIRE(u.option_count() == 3);
        std::string name;
        CATCH_REQUIRE(u.option(0, name) == "val1");
        CATCH_REQUIRE(name == "opt1");
        CATCH_REQUIRE(u.option(1, name) == "val2");
        CATCH_REQUIRE(name == "opt2");
        CATCH_REQUIRE(u.option(2, name) == "val3");
        CATCH_REQUIRE(name == "opt3");
    }
    CATCH_END_SECTION()
}


CATCH_TEST_CASE("uri_encoding", "[uri][encode][decode]")
{
    CATCH_START_SECTION("uri_encoding: verify decoder")
    {
        struct decode_t
        {
            char const *        f_to_decode = nullptr;
            char const *        f_decoded = nullptr;
            bool                f_relax = false;
        };

        decode_t const to_decode[] =
        {
            {
                "+",
                " ",
                false,
            },
            {
                "%a5",
                "\xa5",
                false,
            },
            {
                "%B4",
                "\xb4",
                false,
            },
            {
                "%5F",
                "\x5f",
                false,
            },
            {
                "%4d",
                "\x4d",
                false,
            },
            {
                "%ZZ",
                "%ZZ",
                true,
            },
            {
                "%AZ",
                "%AZ",
                true,
            },
            {
                "%%%",
                "%%%",
                true,
            },
            {
                "ABCDEFGHIJKLMNOPQRSTUVWXYZ",
                "ABCDEFGHIJKLMNOPQRSTUVWXYZ",
                false,
            },
            {
                "abcdefghijklmnopqrstuvwxyz",
                "abcdefghijklmnopqrstuvwxyz",
                false,
            },
            {
                "0123456789",
                "0123456789",
                false,
            },
            {
                ".-/_",
                ".-/_",
                false,
            },
            {
                "~!@,;:()",
                "~!@,;:()",
                false,
            },
            { // in relax more, we really accept like whatever
                "`#$^&*={}[]\\|'\"<>?",
                "`#$^&*={}[]\\|'\"<>?",
                true,
            },
        };

        for(auto const & d : to_decode)
        {
            std::string const result(edhttp::urldecode(d.f_to_decode, d.f_relax));
            CATCH_REQUIRE(result == d.f_decoded);
        }
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("uri_encoding: verify encoder")
    {
        struct decode_t
        {
            char const *        f_to_encode = nullptr;
            char const *        f_encoded = nullptr;
            char const *        f_accepted = nullptr;
        };

        decode_t const to_encode[] =
        {
            {
                " or +",
                "%20or%20%2B",
                nullptr,
            },
            {
                "\xa5",
                "%A5",
                nullptr,
            },
            {
                "\xB4",
                "%B4",
                nullptr,
            },
            {
                "\x8F",
                "%8F",
                nullptr,
            },
            {
                "\x1d",
                "%1D",
                "",
            },
            {
                "%ZZ",
                "%25ZZ",
                "",
            },
            {
                "%AZ",
                "%25AZ",
                "",
            },
            {
                "%%%",
                "%25%25%25",
                nullptr,
            },
            {
                "ABCDEFGHIJKLMNOPQRSTUVWXYZ",
                "ABCDEFGHIJKLMNOPQRSTUVWXYZ",
                nullptr,
            },
            {
                "abcdefghijklmnopqrstuvwxyz",
                "abcdefghijklmnopqrstuvwxyz",
                nullptr,
            },
            {
                "0123456789",
                "0123456789",
                "",
            },
            {
                ".-_",
                ".-_",
                "",
            },
            {
                "~!@,;:()",
                "~!@,;:()",
                "~!@,;:()",
            },
            {
                "`#$^&*={}[]\\|'\"<>?",
                "%60%23%24%5E%26%2A%3D%7B%7D%5B%5D%5C%7C%27%22%3C%3E%3F",
                "",
            },
        };

        for(auto const & d : to_encode)
        {
            std::string const result(edhttp::urlencode(d.f_to_encode, d.f_accepted));
            CATCH_REQUIRE(result == d.f_encoded);
        }
    }
    CATCH_END_SECTION()
}


CATCH_TEST_CASE("uri_scheme", "[uri][scheme]")
{
    CATCH_START_SECTION("uri_scheme: check known schemes that do not infer a secure connection")
    {
        char const * const non_secure_schemes[] =
        {
            "", // empty is considered insecure
            "amqp",
            "domain",
            "ftp",
            "ftp-data",
            "http",
            "imap",
            "pop3",
            "smtp",
            "uucp",
            "syslog",
            "telnet",
            "ws",
            "zero",
        };

        for(auto scheme : non_secure_schemes)
        {
            CATCH_REQUIRE_FALSE(edhttp::is_scheme_secure(scheme));
        }
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("uri_scheme: check known schemes that infer a secure connection")
    {
        char const * const secure_schemes[] =
        {
            "amqps",
            "domain-s",
            "ftps",
            "ftps-data",
            "https",
            "imaps",
            "ircs-u",
            "ldaps",
            "nntps",
            "pop3s",
            "rsync",
            "smtps",
            "snntp",
            "ssh",
            "ssmtp",
            "submissions",
            "suucp",
            "syslog-tls",
            "telnets",
            "urd",
            "wss",
        };

        for(auto scheme : secure_schemes)
        {
std::cerr << "--- testing \"" << scheme << "\".\n";
            CATCH_REQUIRE(edhttp::is_scheme_secure(scheme));
        }
    }
    CATCH_END_SECTION()
}


CATCH_TEST_CASE("uri_port", "[uri][port]")
{
    CATCH_START_SECTION("uri_port: create uri with an invalid string")
    {
        edhttp::uri u("http://www.example.com:8765");

        CATCH_REQUIRE(u.get_port() == 8765);

        for(int i(0); i < 65536; ++i)
        {
            u.set_port(i);
            CATCH_REQUIRE(u.get_port() == i);

            int const anti_port(i ^ 0xFFFF);
            u.set_port(anti_port);
            CATCH_REQUIRE(u.get_port() == anti_port);

            u.set_port(std::to_string(i));
            CATCH_REQUIRE(u.get_port() == i);
        }
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("uri_port: create uri with an invalid string")
    {
        // WARNING: for the test to work you must keep this list of ports
        //          sorted numerically
        //
        int secure_ports[] =
        {
            22, 443, 465, 563, 636, 853, 873, 989, 990, 992, 993, 995,
            4031, 5671, 6514, 6697,
        };

        std::size_t idx(0);
        for(int port(0); port <= 65535; ++port)
        {
            if(idx < std::size(secure_ports)
            && secure_ports[idx] == port)
            {
                CATCH_REQUIRE(edhttp::is_port_secure(port));
                ++idx;
            }
            else
            {
                CATCH_REQUIRE_FALSE(edhttp::is_port_secure(port));
            }
        }
    }
    CATCH_END_SECTION()
}


CATCH_TEST_CASE("uri_error", "[uri][domain][error]")
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

    CATCH_START_SECTION("uri_error: decode invalid %XX")
    {
        CATCH_REQUIRE_THROWS_MATCHES(
                  edhttp::urldecode("%ZZ")
                , edhttp::invalid_uri
                , Catch::Matchers::ExceptionMessage(
                          "edhttp_exception: urldecode(\"%ZZ\", false) failed because of an invalid %xx character (digits are 90 / 90)."));

        CATCH_REQUIRE_THROWS_MATCHES(
                  edhttp::urldecode("%AZ")
                , edhttp::invalid_uri
                , Catch::Matchers::ExceptionMessage(
                          "edhttp_exception: urldecode(\"%AZ\", false) failed because of an invalid %xx character (digits are 65 / 90)."));

        CATCH_REQUIRE_THROWS_MATCHES(
                  edhttp::urldecode("#")
                , edhttp::invalid_uri
                , Catch::Matchers::ExceptionMessage(
                          "edhttp_exception: urldecode(\"#\", false) failed because of an invalid character (35)."));
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("uri_error: decode invalid %XX")
    {
        edhttp::uri u("http://snapwebsites.org:8484/?a=123&b=999&c-is-valid-too");

        CATCH_REQUIRE(u.domain() == "snapwebsites");
        CATCH_REQUIRE(u.query_option_count() == 3);
        std::string name;
        CATCH_REQUIRE(u.query_option(0, name) == "123");
        CATCH_REQUIRE(name == "a");
        CATCH_REQUIRE(u.query_option(1, name) == "999");
        CATCH_REQUIRE(name == "b");
        CATCH_REQUIRE(u.query_option(2, name) == "");
        CATCH_REQUIRE(name == "c-is-valid-too");

        CATCH_REQUIRE_THROWS_MATCHES(
                  u.query_option(3, name)
                , edhttp::out_of_range
                , Catch::Matchers::ExceptionMessage(
                          "out_of_range: query-option 3 does not exist (valid inclusive range 0 to 2)."));
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("uri_error: invalid anchor with a '#' character")
    {
        edhttp::uri u;

        CATCH_REQUIRE_THROWS_MATCHES(
                  u.set_anchor("anchor/cannot/include/a/#/character")
                , edhttp::invalid_parameter
                , Catch::Matchers::ExceptionMessage(
                          "edhttp_exception: anchor string \"anchor/cannot/include/a/#/character\" cannot include a '#' character."));
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("uri_error: options part overflow")
    {
        edhttp::uri u;

        u.set_option("name", "value");
        u.set_option("other", "special value");
        u.set_option("opt1", "val1");
        u.set_option("opt2", "val2");
        u.set_option("opt3", "val3");

        CATCH_REQUIRE(u.option_count() == 5);
        std::string name;
        CATCH_REQUIRE(u.option(0, name) == "value");
        CATCH_REQUIRE(name == "name");
        CATCH_REQUIRE(u.option(1, name) == "val1");
        CATCH_REQUIRE(name == "opt1");
        CATCH_REQUIRE(u.option(2, name) == "val2");
        CATCH_REQUIRE(name == "opt2");
        CATCH_REQUIRE(u.option(3, name) == "val3");
        CATCH_REQUIRE(name == "opt3");
        CATCH_REQUIRE(u.option(4, name) == "special value");
        CATCH_REQUIRE(name == "other");

        CATCH_REQUIRE_THROWS_MATCHES(
                  u.option(5, name)
                , edhttp::out_of_range
                , Catch::Matchers::ExceptionMessage(
                      "out_of_range: no option 5 available (valid inclusive range 0 to 4)."));
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("uri_error: path part overflow")
    {
        edhttp::uri u("https://example.com/with/a/path");

        CATCH_REQUIRE(u.path() == "with/a/path");
        CATCH_REQUIRE(u.path_segment(0) == "with");
        CATCH_REQUIRE(u.path_segment(1) == "a");
        CATCH_REQUIRE(u.path_segment(2) == "path");

        CATCH_REQUIRE_THROWS_MATCHES(
                  u.path_segment(3)
                , edhttp::out_of_range
                , Catch::Matchers::ExceptionMessage(
                      "out_of_range: no path section 3 available (valid inclusive range 0 to 2)."));
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("uri_error: set path with \"..\" at the start")
    {
        edhttp::uri u("https://example.com/with/a/path");

        CATCH_REQUIRE(u.path_segment(0) == "with");
        CATCH_REQUIRE(u.path_segment(1) == "a");
        CATCH_REQUIRE(u.path_segment(2) == "path");

        CATCH_REQUIRE_THROWS_MATCHES(
                  u.set_path("/../replace/the/path")
                , edhttp::invalid_path
                , Catch::Matchers::ExceptionMessage(
                      "edhttp_exception: path \"/../replace/the/path\" is not valid (it includes too many \"..\")."));

        CATCH_REQUIRE_THROWS_MATCHES(
                  u.set_path("../../../../replace/the/path")
                , edhttp::invalid_path
                , Catch::Matchers::ExceptionMessage(
                      "edhttp_exception: path \"with/a/path/../../../../replace/the/path\" is not valid (it includes too many \"..\")."));

        u.set_path("../../../replace/the/path");
        CATCH_REQUIRE(u.path_segment(0) == "replace");
        CATCH_REQUIRE(u.path_segment(1) == "the");
        CATCH_REQUIRE(u.path_segment(2) == "path");
    }
    CATCH_END_SECTION()

    CATCH_START_SECTION("uri_error: set invalid port")
    {
        edhttp::uri u("http://www.example.com:8765");

        CATCH_REQUIRE_THROWS_MATCHES(
                  u.set_port(-1)
                , edhttp::invalid_parameter
                , Catch::Matchers::ExceptionMessage(
                      "edhttp_exception: port \"-1\" is out of range (1 to 65535)."));

        CATCH_REQUIRE_THROWS_MATCHES(
                  u.set_port(65536)
                , edhttp::invalid_parameter
                , Catch::Matchers::ExceptionMessage(
                      "edhttp_exception: port \"65536\" is out of range (1 to 65535)."));

        CATCH_REQUIRE_THROWS_MATCHES(
                  u.set_port("-1")
                , edhttp::invalid_parameter
                , Catch::Matchers::ExceptionMessage(
                      "edhttp_exception: \"-1\" is an invalid port number."));

        CATCH_REQUIRE_THROWS_MATCHES(
                  u.set_port("65536")
                , edhttp::invalid_parameter
                , Catch::Matchers::ExceptionMessage(
                      "edhttp_exception: \"65536\" is an invalid port number."));

    }
    CATCH_END_SECTION()
}


// vim: ts=4 sw=4 et
