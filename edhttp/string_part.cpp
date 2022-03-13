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
#include    "edhttp/string_part.h"



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



/** \brief The string_part constructor without parameters is for the vector.
 *
 * When initializing a vector, the class has to have a constructor with
 * no parameters. Unfortunate since we would prefer to not allow string_part
 * objects without a name which is considered mandatory (would have to
 * test again once we convert the class to only use the STL library.)
 */
string_part::string_part()
{
}


/** \brief Create a named string_part.
 *
 * This function is used to create a valid string_part object.
 *
 * \param[in] name  The name of the string_part object.
 *
 * \sa get_name()
 */
string_part::string_part(std::string const & name)
    : f_name(name)
{
}


/** \brief Retrieve the string_part name.
 *
 * The name of a string_part object cannot be changed once it was created.
 *
 * You may retrieve the name with this function, though.
 *
 * \bug
 * It is currently possible to create a string_part object without a name
 * so the class works with QVector.
 *
 * \return The name as passed in when create the string_part object.
 */
std::string const & string_part::get_name() const
{
    return f_name;
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
std::string const & string_part::get_value() const
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
void string_part::set_value(std::string const & value)
{
    f_value = value;
}


/** \brief Retrieve the level of this string_part object.
 *
 * This function retrieves the level of the string_part object. It is a floating
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
 * \return The string_part object level.
 */
string_part::level_t string_part::get_level() const
{
    return f_level;
}


/** \brief Change the level of this part.
 *
 * This function saves the new \p level parameter in this string_part object.
 * Items without a level (q=<value>) parameter are assigned the special
 * value DEFAULT_LEVEL, which is 1.0.
 *
 * \bug
 * The function does not limit the level. It is expected to be defined
 * between 0.0 and 1.0, though.
 *
 * \param[in] level  The new string_part level.
 */
void string_part::set_level(string_part::level_t const level)
{
    f_level = level;
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
std::string string_part::get_parameter(std::string const & name) const
{
    auto const it(f_param.find(name));
    if(it == f_param.end())
    {
        return std::string();
    }
    return it->second;
}


/** \brief Add a parameter.
 *
 * This function is used to add a parameter to the string_part object.
 *
 * A parameter has a name and a value.
 *
 * \param[in] name  The name of the parameter to add.
 * \param[in] value  The value of the parameter.
 */
void string_part::add_parameter(std::string const & name, std::string const & value)
{
    f_param[name] = value;
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
std::string string_part::to_string() const
{
    std::string result(f_name);

    for(auto const & it : f_param)
    {
        std::string p(it.first);
        if(!it.second.empty())
        {
            p += '=';
            p += it.second;
        }
        result += "; ";
        result += p;
    }

    return result;
}


/** \brief Operator used to sort elements.
 *
 * This operator overload is used by the different sort algorithms
 * that we can apply against this type. In most cases, it is a
 * std::stable_sort(),
 *
 * The function compares the level of the two string_part objects involved.
 *
 * Note that we sort from the largest to the smallest level. In other
 * words, if this string_part has level 1.0 and \p rhs has level 0.5, the
 * function returns true (i.e. 1.0 > 0.5).
 *
 * \param[in] rhs  The right hand side string_part object to compare against.
 *
 * \return true if this string_part is considered smaller than \p rhs.
 */
bool string_part::operator < (string_part const & rhs) const
{
    return f_level > rhs.f_level;
}



} // namespace edhttp
// vim: ts=4 sw=4 et
