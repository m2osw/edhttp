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
#include    "edhttp/token.h"

#include    "edhttp/exception.h"


// C++
//
#include    <climits>
#include    <cstdint>


// last include
//
#include    <snapdev/poison.h>



namespace edhttp
{


namespace
{

/* Modern definition of a token
 *
 *     token          = 1*tchar
 *     tchar          = "!" / "#" / "$" / "%" / "&" / "'" / "*"
 *                    / "+" / "-" / "." / "^" / "_" / "`" / "|" / "~"
 *                    / DIGIT / ALPHA
 */
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


constexpr std::uint32_t const g_restricted_http_token[4] =
{
    /* 00-1F */ 0x00000000,
    /* 20-3F */
          EDHTTP_TOKEN_CHAR(0x20, '-')
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
        | EDHTTP_TOKEN_CHAR(0x40, '_')
    ,
    /* 60-7F */
          EDHTTP_TOKEN_CHAR(0x60, 'a')
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
};
#undef EDHTTP_TOKEN_CHAR


} // no name namespace




token::token(std::string const & s)
    : std::string(s)
{
}


token::token(std::string && s)
    : std::string(std::move(s))
{
}


std::strong_ordering token::operator <=> (token const & rhs)
{
    switch(compare_tokens(*this, rhs))
    {
    case 0:
        return std::strong_ordering::equal;

    case -1:
        return std::strong_ordering::less;

    case 1:
        return std::strong_ordering::greater;

    default:
        throw logic_error("unknown token order.");

    }
}


bool token::is_valid() const
{
    return is_token(*this);
}


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
    if(token.empty())
    {
        throw invalid_token("an HTTP token cannot be empty.");
    }

    // we can have a '$' inside the name, but not at the start
    //
    // TODO: try to find out why I added that restriction because it's
    //       definitely not in the specification
    //
    //if(token[0] == '$')
    //{
    //    return false;
    //}

    for(auto const t : token)
    {
        std::uint8_t const c(static_cast<std::uint8_t>(t));
        if(c >= 127
        || (g_http_token[c >> 5] & (1 << (c & 0x1F))) == 0)
        {
            return false;
        }
    }

    return true;
}


/** \brief Some systems do not allow all characters, this check for such.
 *
 * Various systems only allow alphanumeric, dashes and underscores in
 * header names. Also some systems convert the dashes into underscores
 * which means "User-Agent" and "User_Agent" are viewed as the same
 * header.
 *
 * \return true if \p token represents a valid restricted token.
 */
bool is_restricted_token(std::string const & token)
{
    if(token.empty())
    {
        throw invalid_token("an HTTP token cannot be empty.");
    }

    // TODO: the first character is further restricted in newer documentation
    //       (ALPHA / "*")
    //
    for(auto const t : token)
    {
        std::uint8_t const c(static_cast<std::uint8_t>(t));
        if(c >= 127
        || (g_restricted_http_token[c >> 5] & (1 << (c & 0x1F))) == 0)
        {
            return false;
        }
    }

    return true;
}


/** \brief Compare two tokens.
 *
 * This function compares two token together. If equal, the function
 * returns 0. Otherwise, the function returns -1 when lhs is before
 * rhs and +1 when after.
 *
 * The function is case insensitive ('A' == 'a' is true).
 *
 * The \p underscore_equal_dash parameter allows one to compare the
 * '-' character as equal to the '_' character.
 *
 * \exception invalid_token
 * If one of the tokens includes any invalid character (non-restricted) then
 * this exception is raised. WARNING: only the characters involved in the
 * comparison are verified. If one token is longer than the other, the
 * additional characters do not get checked and the exception does not
 * happen. If you want to make sure that a token is valid or not, call
 * one of the is_token() or is_restricted_token() functions.
 *
 * \param[in] lhs  Left hand side token.
 * \param[in] rhs  Right hand side token.
 *
 * \return -1, 0, 1 depending on whether \p lhs is before, equal, or after \p rhs.
 *
 * \sa is_token()
 * \sa is_restricted_token()
 */
int compare_tokens(std::string const & lhs, std::string const & rhs, bool underscore_equal_dash)
{
    auto to_lower = [underscore_equal_dash](char const c) -> int
    {
        if(c >= 127
        || (g_http_token[c >> 5] & (1 << (c & 0x1F))) == 0)
        {
            throw invalid_token("one of the input token includes an invalid character.");
        }

        return c == '_' && underscore_equal_dash
                ? '-'
                : (c >= 'A' && c <= 'Z'
                        ? c | 0x20
                        : c);
    };

    for(std::size_t idx(0); idx < lhs.length() && idx < rhs.length(); ++idx)
    {
        int const r(to_lower(lhs[idx]) - to_lower(rhs[idx]));
        if(r != 0)
        {
            // return -1 or 1
            //
            return (r >> (sizeof(r) * CHAR_BIT - 1)) | 1;
        }
    }

    int const l(lhs.length() - rhs.length());
    if(l != 0)
    {
        return (l >> (sizeof(l) * CHAR_BIT - 1)) | 1;
    }

    return 0;
}



} // namespace edhttp
// vim: ts=4 sw=4 et
