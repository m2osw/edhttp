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
#include    "edhttp/parametrized_value.h"

#include    "edhttp/exception.h"
#include    "edhttp/token.h"


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



/** \brief Create a named parametrized_value.
 *
 * This function is used to create a valid parametrized_value object.
 *
 * \param[in] name  The name of the parametrized_value object.
 *
 * \sa get_name()
 */
//parametrized_value::parametrized_value(std::string const & name)
//    : f_name(name)
//{
//}


/** \brief Retrieve the parametrized_value name.
 *
 * The name of a parametrized_value object cannot be changed once it was created.
 *
 * You may retrieve the name with this function, though.
 *
 * \bug
 * It is currently possible to create a parametrized_value object without a name
 * so the class works with QVector.
 *
 * \return The name as passed in when create the parametrized_value object.
 */
std::string const & parametrized_value::get_name() const
{
    return f_name;
}


/** \brief Set the name of the parameter.
 *
 * This function allows you to set the name of the parameter.
 *
 * \param[in] name  The name of this parameter.
 */
void parametrized_value::set_name(std::string const & name)
{
    f_name = name;
}


/** \brief Retrieve the value of this part.
 *
 * By default, a part is not expected to include a value, but there
 * are many strings in HTTP headers that accept a syntax where parameters
 * can be given a value. For example, in the Cache-Control field, we
 * can have a "max-age=123" parameter. This function returns the "123".
 * The name ("max-age") is returned by the get_name() function.
 *
 * In a weighted HTTP string such as a string of language definitions,
 * the named value has no value. It is expected to represent a flag
 * which is set (i.e. do not interpret a part with an empty string
 * as "false").
 *
 * \return The value of this part of the string.
 */
std::string const & parametrized_value::get_value() const
{
    return f_value;
}


/** \brief This function is used to setup the value of a part.
 *
 * This function defines the value of a part. By default a part is just
 * defined and its value is the empty string (it is still viewed as being
 * "true", but without anything more than that.)
 *
 * The function is called by the parser when it finds a part name followed
 * by an equal sign.
 *
 * \param[in] value  The new value of this part.
 */
void parametrized_value::set_value(std::string const & value)
{
    f_value = value;
}


/** \brief Retrieve the level of this parametrized_value object.
 *
 * This function retrieves the level of the parametrized_value object. It is a floating
 * point value.
 *
 * The level is taken from the "q" parameter. For example, in:
 *
 * \code
 *      fr; q=0.3
 * \endcode
 *
 * the level is viewed as 0.3.
 *
 * \return The parametrized_value object level.
 */
parametrized_value::level_t parametrized_value::get_level() const
{
    return f_level;
}


/** \brief Change the level of this part.
 *
 * This function saves the new \p level parameter in this parametrized_value object.
 * Items without a level (q=<value>) parameter are assigned the special
 * value DEFAULT_LEVEL, which is 1.0.
 *
 * \bug
 * The function does not limit the level. It is expected to be defined
 * between 0.0 and 1.0, though.
 *
 * \param[in] level  The new parametrized_value level.
 */
void parametrized_value::set_level(parametrized_value::level_t const level)
{
    f_level = level;
}


/** \brief Return true if the named parameter is defined.
 *
 * Note that in many cases a parameter is just a name without a value.
 * This function allows you to determine that a name was found and added
 * to the list of parameters. The get_parameter() would just return an
 * empty string whether the parameter is defined or not.
 *
 * \param[in] name  The name of the parameter to check.
 *
 * \return true if the named parameter is defined in this object.
 */
bool parametrized_value::has_parameter(std::string const & name) const
{
    return f_param.contains(name);
}


/** \brief Retrieve the value of a parameter.
 *
 * This function returns the value of a parameter given its name.
 *
 * If the parameter is not exist defined, then the function returns
 * an empty string. A parameter may exist and be set to the empty
 * string. There is no way to know at this point.
 *
 * \param[in] name  The name of the parameter to retrieve.
 *
 * \return The value of the parameter or "" if undefined.
 */
std::string parametrized_value::get_parameter(std::string const & name) const
{
    auto const it(f_param.find(name));
    if(it == f_param.end())
    {
        return std::string();
    }
    return it->second;
}


bool parametrized_value::get_parameter_as_integer(std::string const & name, std::int64_t & result) const
{
    auto const it(f_param.find(name));
    if(it == f_param.end()
    || it->second.empty())
    {
        return false;
    }

    errno = 0;
    char * end(nullptr);
    std::int64_t const v(strtol(it->second.c_str(), &end, 10));
    if((v == -1 && errno != 0)
    || (end != nullptr && *end != '\0'))
    {
        return false;
    }

    result = v;
    return true;
}


bool parametrized_value::get_parameter_as_floating_point(std::string const & name, double & result) const
{
    auto const it(f_param.find(name));
    if(it == f_param.end()
    || it->second.empty())
    {
        return false;
    }

    errno = 0;
    char * end(nullptr);
    double const v(strtod(it->second.c_str(), &end));
    if(( /*v == 0.0 &&*/ errno != 0)
    || (end != nullptr && *end != '\0'))
    {
        return false;
    }

    result = v;
    return true;
}


/** \brief Add a parameter.
 *
 * This function is used to add a parameter to the parametrized_value object.
 *
 * A parameter has a name and a value.
 *
 * \note
 * If the parameter is known for a specific value, then this function
 * parses the value and saves it converted. This currently applies to
 * the "q" parameter. If the value is considered invalid, it is just
 * ignored so for the "q" parameter, the level of the parametrized_value does
 * not change.
 *
 * \param[in] name  The name of the parameter to add.
 * \param[in] value  The value of the parameter.
 */
void parametrized_value::add_parameter(std::string const & name, std::string const & value)
{
    f_param[name] = value;

    // automatically handle parameters we understand
    //
    if(name == "q")
    {
        // qvalue: ( "0" [ "." *3DIGIT ] ) / ( "1" [ "." *3"0" ] )
        //
        if(value.length() >= 1
        && value.length() <= 5
        && (value.length() == 1 || value[1] == '.'))
        {
            double level(-1.0);
            if(advgetopt::validator_double::convert_string(value, level))
            {
                if(level >= 0.0 && level <= 1.0)
                {
                    f_level = level;
                }
            }
        }
    }
}


/** \brief Convert one part back into a weighted HTTP string.
 *
 * This function builds one part of a weighted HTTP string. The string
 * will look something like:
 *
 * \code
 *      es; q=0.8
 * \endcode
 *
 * \return The part converted to one string.
 */
std::string parametrized_value::to_string() const
{
    std::string result(f_name);
    if(!f_value.empty())
    {
        if(!result.empty())
        {
            result += '=';
        }

        if(is_token(f_value))
        {
            result += f_value;
        }
        else
        {
            result += qvalue(f_value);
        }
    }

    for(auto const & it : f_param)
    {
        result += "; ";
        result += it.first;
        if(!it.second.empty())
        {
            result += '=';

            if(is_token(it.second))
            {
                result += it.second;
            }
            else
            {
                result += qvalue(it.second);
            }
        }
    }

    return result;
} // LCOV_EXCL_LINE


/** \brief Operator used to sort elements.
 *
 * This operator overload is used by the different sort algorithms
 * that we can apply against this type. In most cases, it is a
 * std::stable_sort(),
 *
 * The function compares the level of the two parametrized_value objects involved.
 *
 * Note that we sort from the largest to the smallest level. In other
 * words, if this parametrized_value has level 1.0 and \p rhs has level 0.5, the
 * function returns true (i.e. 1.0 > 0.5).
 *
 * \param[in] rhs  The right hand side parametrized_value object to compare against.
 *
 * \return true if this parametrized_value is considered smaller than \p rhs.
 */
bool parametrized_value::operator < (parametrized_value const & rhs) const
{
    return f_level > rhs.f_level;
}


/** \brief Add backslashes for quoted strings.
 *
 * This function transforms a string to be added inside a quoted string
 * with backslashes to escape the quote (") and backslash (\) characters.
 *
 * \param[in] value  The string to be converted in a quoted value.
 *
 * \return The quoted value.
 */
std::string parametrized_value::qvalue(std::string const & value)
{
    std::string result;
    result.reserve(value.length() + 10);
    result += '"';
    for(auto const c : value)
    {
        switch(c)
        {
        case '"':
        case '\\':
            result += '\\';
            break;

        }
        result += c;
    }
    result += '"';

    return result;
} // LCOV_EXCL_LINE



} // namespace edhttp
// vim: ts=4 sw=4 et
