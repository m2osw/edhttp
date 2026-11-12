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
#include    <snapdev/trim_string.h>


// last include
//
#include    <snapdev/poison.h>



namespace edhttp
{



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
 * \param[in] str  The list of weighted HTTP strings.
 * \param[in] immediate_value  Whether the first word is an immediate value.
 */
parametrized_value_list::parametrized_value_list(
          std::string const & str
        , bool immediate_value)
{
    snapdev::NOT_USED(parse(str, immediate_value));
}


/** \brief Parse a possibly weighted HTTP string.
 *
 * This function parses a possibly weighted HTTP string.
 *
 * The HTTP protocol supports this format for many fields.
 * The parser creates a list of name/value parts which is saves
 * in parametrized_value object. The parametrized_value objects are separated
 * by commas. The name/value parts are separated by semi-colons.
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
 *      token: 1*tchar
 *      tchar: "!" / "#" / "$" / "%" / "&" / "'" / "*"
 *           / "+" / "-" / "." / "^" / "_" / "`" / "|" / "~"
 *           / DIGIT / ALPHA
 * \endcode
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
 * \param[in] immediate_value  Whether the first word is an immediate value.
 * \param[in] reset  Reset the existing weighted HTTP strings if true.
 *                   If false, append the new data.
 *
 * \return true if no error were detected, false otherwise.
 */
bool parametrized_value_list::parse(
      std::string const & str
    , bool immediate_value
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

    class parser_t
    {
    public:
        enum class token_t
        {
            TOK_EOF,
            TOK_ERROR,      // parser needs to skip up to next ','

            TOK_TOKEN,      // HTTP token
            TOK_QVALUE,     // double quoted string

            TOK_EQUAL = '=',
            TOK_COMMA = ',',
            TOK_SEMICOLON = ';',
        };

        parser_t(
                  std::string const & in
                , bool immediate_value
                , parametrized_value::vector_t & parts
                , std::string & error_messages)
            : f_in(in.c_str())
            , f_immediate_value(immediate_value)
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
            if(f_unget != '\0')
            {
                throw logic_error("unget() called more than once."); // LCOV_EXCL_LINE
            }
            f_unget = c;
        }

        token_t get_token()
        {
            for(;;)
            {
                int c(getc());
                if(c == EOF)
                {
                    return token_t::TOK_EOF;
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
                    return token_t(c);

                case '"':
                    // found a quoted token
                    //
                    {
                        bool valid_string(true);
                        f_token.clear();
                        for(c = getc(); c != EOF && c != '"'; c = getc())
                        {
                            if(c == '\\')
                            {
                                // any valid ASCII character can be escaped
                                //
                                c = getc();
                                if(c == EOF)
                                {
                                    f_error_messages += "unexpected standalone backslash at the end of a quoted string.\n";
                                    return token_t::TOK_ERROR;
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
                                f_token += c;
                            }
                        }
                        if(c != '"')
                        {
                            f_error_messages += "closing quote missing.\n";
                            return token_t::TOK_ERROR;
                        }
                        if(!valid_string)
                        {
                            // quotes where good, but some characters are
                            // obs-text which we refuse so return an error
                            //
                            // DO NOT ADD YET ANOTHER ERROR TO f_error_messages
                            //
                            return token_t::TOK_ERROR;
                        }
                    }
                    return token_t::TOK_QVALUE;

                default:
                    // default must be a valid token
                    //
                    {
                        f_token.clear();
                        f_token.reserve(32);
                        do
                        {
                            f_token += c;
                            c = getc();
                        }
                        while(c != EOF
                           && c != ',' && c != ';'
                           && c != '='
                           && c != ' ' && c != '\t');
                        if(c != ' '
                        && c != '\t')
                        {
                            ungetc(c);
                        }

                        if(!is_token(f_token))
                        {
                            f_error_messages += "a parameter name must be a valid HTTP token.\n";
                            return token_t::TOK_ERROR;
                        }
                    }
                    return token_t::TOK_TOKEN;

                }
            }
        }

        bool parse_parameterized_item()
        {
            // one item is:
            //
            //     ; standard weighted strings
            //     token = <value> ; token [ = <value> ] ; ...
            //
            //     ; weighted string with immediate values
            //     <value> ; token [ = <value> ] ; ...
            //
            // this is converted in what we call a parametrized_value; the first
            // token is the name of the parametrized_value; and the following
            // ones are the parameters; the function is called with the
            // first token already in f_token
            //
            parametrized_value part(f_token);

            token_t token(get_token());
            if(!f_immediate_value
            && token == token_t::TOK_EQUAL)
            {
                token = get_token();
                switch(token)
                {
                case token_t::TOK_TOKEN:
                case token_t::TOK_QVALUE:
                    part.set_value(f_token);
                    break;

                case token_t::TOK_ERROR:
                    // get_token() already emitted an error
                    return false;

                default:
                    f_error_messages +=
                          "unexpected token ("
                        + std::to_string(static_cast<int>(token))
                        + ") after the = sign.\n";
                    return false;

                }
                token = get_token();
            }

            if(token == token_t::TOK_SEMICOLON)
            {
                for(;;)
                {
                    token = get_token();
                    if(token == token_t::TOK_EOF
                    || token == token_t::TOK_COMMA)
                    {
                        break;
                    }

                    if(token != token_t::TOK_TOKEN)
                    {
                        f_error_messages += "unexpected token in an item list of parameters.\n";
                        return false;
                    }

                    std::string param_name(f_token);

                    token = get_token();
                    if(token != token_t::TOK_EQUAL)
                    {
                        f_error_messages += "item parameter \""
                                          + param_name
                                          + "\" must be assigned a value.\n";
                        return false;
                    }

                    token = get_token();
                    switch(token)
                    {
                    case token_t::TOK_TOKEN:
                    case token_t::TOK_QVALUE:
                        break;

                    default:
                        f_error_messages += "unexpected token after the = sign to assign a parameter's value.\n";
                        return false;

                    }

                    part.add_parameter(param_name, f_token);

                    token = get_token();

                    if(token == token_t::TOK_EOF
                    || token == token_t::TOK_COMMA)
                    {
                        break;
                    }

                    if(token != token_t::TOK_SEMICOLON)
                    {
                        f_error_messages += "item's parameters must be separated by semicolon.\n";
                        return false;
                    }
                }
            }
            else
            {
                if(token != token_t::TOK_EOF
                && token != token_t::TOK_COMMA)
                {
                    f_error_messages += "item not ended properly.\n";
                    return false;
                }
            }


            f_parts.push_back(part);

            return true;
        }

        void parse()
        {
            bool valid(true);
            for(;;)
            {
                token_t token(get_token());
                switch(token)
                {
                case token_t::TOK_EOF:
                    return;

                case token_t::TOK_TOKEN:
                    if(!parse_parameterized_item())
                    {
                        // there was an error, we need to skip everything
                        // up to EOF or the next comma
                        //
                        // Note: if a get_token() returns EOF, then calling
                        //       it again always returns EOF
                        //
                        do
                        {
                            token = get_token();
                        }
                        while(token != token_t::TOK_EOF
                           && token != token_t::TOK_TOKEN);
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
        int                     f_unget = '\0';
        bool                    f_immediate_value = false;
        parametrized_value::vector_t & f_parts;
        std::string &           f_error_messages;

        std::string             f_token = std::string();
    };

    parser_t parser(str, immediate_value, f_parts, f_error_messages);
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
 * \param[in] name  The name of the part for which the level is requested.
 *
 * \return The part level or UNDEFINED_LEVEL.
 */
parametrized_value::level_t parametrized_value_list::get_level(std::string const & name)
{
    const int max_parts(f_parts.size());
    for(int i(0); i < max_parts; ++i)
    {
        if(f_parts[i].get_name() == name)
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
