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
#include    <edhttp/parametrized_value_list.h>

#include    <edhttp/exception.h>
#include    <edhttp/token.h>
#include    <edhttp/utils.h>


// libutf8
//
#include    <libutf8/libutf8.h>


// snaplogger
//
#include    <snaplogger/message.h>


// snapdev
//
#include    <snapdev/not_reached.h>
#include    <snapdev/not_used.h>
#include    <snapdev/safe_assert.h>
#include    <snapdev/trim_string.h>


// last include
//
#include    <snapdev/poison.h>



namespace edhttp
{
namespace
{



class parser_t
{
public:
    enum class token_t
    {
        TOK_EOF,
        TOK_ERROR,      // parser needs to skip up to next ','

        TOK_TOKEN,      // HTTP token
        TOK_VALUE,      // direct value
        TOK_QVALUE,     // double quoted string

        TOK_EQUAL = '=',
        TOK_COMMA = ',',
        TOK_SEMICOLON = ';',
    };

    enum class allow_value_t
    {
        ALLOW_VALUE_NO,
        ALLOW_VALUE_YES,
        ALLOW_VALUE_WITHOUT_EQUAL,
    };

    parser_t(
              std::string const & in
            , parametrized_value_mode_t mode
            , parametrized_value::vector_t & parts
            , std::string & error_messages)
        : f_in(in.c_str())
        , f_mode(mode)
        , f_parts(parts)
        , f_error_messages(error_messages)
    {
    }

    parser_t(parser_t const & rhs) = delete;
    parser_t & operator = (parser_t const & rhs) = delete;

    int getc()
    {
        if(f_unget != '\0')
        {
            int const c(f_unget);
            f_unget = '\0';
            return c;
        }

        if(*f_in == '\0')
        {
            return EOF;
        }

        return static_cast<unsigned char>(*f_in++);
    }

    void ungetc(int c)
    {
        if(c == EOF)
        {
            return;
        }

        if(f_unget != '\0')
        {
            throw logic_error("unget() called more than once."); // LCOV_EXCL_LINE
        }
        f_unget = c;
    }

    static bool is_end_of_string(int c)
    {
        return c == '"';
    }

    static bool is_end_of_cookie_string(int c)
    {
        return c == '"' || c == ';' || c == ',';
    }

    void next_token(allow_value_t allow_value = allow_value_t::ALLOW_VALUE_NO)
    {
        for(;;)
        {
            int c(getc());
            if(c == EOF)
            {
                f_token = token_t::TOK_EOF;
                return;
            }

            switch(c)
            {
            case ' ':
            case '\t':
                // skip those
                //
                break;

            case '=':
            case ',':
            case ';':
                f_token = token_t(c);
                return;

            case '"':
                // found a quoted value
                //
std::cerr << "--- found the double quotes but maybe we are missing the QUOTED STRING flag?\n";
                if((f_mode & PARAMETRIZED_VALUE_MODE_QUOTED_STRING) != 0)
                {
                    bool const keep_quotes((f_mode & PARAMETRIZED_VALUE_MODE_QUOTED_STRING_KEEP_QUOTES) != 0);
                    bool valid_string(true);
                    f_value.clear();
                    if(keep_quotes)
                    {
                        f_value += c;
                    }
                    auto found_end_of_string((f_mode & PARAMETRIZED_VALUE_MODE_QUOTED_STRING_COOKIE_SEMICOLON) == 0
                                        ? is_end_of_string
                                        : is_end_of_cookie_string);
                    for(c = getc(); c != EOF && !found_end_of_string(c); c = getc())
                    {
                        if(c == '\\'
                        && (f_mode & PARAMETRIZED_VALUE_MODE_QUOTED_STRING_ESCAPE_CHARACTER) != 0)
                        {
                            // any valid ASCII character can be escaped
                            //
                            c = getc();
                            if(c == EOF)
                            {
                                f_error_messages += "unexpected standalone backslash at the end of a quoted string.\n";
                                f_token = token_t::TOK_ERROR;
                                return;
                            }
                        }

                        // only accept VCHAR, SP, and HTAB
                        //
                        if((c < ' ' && c != '\t')
                        || c > '~')
                        {
                            if(valid_string)
                            {
                                f_error_messages += "invalid character in quoted string.\n";
                                valid_string = false;
                            }
                        }
                        else
                        {
                            f_value += c;
                        }
                    }
                    if(c != '"'
                    && (f_mode & PARAMETRIZED_VALUE_MODE_QUOTED_STRING_COOKIE_SEMICOLON) == 0)
                    {
                        f_error_messages += "closing quote missing.\n";
                        f_token = token_t::TOK_ERROR;
                        return;
                    }
                    if(!valid_string)
                    {
                        // quotes where good, but some characters are
                        // obs-text which we refuse so return an error
                        //
                        // DO NOT ADD YET ANOTHER ERROR TO f_error_messages
                        //
                        f_token = token_t::TOK_ERROR;
                        return;
                    }

                    // the value may end because of a ';' or ',' instead of '"'
                    //
                    if(keep_quotes && c == '"')
                    {
                        f_value += c;
                    }

                    f_token = token_t::TOK_QVALUE;
                    return;
                }
                [[fallthrough]]; // TBD, can the " be part of a value instead or is it just an error?
            default:
                f_value.clear();
                f_value.reserve(32);
                if(allow_value != allow_value_t::ALLOW_VALUE_NO)
                {
                    // in this case the caller allows a value
                    //
                    // i.e. The value of a parameter does not always
                    //      need quotation and if the entry itself can
                    //      be an immediate value.
                    //
                    bool valid_value(true);
                    do
                    {
                        // only accept VCHAR, SP, and HTAB
                        //
                        if((c < ' ' && c != '\t')
                        || c > '~')
                        {
                            if(valid_value)
                            {
                                f_error_messages += "invalid character in value.\n";
                                valid_value = false;
                            }
                        }
                        else
                        {
                            f_value += c;
                        }
                        c = getc();
                    }
                    while(c != EOF && c != ',' && c != ';'
                        && (allow_value == allow_value_t::ALLOW_VALUE_WITHOUT_EQUAL ? c != '=' : true));
                    ungetc(c);
                    f_value = snapdev::trim_string(f_value);
                    f_token = token_t::TOK_VALUE;
std::cerr << "--- value?! [" << f_value << "] (" << static_cast<int>(allow_value) << ")\n";
                    return;
                }
                else
                {
                    // default must be a valid token
                    //
                    do
                    {
                        f_value += c;
                        c = getc();
                    }
                    while(c != EOF
                       && c != ',' && c != ';'
                       && c != '='
                       && c != ' ' && c != '\t');
                    if(c != EOF
                    && c != ' '
                    && c != '\t')
                    {
                        ungetc(c);
                    }
std::cerr << "--- token [" << f_value << "] (" << static_cast<int>(f_mode) << ")\n";

                    if(!is_token(f_value))
                    {
                        f_error_messages += "a parameter name must be a valid HTTP token.\n";
                        f_token = token_t::TOK_ERROR;
                        return;
                    }
                    f_token = token_t::TOK_TOKEN;
                    return;
                }
                snapdev::NOT_REACHED();

            }
        }
    }

    bool get_part_bare_item(parametrized_value & part)
    {
        if((f_mode & PARAMETRIZED_VALUE_MODE_BARE_ITEM_VALUE_ONLY) != 0)
        {
            part.set_value(f_value);
            next_token();
            return true;
        }

        part.set_name(f_value);

        next_token();
        if(f_token == token_t::TOK_EQUAL)
        {
            if((f_mode & PARAMETRIZED_VALUE_MODE_BARE_ITEM_NAME_ONLY) != 0)
            {
                f_error_messages +=
                      "unexpected equal (=) after the bare item name.\n";
                return false;
            }

            // if a non-token value needs quotation then we need to pass false
            //
            next_token((f_mode & PARAMETRIZED_VALUE_MODE_QUOTE_NON_TOKEN_VALUE) == 0
                                    ? allow_value_t::ALLOW_VALUE_NO
                                    : allow_value_t::ALLOW_VALUE_YES);
            switch(f_token)
            {
            case token_t::TOK_TOKEN:
            case token_t::TOK_VALUE:
            case token_t::TOK_QVALUE:
                part.set_value(f_value);
                break;

            case token_t::TOK_ERROR:
                // next_token() already emitted an error
                return false;

            default:
                f_error_messages +=
                      "unexpected token ("
                    + std::to_string(static_cast<int>(f_token))
                    + ") after the = sign of the bare item.\n";
                return false;

            }
            next_token();
        }
        else if((f_mode & (PARAMETRIZED_VALUE_MODE_BARE_ITEM_NAME_ONLY | PARAMETRIZED_VALUE_MODE_BARE_ITEM_NAME_OPTIONAL_VALUE)) == 0)
        {
            f_error_messages +=
                  "the bare item name was expected to be followed by an equal sign (=) and a value.\n";
            return false;
        }

        return true;
    }

    bool get_part_parameters(parametrized_value & part)
    {
        for(;;)
        {
            //next_token();
            //if(f_token == token_t::TOK_EOF
            //|| f_token == token_t::TOK_COMMA)
            //{
            //    break;
            //}

            if(f_token != token_t::TOK_TOKEN)
            {
                f_error_messages += "unexpected token ("
                        + std::to_string(static_cast<int>(f_token))
                        + ") in an item list of parameters.\n";
                return false;
            }

            std::string const param_name(f_value);

            next_token();
            if(f_token != token_t::TOK_EQUAL)
            {
                if((f_mode & PARAMETRIZED_VALUE_MODE_PARAMETER_OPTIONAL_VALUE) == 0)
                {
                    f_error_messages += "item parameter \""
                                      + param_name
                                      + "\" must be assigned a value.\n";
                    return false;
                }
                f_value.clear();
            }
            else if((f_mode & PARAMETRIZED_VALUE_MODE_PARAMETER_NAME_ONLY) != 0)
            {
                f_error_messages += "item parameter \""
                                  + param_name
                                  + "\" cannot be assigned a value.\n";
                return false;
            }
            else
            {
                next_token((f_mode & PARAMETRIZED_VALUE_MODE_QUOTE_NON_TOKEN_VALUE) == 0
                                ? allow_value_t::ALLOW_VALUE_NO
                                : allow_value_t::ALLOW_VALUE_YES);
                switch(f_token)
                {
                case token_t::TOK_TOKEN:
                case token_t::TOK_VALUE:
                case token_t::TOK_QVALUE:
                    break;

                default:
                    f_error_messages += "unexpected token after the = sign to assign a parameter's value.\n";
                    return false;

                }
            }

            part.add_parameter(param_name, f_value);

            next_token();

            if(f_token == token_t::TOK_EOF
            || f_token == token_t::TOK_COMMA)
            {
                break;
            }

            if(f_token != token_t::TOK_SEMICOLON)
            {
                f_error_messages += "item's parameters must be separated by a semicolon.\n";
                return false;
            }

            next_token();

            if(f_token == token_t::TOK_EOF
            || f_token == token_t::TOK_COMMA)
            {
                break;
            }
        }

        return true;
    }

    bool parse_parameterized_item()
    {
        parametrized_value part;

        // we have a case where the first value is not different from any
        // of the other parameters; in that special case (as in the
        // Cache-Control) we want that first value to be read as a parameter
        //
        if((f_mode & PARAMETRIZED_VALUE_MODE_NO_BARE_ITEM) == 0)
        {
            if(!get_part_bare_item(part))
            {
                return false;
            }
            if(f_token != token_t::TOK_EOF)
            {
                if(f_token == token_t::TOK_SEMICOLON)
                {
                    next_token();
                    if(f_token != token_t::TOK_EOF
                    && f_token != token_t::TOK_COMMA)
                    {
                        if(!get_part_parameters(part))
                        {
                            return false;
                        }
                    }
                }
                else if(f_token != token_t::TOK_EOF
                     && f_token != token_t::TOK_COMMA)
                {
                    f_error_messages += "item not ended properly.\n";
                    return false;
                }
            }
        }
        else
        {
            // no bare item in this case so we need to directly jump to
            // the parameters
            //
            if(!get_part_parameters(part))
            {
                return false;
            }
        }

        f_parts.push_back(part);

        return true;
    }

    void parse()
    {
        bool valid(true);
        allow_value_t const value_only((f_mode & PARAMETRIZED_VALUE_MODE_BARE_ITEM_VALUE_ONLY) == 0
                                    ? allow_value_t::ALLOW_VALUE_NO
                                    : allow_value_t::ALLOW_VALUE_WITHOUT_EQUAL);
        for(;;)
        {
            next_token(value_only);
            switch(f_token)
            {
            case token_t::TOK_EOF:
                return;

            case token_t::TOK_TOKEN:
            case token_t::TOK_VALUE:
                if(!parse_parameterized_item())
                {
                    // there was an error, we need to skip everything
                    // up to EOF or the next comma
                    //
                    // Note: if a next_token() returns EOF, then calling
                    //       it again always returns EOF
                    //
                    do
                    {
                        next_token();
                    }
                    while(f_token != token_t::TOK_EOF
                       && f_token != token_t::TOK_COMMA);
                }
                break;

            case token_t::TOK_COMMA:
                // just skip empty items silently
                //
                break;

            default:
                // anything else is an error; but emit only one message
                //
                if(valid)
                {
                    valid = false;
                    f_error_messages += "unexpected token, skipping to next item.\n";
                }
                break;

            }
        }

        snapdev::NOT_REACHED();
    }

private:
    char const *            f_in = nullptr;
    parametrized_value_mode_t const
                            f_mode;
    int                     f_unget = '\0';
    bool                    f_immediate_value = false;
    parametrized_value::vector_t &
                            f_parts;
    std::string &           f_error_messages;

    token_t                 f_token = token_t::TOK_ERROR;
    std::string             f_value = std::string();
};


} // no name namespace


/** \brief Create a new weighted HTTP string object.
 *
 * The constructor is most often passed a language string to be parsed
 * immediately. The string can be empty, though.
 *
 * This function calls the parse() function on the input string.
 *
 * The parser is able to accept a "name = value" at the start or make
 * it directly a "value". This is useful to parse many more types
 * of entries.
 *
 * \warning
 * If the parsing generates, errors, the object is still created,
 * possible without any parameter or just a few. You can verify
 * whether it worked using `if(!error_messages().empty()) ...`.
 *
 * \param[in] str  The list of weighted HTTP strings.
 * \param[in] mode  Defines what is allowed where.
 */
parametrized_value_list::parametrized_value_list(
          std::string const & str
        , parametrized_value_mode_t mode)
    : f_mode(mode)
{
    snapdev::SAFE_ASSERT((f_mode & (PARAMETRIZED_VALUE_MODE_BARE_ITEM_NAME_ONLY | PARAMETRIZED_VALUE_MODE_BARE_ITEM_VALUE_ONLY)) != (PARAMETRIZED_VALUE_MODE_BARE_ITEM_NAME_ONLY | PARAMETRIZED_VALUE_MODE_BARE_ITEM_VALUE_ONLY), "One of the bare item name or value is required. You cannot forbid both.");

    snapdev::NOT_USED(parse(str));
}


/** \brief Parse a parametrized HTTP string.
 *
 * This function parses a parametrized HTTP string.
 *
 * The HTTP protocol supports this format for many headers.
 * The parser creates a list of name/value parts which are saves
 * in parametrized_value objects. The parametrized_value objects are
 * separated by commas. The parameters parts are separated by semi-colons.
 *
 * We view the `q=<float>` parameter as a special parameter which
 * is automatically converted in a weight. This allows for sorting
 * some of those lists.
 *
 * The supported string format is as follow:
 *
 * \code
 *      start: params
 *      params: options
 *            | params ',' options
 *      options: opt
 *             | options ';' opt
 *      opt: opt_name
 *         | opt_name '=' opt_value
 *      opt_name: token
 *      opt_value: token
 *               | quoted_string
 *      quoted_string: '"' *( CHAR ) '"'
 *      quoted_string: '"' *( quoted_text ) '"'
 *      quoted_text: [ \t!#-[\]-~]
 *                 | '\\' [ \t!-~]
 *      token: 1*tchar
 *      tchar: "!" / "#" / "$" / "%" / "&" / "'" / "*"
 *           / "+" / "-" / "." / "^" / "_" / "`" / "|" / "~"
 *           / DIGIT / ALPHA
 * \endcode
 *
 * The \p mode parameter defined how the parser really functions. For
 * example, for a cookie, the quoted string is just a mimic and a ';'
 * within that string is still viewed as the next parameter `name=value`.
 * Many of those do not view the first entry as anything special (such
 * as the Cache-Control header). Some strings are strict about their
 * parameters which must all have a value. The Accept-Language header
 * sees the first entry as a value, not a name=value, and that value
 * can include characters forbidden in an HTTP token. By tweaking the
 * mode, you can define how each part gets handled.
 *
 * Note that in most cases, further processing is required to handle
 * the specific header you are working with. For example, a language
 * range must be composed of at one to eight letters optionally followed
 * by one to eight letters or digits. This function will not verify that.
 *
 * From [RFC-9110](https://www.rfc-editor.org/rfc/rfc9110.html)
 * and [RFC-4647](https://www.rfc-editor.org/rfc/rfc4647.html)
 * here is an example for the "Accept-Language" header:
 *
 * \code
 *     Accept-Language  = [ ( language-range [ weight ] ) *( OWS "," OWS (
 *                           language-range [ weight ] ) ) ]
 *     language-range   = (1*8ALPHA *("-" 1*8alphanum)) / "*"
 *     alphanum         = ALPHA / DIGIT
 *     weight           = OWS ";" OWS "q=" qvalue
 *     qvalue           = ( "0" [ "." 0*3DIGIT ] )
 *                      / ( "1" [ "." 0*3("0") ] )
 *     OWS              = *( SP / HTAB )
 * \endcode
 *
 * For example, the following defines a few language strings
 * with their weights ("levels"):
 *
 * \code
 *      fr;q=0.8,en;q=0.5,de;q=0.1
 * \endcode
 *
 * This ends up being parsed as:
 *
 * \li fr, level 0.8
 * \li en, level 0.5
 * \li de, level 0.1
 *
 * Note that the input can be in any order. The vector is returned in the
 * order it was read (first is most important if no levels were specified).
 * If you want to sort by level, make sure to retrieve the vector with
 * get_parts() and then sort it with sort_by_level().
 *
 * Remember that by default a parametrized_value object uses the DEFAULT_LEVEL which
 * is 1.0. In other words, objects with no `q=...` parameter will likely
 * become first in the list.
 *
 * \code
 *     edhttp::parametrized_value_list language_country(locales);
 *     language_country.sort_by_level();
 * \endcode
 *
 * The sort is stable which is very important because if two strings have
 * the same level, then they have to stay in the order they were in the
 * header.
 *
 * See reference:
 * https://www.w3.org/Protocols/rfc2616/rfc2616-sec14.html#sec14.4
 *
 * \note
 * The function may return false if errors were detected. However, it
 * will keep whatever strings were properly parsed so far.
 * The errors can be retrieved using error_messages(). The functions
 * clears the string on each call.
 *
 * \todo
 * We may want to enhance the implementation to really limit all
 * the characters to what is clearly supported in HTTP/1.1 (Which
 * is the same in HTTP/2.) On the other hand, being "flexible" is not
 * always a bad thing as long as the use of data coming from a client
 * is properly checked for possibly tainted parameters (things that
 * could be doggy and as such need to be ignored).
 *
 * \param[in] str  A possibly weighted HTTP string to parse.
 * \param[in] reset  Reset the existing weighted HTTP strings if true.
 *                   If false, append the new data.
 *
 * \return true if no error were detected, false otherwise.
 */
bool parametrized_value_list::parse(
      std::string const & str
    , bool reset)
{
    f_error_messages.clear();

    if(f_original.empty() || reset)
    {
        f_parts.clear();
        f_original = str;
    }
    else
    {
        f_original.reserve(f_original.length() + str.length() + 1);
        f_original += ',';
        f_original += str;
    }

    parser_t parser(str, f_mode, f_parts, f_error_messages);
    parser.parse();

    if(!f_error_messages.empty())
    {
        // in case the caller "forgets" to print errors...
        //
        SNAP_LOG_ERROR
            << "parsing of \""
            << libutf8::to_visible_ascii(str)
            << "\" generated errors:\n"
            << f_error_messages
            << SNAP_LOG_SEND;
    }

    return f_error_messages.empty();
}


/** \brief Retrieve the level of the named parameter.
 *
 * This function searches for a part named \p name. If found, then its
 * level gets returned.
 *
 * A part with an unspecified level will have a level of DEFAULT_LEVEL
 * (which is 1.0f).
 *
 * If \p name is not found in the list of parts, this function returns
 * UNDEFINED_LEVEL (which is -1.0f).
 *
 * \todo
 * Determine whether we should limit to just the part value or name
 * with a parameter.
 *
 * \param[in] name  The name or value of the part for which the level is
 *                  requested.
 *
 * \return The part level or UNDEFINED_LEVEL.
 */
parametrized_value::level_t parametrized_value_list::get_level(std::string const & name)
{
    const int max_parts(f_parts.size());
    for(int i(0); i < max_parts; ++i)
    {
std::cerr << "--- part name [" << f_parts[i].get_name() << "/" << f_parts[i].get_value() << "] searching for [" << name << "]\n";

        if(f_parts[i].get_name() == name
        || f_parts[i].get_value() == name)
        {
            return f_parts[i].get_level();
        }
    }
    return parametrized_value::UNDEFINED_LEVEL();
}


/** \brief Use the weight (q=... values) to sort these HTTP strings.
 *
 * This function runs a stable sort against the weighted strings. This
 * is not called by default because some lists of strings are to
 * be kept sorted the way they are sent to us by the client.
 *
 * The function can be called multiple times, although, unless you
 * modify parts, there should be no need to do it more than once.
 */
void parametrized_value_list::sort_by_level()
{
    std::stable_sort(f_parts.begin(), f_parts.end());
}


parametrized_value * parametrized_value_list::get_part(std::string const & name) const
{
    const int max_parts(f_parts.size());
    for(int i(0); i < max_parts; ++i)
    {
        if(f_parts[i].get_name() == name)
        {
            return const_cast<parametrized_value *>(&f_parts[i]);
        }
    }
    return nullptr;
}


parametrized_value * parametrized_value_list::get_part_by_value(std::string const & name) const
{
    const int max_parts(f_parts.size());
    for(int i(0); i < max_parts; ++i)
    {
        if(f_parts[i].get_value() == name)
        {
            return const_cast<parametrized_value *>(&f_parts[i]);
        }
    }
    return nullptr;
}


/** \brief Convert all the parts to a full weighted HTTP string.
 *
 * This function converts all the parts of a weighted HTTP string
 * object to one string. The string representing each part is
 * generated using the parametrized_value::to_string() function.
 *
 * \return The string representing this weighted HTTP string.
 */
std::string parametrized_value_list::to_string() const
{
    std::string result;
    int const max_parts(f_parts.size());
    for(int i(0); i < max_parts; ++i)
    {
        if(!result.empty())
        {
            result += ", ";
        }
        result += f_parts[i].to_string();
    }
    return result;
} // LCOV_EXCL_LINE



} // namespace edhttp
// vim: ts=4 sw=4 et
