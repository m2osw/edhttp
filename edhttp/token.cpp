// Copyright (c) 2011-2022  Made to Order Software Corp.  All Rights Reserved
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
#include    "edhttp/token.h"

#include    "edhttp/exception.h"


// last include
//
#include    <snapdev/poison.h>



namespace edhttp
{


namespace
{

//     CHAR           = <any US-ASCII character (octets 0 - 127)>
//     token          = 1*<any CHAR except CTLs or separators>
//     separators     = "(" | ")" | "<" | ">" | "@"
//                    | "," | ";" | ":" | "\" | <">
//                    | "/" | "[" | "]" | "?" | "="
//                    | "{" | "}" | SP | HT
//
#define EDHTTP_TOKEN_CHAR(base, c) (static_cast<std::uint32_t>(1)<<(((c)-(base))&0x1F))
constexpr std::uint32_t const g_http_token[4] =
{
    /* 00-1F */ 0x00000000,
    /* 20-3F */
          EDHTTP_TOKEN_CHAR(0x20, '!')
        | EDHTTP_TOKEN_CHAR(0x20, '#')
        | EDHTTP_TOKEN_CHAR(0x20, '$')
        | EDHTTP_TOKEN_CHAR(0x20, '%')
        | EDHTTP_TOKEN_CHAR(0x20, '&')
        | EDHTTP_TOKEN_CHAR(0x20, '\'')
        | EDHTTP_TOKEN_CHAR(0x20, '*')
        | EDHTTP_TOKEN_CHAR(0x20, '+')
        | EDHTTP_TOKEN_CHAR(0x20, '-')
        | EDHTTP_TOKEN_CHAR(0x20, '.')
        | EDHTTP_TOKEN_CHAR(0x20, '0')
        | EDHTTP_TOKEN_CHAR(0x20, '1')
        | EDHTTP_TOKEN_CHAR(0x20, '2')
        | EDHTTP_TOKEN_CHAR(0x20, '3')
        | EDHTTP_TOKEN_CHAR(0x20, '4')
        | EDHTTP_TOKEN_CHAR(0x20, '5')
        | EDHTTP_TOKEN_CHAR(0x20, '6')
        | EDHTTP_TOKEN_CHAR(0x20, '7')
        | EDHTTP_TOKEN_CHAR(0x20, '8')
        | EDHTTP_TOKEN_CHAR(0x20, '9')
    ,
    /* 40-5F */
          EDHTTP_TOKEN_CHAR(0x40, 'A')
        | EDHTTP_TOKEN_CHAR(0x40, 'B')
        | EDHTTP_TOKEN_CHAR(0x40, 'C')
        | EDHTTP_TOKEN_CHAR(0x40, 'D')
        | EDHTTP_TOKEN_CHAR(0x40, 'E')
        | EDHTTP_TOKEN_CHAR(0x40, 'F')
        | EDHTTP_TOKEN_CHAR(0x40, 'G')
        | EDHTTP_TOKEN_CHAR(0x40, 'H')
        | EDHTTP_TOKEN_CHAR(0x40, 'I')
        | EDHTTP_TOKEN_CHAR(0x40, 'J')
        | EDHTTP_TOKEN_CHAR(0x40, 'K')
        | EDHTTP_TOKEN_CHAR(0x40, 'L')
        | EDHTTP_TOKEN_CHAR(0x40, 'M')
        | EDHTTP_TOKEN_CHAR(0x40, 'N')
        | EDHTTP_TOKEN_CHAR(0x40, 'O')
        | EDHTTP_TOKEN_CHAR(0x40, 'P')
        | EDHTTP_TOKEN_CHAR(0x40, 'Q')
        | EDHTTP_TOKEN_CHAR(0x40, 'R')
        | EDHTTP_TOKEN_CHAR(0x40, 'S')
        | EDHTTP_TOKEN_CHAR(0x40, 'T')
        | EDHTTP_TOKEN_CHAR(0x40, 'U')
        | EDHTTP_TOKEN_CHAR(0x40, 'V')
        | EDHTTP_TOKEN_CHAR(0x40, 'W')
        | EDHTTP_TOKEN_CHAR(0x40, 'X')
        | EDHTTP_TOKEN_CHAR(0x40, 'Y')
        | EDHTTP_TOKEN_CHAR(0x40, 'Z')
        | EDHTTP_TOKEN_CHAR(0x40, '^')
        | EDHTTP_TOKEN_CHAR(0x40, '_')
    ,
    /* 60-7F */
          EDHTTP_TOKEN_CHAR(0x60, '`')
        | EDHTTP_TOKEN_CHAR(0x60, 'a')
        | EDHTTP_TOKEN_CHAR(0x60, 'b')
        | EDHTTP_TOKEN_CHAR(0x60, 'c')
        | EDHTTP_TOKEN_CHAR(0x60, 'd')
        | EDHTTP_TOKEN_CHAR(0x60, 'e')
        | EDHTTP_TOKEN_CHAR(0x60, 'f')
        | EDHTTP_TOKEN_CHAR(0x60, 'g')
        | EDHTTP_TOKEN_CHAR(0x60, 'h')
        | EDHTTP_TOKEN_CHAR(0x60, 'i')
        | EDHTTP_TOKEN_CHAR(0x60, 'j')
        | EDHTTP_TOKEN_CHAR(0x60, 'k')
        | EDHTTP_TOKEN_CHAR(0x60, 'l')
        | EDHTTP_TOKEN_CHAR(0x60, 'm')
        | EDHTTP_TOKEN_CHAR(0x60, 'n')
        | EDHTTP_TOKEN_CHAR(0x60, 'o')
        | EDHTTP_TOKEN_CHAR(0x60, 'p')
        | EDHTTP_TOKEN_CHAR(0x60, 'q')
        | EDHTTP_TOKEN_CHAR(0x60, 'r')
        | EDHTTP_TOKEN_CHAR(0x60, 's')
        | EDHTTP_TOKEN_CHAR(0x60, 't')
        | EDHTTP_TOKEN_CHAR(0x60, 'u')
        | EDHTTP_TOKEN_CHAR(0x60, 'v')
        | EDHTTP_TOKEN_CHAR(0x60, 'w')
        | EDHTTP_TOKEN_CHAR(0x60, 'x')
        | EDHTTP_TOKEN_CHAR(0x60, 'y')
        | EDHTTP_TOKEN_CHAR(0x60, 'z')
        | EDHTTP_TOKEN_CHAR(0x60, '|')
        | EDHTTP_TOKEN_CHAR(0x60, '~')
};
#undef EDHTTP_TOKEN_CHAR


} // no name namespace



/** \brief Check whether \p token represents a valid token as per HTTP.
 *
 * HTTP headers make use of tokens for field names. These also appear
 * in some other places, for example the name of a user agent has to be
 * a valid token:
 *
 * \code
 *     User-Agent: <token>/<version> <comment>
 * \endcode
 *
 * \param[in] token  The string to verify.
 *
 * \return true if \p token is considered to be a valid token.
 */
bool is_token(std::string const & token)
{
    int const max_len(token.length());
    if(max_len == 0)
    {
        throw cookie_parse_exception("the name of a cookie cannot be empty");
    }

    if(token[0] == '$')
    {
        return false;
    }

    for(int i(0); i < max_len; ++i)
    {
        std::uint8_t const c(static_cast<std::uint8_t>(token[i]));
        if(c >= 127
        || (g_http_token[c >> 5] & (1 << (c & 0x1F))) == 0)
        {
            return false;
        }
    }

    return true;
}



} // namespace edhttp
// vim: ts=4 sw=4 et
