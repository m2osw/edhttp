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
#include    "edhttp/weighted_http_string.h"



// advgetopt
//
#include    <advgetopt/validator_double.h>


// snaplogger
//
#include    <snaplogger/message.h>


// snapdev
//
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
 * \param[in] str  The list of weighted HTTP strings.
 */
weighted_http_string::weighted_http_string(std::string const & str)
    //: f_str() -- auto-init
    //, f_parts() -- auto-init
{
    parse(str);
}


/** \brief Parse a weighted HTTP string.
 *
 * This function parses an "extended weighted HTTP string".
 *
 * By extended we means that we support more than just weights
 * so as to support lists of parameters like in the Cache-Control
 * field. The extensions are two folds:
 *
 * \li The first name can be a parameter with a value (a=b)
 * \li The value of a parameter can be a string of characters
 *
 * As a result, the supported string format is as follow:
 *
 * \code
 *      start: params
 *      params: options
 *            | params ',' options
 *      options: opt
 *             | options ';' opt
 *      opt: opt_name
 *         | opt_name '=' opt_value
 *      opt_name: CHAR - [,;=]
 *      opt_value: token
 *               | quoted_string
 *      token: CHAR - [,;]
 *      quoted_string: '"' CHAR '"'
 *                   | "'" CHAR "'"
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
 *
 * If you want to sort by level, make sure to retrieve the vector with
 * get_parts() and then sort it with sort_by_level().
 *
 * Remember that by default a string_part object uses the DEFAULT_LEVEL which
 * is 1.0. In other words, objects with no `q=...` parameter will likely
 * become first in the list.
 *
 * \code
 *      edhttp::weighted_http_string language_country(locales);
 *      language_country.sort_by_level();
 * \endcode
 *
 * The "stable" is very important because if two strings have the same
 * level, then they have to stay in the order they were in the input
 * string.
 *
 * See reference:
 * https://www.w3.org/Protocols/rfc2616/rfc2616-sec14.html#sec14.4
 *
 * \note
 * The function may return false if errors were detected. However, it
 * will keep whatever strings were loaded so far.
 *
 * \todo
 * We may want to ameliorate the implementation to really limit all
 * the characters to what is clearly supported in HTTP/1.1 (Which
 * is the same in HTTP/2.) On the other hand, being "flexible" is not
 * always a bad thing as long as the use of data coming from a client
 * is properly checked for possibly tainted parameters (things that
 * could be doggy and as such need to be ignored.)
 *
 * \param[in] str  A weight HTTP string to parse.
 * \param[in] reset  Reset the existing weighted HTTP strings if true.
 *
 * \return true if no error were detected, false otherwise.
 */
bool weighted_http_string::parse(std::string const & str, bool reset)
{
    f_error_messages.clear();

    int pos(0);
    if(f_str.empty() || reset)
    {
        f_parts.clear();
        f_str = str;
    }
    else
    {
        f_str += ',';
        pos = f_str.length();
        f_str += str;
    }

    char const * s(f_str.c_str() + pos);
    for(;;)
    {
        while(std::isspace(*s) || *s == ',')
        {
            ++s;
        }
        if(*s == '\0')
        {
            // reached the end of the string, we got a clean input
            //
            break;
        }
        char const * v(s);
        while(*s != '\0' && *s != ',' && *s != ';' && *s != '=' && *s != ' ' && *s != '\t')
        {
            ++s;
        }

        // Note: we check the length of the resulting name, the
        //       RFC 2616 definition is:
        //
        //          language-tag  = primary-tag *( "-" subtag )
        //          primary-tag   = 1*8ALPHA
        //          subtag        = 1*8ALPHA
        //
        //       so the maximum size is 8 + 1 + 8 = 17 (1 to 8 characters,
        //       the dash, 1 to 8 characters) and the smallest is 1.
        //
        std::string name(snapdev::trim_string(std::string(v, s - v), true, true, true));
        if(name.empty() || name.length() > 17)
        {
            // something is invalid, name is not defined (this can
            // happen if you just put a ';') or is too large
            //
            // XXX: should we signal the error in some way?
            //
            f_error_messages += "part name is empty or too long (limit is 17 characters.)\n";
            break;
        }
        // TODO: we want to check that `name` validity (i.e. 8ALPHA)
        //
        string_part part(name);

        // we allow spaces after the name and before the ';', '=', and ','
        //
        while(*s == ' ' || *s == '\t')
        {
            ++s;
        }

        // check whether that parameter has a value
        //
        if(*s == '=')
        {
            ++s;

            // allow spaces after an equal sign
            //
            while(*s == ' ' || *s == '\t')
            {
                ++s;
            }

            // values can be quoted
            //
            if(*s == '"' || *s == '\'')
            {
                auto const quote(*s);
                ++s;
                v = s;
                while(*s != '\0' && *s != quote)
                {
                    // accept any character within the quotes
                    // no backslash supported
                    //
                    ++s;
                }
                part.set_value(std::string(v, s - v));
                if(*s == quote)
                {
                    ++s;
                }

                // allow spaces after the closing quote
                //
                while(*s == ' ' || *s == '\t')
                {
                    ++s;
                }
            }
            else
            {
                v = s;
                while(*s != '\0' && *s != ';' && *s != ',')
                {
                    ++s;
                }
                part.set_value(snapdev::trim_string(std::string(v, s - v), true, true, true));
            }
        }

        // XXX: should we check whether another part with the same
        //      name already exists in the resulting vector?

        // read all the parameters, although we only keep
        // the 'q' parameter at this time
        //
        while(*s == ';')
        {
            // skip spaces and extra ';'
            //
            do
            {
                ++s;
            }
            while(*s == ';' || *s == ' ' || *s == '\t');

            // read parameter name
            //
            v = s;
            while(*s != '\0' && *s != ',' && *s != ';' && *s != '=')
            {
                ++s;
            }
            std::string const param_name(snapdev::trim_string(std::string(v, s - v), true, true));

            // TODO: we want to check that `param_name` validity (i.e. `token`)
            //       all the following separators are not considered legal
            //       and also controls (< 0x20) and most certainly characters
            //       over 0x7E
            //
            //        separators     = "(" | ")" | "<" | ">" | "@"
            //                       | "," | ";" | ":" | "\" | <">
            //                       | "/" | "[" | "]" | "?" | "="
            //                       | "{" | "}" | SP | HT
            // See:
            // https://www.w3.org/Protocols/rfc2616/rfc2616-sec2.html#sec2.2
            //
            if(!param_name.empty())
            {
                std::string param_value;
                if(*s == '=')
                {
                    ++s;
                    while(*s == ' ' || *s == '\t')
                    {
                        ++s;
                    }
                    if(*s == '\'' || *s == '"')
                    {
                        char const quote(*s);
                        ++s;
                        v = s;
                        while(*s != '\0' && *s != quote)
                        {
                            ++s;
                        }
                        param_value = snapdev::trim_string(std::string(v, s - v));
                        if(*s == quote)
                        {
                            ++s;
                        }

                        // allow spaces after the closing quote
                        //
                        while(*s == ' ' || *s == '\t')
                        {
                            ++s;
                        }
                    }
                    else
                    {
                        v = s;
                        while(*s != '\0' && *s != ',' && *s != ';')
                        {
                            ++s;
                        }
                        param_value = snapdev::trim_string(std::string(v, s - v), true, true, true);
                    }
                }
                part.add_parameter(param_name, param_value);

                // handle parameters we understand
                //
                if(param_name == "q")
                {
                    double level;
                    if(!advgetopt::validator_double::convert_string(param_value, level))
                    {
                        // the "quality" (q=...) parameter is not a valid
                        // floating point value
                        //
                        f_error_messages += "the quality value (q=...) is not a valid floating point.\n";
                    }
                    else if(level >= 0.0)
                    {
                        part.set_level(level);
                    }
                    else
                    {
                        // The "quality" (q=...) parameter cannot be
                        // a negative number
                        //
                        f_error_messages += "the quality value (q=...) cannot be a negative number.\n";
                    }
                }
                // TODO add support for other parameters, "charset" is one of
                //      them in the Accept header which we want to support
            }
            if(*s != '\0' && *s != ';' && *s != ',')
            {
                f_error_messages += "found a spurious character in a weighted string.\n";

                // ignore that entry...
                //
                ++s;
                while(*s != '\0' && *s != ',' && *s != ';')
                {
                    ++s;
                }
            }
        }

        f_parts.push_back(part);

        if(*s != ',' && *s != '\0')
        {
            f_error_messages += "part not ended by a comma or end of string.\n";
        }
    }

    if(!f_error_messages.empty())
    {
        // in case the caller "forgets" to print errors...
        //
        SNAP_LOG_ERROR
            << "parsing of \""
            << str
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
string_part::level_t weighted_http_string::get_level(std::string const & name)
{
    const int max_parts(f_parts.size());
    for(int i(0); i < max_parts; ++i)
    {
        if(f_parts[i].get_name() == name)
        {
            return f_parts[i].get_level();
        }
    }
    return string_part::UNDEFINED_LEVEL();
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
void weighted_http_string::sort_by_level()
{
    std::stable_sort(f_parts.begin(), f_parts.end());
}


/** \brief Convert all the parts to a full weighted HTTP string.
 *
 * This function converts all the parts of a weighted HTTP string
 * object to one string. The string representing each part is
 * generated using the string_part::to_string() function.
 *
 * \return The string representing this weighted HTTP string.
 */
std::string weighted_http_string::to_string() const
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
}



} // namespace edhttp
// vim: ts=4 sw=4 et
