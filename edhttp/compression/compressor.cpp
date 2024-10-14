// Copyright (c) 2013-2024  Made to Order Software Corp.  All Rights Reserved
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
#include    "edhttp/compression/compressor.h"

#include    "edhttp/exception.h"


// snaplogger
//
#include    <snaplogger/message.h>


// snapdev
//
#include    <snapdev/not_used.h>


// C++
//
#include    <cstring>
#include    <ranges>


// C
//
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wold-style-cast"
#include    <zlib.h>
#pragma GCC diagnostic pop


// last include
//
#include    <snapdev/poison.h>



namespace edhttp
{


namespace
{



typedef std::map<std::string, compressor *>   compressor_map_t;

// IMPORTANT NOTE:
// This list only makes use of bare pointers for many good reasons.
// (i.e. all compressors are defined statitcally, not allocated)
// Do not try to change it! Thank you.
compressor_map_t * g_compressors;



} // no name namespace


/** \brief Special compressor name to get the best compression available.
 *
 * Whenever we send a page on the Internet, we can compress it with zlib
 * (gzip, really). However, more and more, browsers are starting to support
 * other compressors. For example, Chrome supports "sdch" (a vcdiff
 * compressor) and FireFox is testing with lzma.
 *
 * Using the name "best" for the compressor will test with all available
 * compressions and return the smallest result whatever it is.
 */
char const * compressor::BEST_COMPRESSION = "best";


/** \brief Special compressor name returned in some cases.
 *
 * When trying to compress a buffer, there are several reasons why the
 * compression may "fail". When that happens the result is the same
 * as the input, meaning that the data is not going to be compressed
 * at all.
 *
 * You should always verify whether the compression worked by testing
 * the compressor_name variable on return.
 */
char const * compressor::NO_COMPRESSION = "none";


/** \brief Register the compressor.
 *
 * Whenever you implement a compressor, the constructor must call
 * this constructor with the name of the compressor. Remember that
 * the get_name() function is NOT ready from this constructor which
 * is why we require you to specify the name in the constructor.
 *
 * This function registers the compressor in the internal list of
 * compressors and then returns.
 *
 * \param[in] name  The name of the compressor.
 */
compressor::compressor(char const * name)
{
    if(g_compressors == nullptr)
    {
        g_compressors = new compressor_map_t;
    }
    (*g_compressors)[name] = this;
}


/** \brief Clean up the compressor.
 *
 * This function unregisters the compressor.
 *
 * \note
 * It is expected that compressors get destroyed on exit only as they are
 * expected to be implemented and defined statically. However, for test
 * purposes, it can be practical to add & remove test compressors.
 */
compressor::~compressor()
{
    for(auto it(g_compressors->begin()); it != g_compressors->end(); ++it)
    {
        if(it->second == this)
        {
            g_compressors->erase(it);
            break;
        }
    }

    if(g_compressors->empty())
    {
        delete g_compressors;
        g_compressors = nullptr;
    }
}


/** \brief Return a list of names of the available compressors.
 *
 * In case you have more than one `Accept-Encoding` this list may end up being
 * helpful to know whether a compression is available or not.
 *
 * \return A list of names of all the available compressors.
 */
advgetopt::string_list_t compressor_list()
{
    if(g_compressors == nullptr)
    {
        return advgetopt::string_list_t();
    }

    auto kv = std::views::keys(*g_compressors);
    return advgetopt::string_list_t{ kv.begin(), kv.end() };
}


/** \brief Return a pointer to the named compressor.
 *
 * This function checks whether a compressor named \p compressor_name
 * exists, if so it gets returned, otherwise a null pointer is returned.
 *
 * \param[in] compressor_name  The name of the concerned compressor.
 *
 * \return A pointer to a compressor_t object or nullptr.
 */
compressor * get_compressor(std::string const & compressor_name)
{
    if(g_compressors != nullptr)
    {
        auto it(g_compressors->find(compressor_name));
        if(it != g_compressors->end())
        {
            return it->second;
        }
    }

    return nullptr;
}


/** \brief Compress the \p input buffer.
 *
 * This function compresses the \p input buffer using the specified
 * \p compressor_name_or_tag parameter and returns the result
 * in a copy.
 *
 * The \p compressor_name_or_tag can be set to:
 *
 * \li BEST_COMPRESSION
 *
 * In this case, all the available compressors get tested and the best
 * (i.e. smallest) result is returned. On exit, the
 * \p compressor_name_or_tag is updated with the compressor used.
 *
 * \li The name of a tag
 *
 * Similar to the BEST_COMPRESSION case, you can use a tag to test the
 * compressors that match that one tag. In a compressor, tags are written
 * in one string and each name is comma separated. That string also starts
 * and ends with a comma. This function detects that the name of a tag was
 * specificed if the \p compressor_name_or_tag parameter is set to a string
 * that starts & ends with a comma. Then the function goes through the whole
 * list of compressors and try to compress the \p input data with only the
 * ones that have that tag and the best (i.e. most compressed) result is
 * returned.
 *
 * \li Specific Compressor
 *
 * Finally, the \p compressor_name_or_tag can be set to a specific compressor
 * name in which case that specific compressor is used.
 *
 * \b IMPORTANT \b NOTE:
 *
 * There are several reasons why the compressor may refuse compressing
 * your \p input buffer and return said \p input as is. When this happens
 * the name of the compressor is changed to NO_COMPRESSION.
 *
 * \li The \p input is empty.
 * \li The \p input buffer is too small for that compressor.
 * \li The compression \p level is set to a value under 5%.
 * \li The buffer is way too large and allocating the compression buffer
 *     failed (this should never happen on a serious server!)
 * \li The named compressor does not exist.
 * \li The resulting compressed buffer is larger or equal to the size of
 *     the input buffer.
 *
 * Again, if the compression fails for whatever reason, \p compressor_name
 * is set to NO_COMPRESSION. You have to make sure to test that name on
 * return to know whether it worked or not.
 *
 * \todo
 * The function needs to allow for a selection of compressor names or tags
 * instead of either all of them or just one because we may have compressors
 * that are not compatible with, say HTTP, and yet we want to use this
 * function for the purpose of compressing data for that specific case.
 *
 * \exception missing_name
 * This exception is raised if the \p compression_name_or_tag parameter is
 * an empty string on entry.
 *
 * \param[in,out] compressor_name_or_tag  The name of the compressor to use
 * or a tag (i.e. ",http,"); on exit, the name of the compressor used to
 * compress the data or NO_COMPRESSION if not compressed.
 * \param[in] input  The input buffer which has to be compressed.
 * \param[in] level  The level of compression (0 to 100).
 * \param[in] text  Whether the input is text, set to false if not sure.
 *
 * \return A byte array with the compressed input data.
 */
buffer_t compress(std::string & compressor_name_or_tag, buffer_t const & input, level_t level, bool text)
{
    if(compressor_name_or_tag.empty())
    {
        throw missing_name("the first parameter to the compress() function cannot be an empty string.");
    }

    // clamp the level, just in case
    //
    level = std::clamp(level, static_cast<level_t>(0), static_cast<level_t>(100));

    // nothing to compress if empty or too small a level
    //
    if(input.size() == 0 || level < 5)
    {
#ifdef DEBUG
SNAP_LOG_TRACE
<< "nothing to compress"
<< SNAP_LOG_SEND;
#endif
        compressor_name_or_tag = compressor::NO_COMPRESSION;
        return input;
    }

    buffer_t result;

    // create a lambda of common code
    //
    auto select_best = [&result, &compressor_name_or_tag, &input, level, text](compressor * c)
    {
        if(result.empty())
        {
            result = c->compress(input, level, text);
            compressor_name_or_tag = c->get_name();
        }
        else
        {
            buffer_t test(c->compress(input, level, text));
            if(test.size() < result.size())
            {
                result.swap(test);
                compressor_name_or_tag = c->get_name();
            }
        }
    };

    if(compressor_name_or_tag == compressor::BEST_COMPRESSION)
    {
        for(auto const & c : *g_compressors)
        {
            select_best(c.second);
        }
    }
    else if(compressor_name_or_tag.front() == ','
         && compressor_name_or_tag.back() == ',')
    {
        if(compressor_name_or_tag.size() >= 3)
        {
            throw missing_name("the first parameter to the compress() function, when set to a tag, cannot just be two commas.");
        }
        if(compressor_name_or_tag.find(",", 1, compressor_name_or_tag.length() - 2))
        {
            throw too_many_names("the parameter to the compress() function, when set to a tag, cannot include more than two commas.");
        }

        char const * match_tag(compressor_name_or_tag.c_str());
        for(auto const & c : *g_compressors)
        {
            // make sure this compressor has the caller's specified tag
            // before calling select_best() against it
            //
            char const * tags(c.second->get_tags());
            if(tags != nullptr
            && std::strstr(tags, match_tag) != nullptr)
            {
                select_best(c.second);
            }
        }
    }
    else
    {
        auto it(g_compressors->find(compressor_name_or_tag));
        if(it == g_compressors->end())
        {
            // compressor is not available, return input as is...
            //
            compressor_name_or_tag = compressor::NO_COMPRESSION;
#ifdef DEBUG
SNAP_LOG_TRACE
<< "compressor \""
<< compressor_name_or_tag
<< "\" not found?!"
<< SNAP_LOG_SEND;
#endif
            return input;
        }

        result = it->second->compress(input, level, text);
    }

    // avoid the compression if the result is larger or equal to the input!
    //
    if(result.size() >= input.size())
    {
        compressor_name_or_tag = compressor::NO_COMPRESSION;
#ifdef DEBUG
SNAP_LOG_TRACE
<< "compression is larger in size?!"
<< SNAP_LOG_SEND;
#endif
        return input;
    }

    return result;
}


/** \brief Decompress a buffer.
 *
 * This function checks the specified input buffer and decompresses it if
 * a compressor recognized its magic signature.
 *
 * If none of the compressors were compatible then the input is returned
 * as is. The compressor_name is set to NO_COMPRESSION in this case. This
 * does not really mean the buffer is not compressed, although it is likely
 * correct.
 *
 * \param[out] compressor_name  Receives the name of the compressor used
 *                              to decompress the input data.
 * \param[in] input  The input to decompress.
 *
 * \return The decompressed buffer.
 */
buffer_t decompress(std::string & compressor_name, buffer_t const & input)
{
    // nothing to decompress if empty
    //
    if(!input.empty())
    {
        for(auto const & c : *g_compressors)
        {
            if(c.second->compatible(input))
            {
                compressor_name = c.second->get_name();
                return c.second->decompress(input);
            }
        }
    }

    compressor_name = compressor::NO_COMPRESSION;
    return input;
}



} // namespace edhttp
// vim: ts=4 sw=4 et
