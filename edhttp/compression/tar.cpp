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
#include    "edhttp/compression/archiver.h"

#include    "edhttp/exception.h"


// snaplogger
//
//#include    <snaplogger/message.h>


// snapdev
//
//#include <snapdev/not_used.h>


// C
//
//#pragma GCC diagnostic push
//#pragma GCC diagnostic ignored "-Wold-style-cast"
#include <tar.h>
//#pragma GCC diagnostic pop


// last include
//
#include <snapdev/poison.h>



namespace edhttp
{


//namespace
//{
//
//typedef QMap<QString, compressor_t *>   compressor_map_t;
//typedef QMap<QString, archiver_t *>   archiver_map_t;
//
//// IMPORTANT NOTE:
//// This list only makes use of bare pointers for many good reasons.
//// (i.e. all compressors are defined statitcally, not allocated)
//// Do not try to change it! Thank you.
//compressor_map_t * g_compressors;
//
//// IMPORTANT NOTE:
//// This list only makes use of bare pointers for many good reasons.
//// (i.e. all archivers are defined statitcally, not allocated)
//// Do not try to change it! Thank you.
//archiver_map_t * g_archivers;
//
//int bound_level(int level, int min, int max)
//{
//    return level < min ? min : (level > max ? max : level);
//}
//
//} // no name namespace



class tar
    : public archiver
{
public:
                            tar();

    virtual char const *    get_name() const override;
    virtual void            append_file(archiver_file const & file) override;
    virtual bool            next_file(archiver_file & file) const override;
    virtual void            rewind_file() override;

private:
    void                    append_int(char * header, int value, unsigned int length, int base, char fill);
    std::uint32_t           read_int(char const * header, int length, int base) const;
    std::uint32_t           check_sum(char const * s) const;

    mutable std::uint32_t   f_pos = 0;
};


tar::tar()
    : archiver("tar")
{
}

char const * tar::get_name() const
{
    return "tar";
}


/** \brief Create a tar header and copy the file content to the archive.
 *
 * This function creates a tar header (See definition in `/usr/include/tar.h`
 * for offset, length, and more).
 *
 * \param[in] file  The archiver file to be added to this tarball.
 */
void tar::append_file(archiver_file const & file)
{
    // initialize header
    //
    std::vector<char> header(512);
    header[257] = 'u'; // magic "ustar "
    header[258] = 's';
    header[259] = 't';
    header[260] = 'a';
    header[261] = 'r';
    header[262] = ' ';
    header[263] = ' '; // version " \0"
    //header[264] = '\0'; -- already '\0'

    // filename
    //
    std::string str(file.get_filename());
    if(str.size() > 100)
    {
        // TODO: add support for longer filenames
        //       (tar supports longer filename by using another block of data)
        //
        throw name_too_large("this file cannot be added to a tar archive at this point (filename too long)");
    }
    std::copy(str.data(), str.data() + str.size(), header.begin() + 0);

    // mode, uid, gid, mtime
    //
    append_int(&header[100], file.get_mode(), 7, 8, '0');
    append_int(&header[108], file.get_uid(),  7, 8, '0');
    append_int(&header[116], file.get_gid(),  7, 8, '0');

    // TODO: add support for the nanoseconds
    //
    append_int(&header[136], static_cast<int>(file.get_mtime().tv_sec), 11, 8, '0');

    // user/group names
    //
    str = file.get_user();
    if(str.length() > 32)
    {
        throw name_too_large("this file cannot be added to a tar archive at this point (user name too long)");
    }
    std::copy(str.data(), str.data() + str.size(), header.begin() + 265);

    str = file.get_group();
    if(str.length() > 32)
    {
        throw name_too_large("this file cannot be added to a tar archive at this point (group name too long)");
    }
    std::copy(str.data(), str.data() + str.size(), header.begin() + 297);

    // type, size
    //
    switch(file.get_type())
    {
    case file_type_t::FILE_TYPE_REGULAR:
        header[156] = REGTYPE; // regular (tar type)
        append_int(&header[124], file.get_data().size(), 11, 8, '0');
        break;

    case file_type_t::FILE_TYPE_DIRECTORY:
        header[156] = DIRTYPE; // directory (tar type)
        append_int(&header[124], 0, 11, 8, '0'); // size must be zero in ASCII
        break;

    //default: ... we could throw but here the compiler fails if we
    //             are missing some types
    }

    std::uint32_t checksum(check_sum(&header[0]));
    if(checksum > 32767)
    {
        // no null in this case (very rare if at all possible)
        //
        append_int(&header[148], checksum, 7, 8, '0');
    }
    else
    {
        append_int(&header[148], checksum, 6, 8, '0');
    }
    header[155] = ' ';

    f_archive.insert(f_archive.end(), header.begin(), header.end());

    switch(file.get_type())
    {
    case file_type_t::FILE_TYPE_REGULAR:
        f_archive.insert(f_archive.end(), file.get_data().begin(), file.get_data().end());
        {
            // padding to next 512 bytes
            //
            std::uint32_t size(file.get_data().size());
            size &= 511;
            if(size != 0)
            {
                std::vector<char> pad(512 - size);
                f_archive.insert(f_archive.end(), pad.begin(), pad.end());
            }
        }
        break;

    default:
        // no data for that type
        break;

    }
}


bool tar::next_file(archiver_file & file) const
{
    // any more files?
    // (make sure there is at least a header for now)
    //
    if(f_pos + 512 > f_archive.size())
    {
        return false;
    }

    // read the header
    std::vector<char> header(f_archive.data() + f_pos, f_archive.data() + f_pos + 512);

    // MAGIC
    if(header[257] != 'u' || header[258] != 's' || header[259] != 't' || header[260] != 'a'
    || header[261] != 'r' || (header[262] != ' ' && header[262] != '\0'))
    {
        // if no MAGIC we may have empty blocks (which are legal at the
        // end of the file)
        for(int i(0); i < 512; ++i)
        {
            if(header[i] != '\0')
            {
                throw incompatible(
                      "ustar magic code missing at position "
                    + std::to_string(f_pos));
            }
        }
        f_pos += 512;
        // TODO: test all the following blocks as they all should be null
        //       (as you cannot find an empty block within the tarball)
        return false;
    }

    std::uint32_t const file_checksum(read_int(&header[148], 8, 8));
    std::uint32_t const comp_checksum(check_sum(&header[0]));
    if(file_checksum != comp_checksum)
    {
        throw incompatible(
                "ustar checksum code does not match what was expected");
    }

    std::string filename(&header[0], strnlen(&header[0], 100));
    if(header[345] != '\0')
    {
        // this one has a prefix (long filename)
        //
        std::string const prefix(&header[345], strnlen(&header[345], 155));
        if(prefix.back() == '/')
        {
            // I think this case is considered a bug in a tarball...
            //
            filename = prefix + filename;
        }
        else
        {
            filename = prefix + '/' + filename;
        }
    }
    file.set_filename(filename);

    switch(header[156])
    {
    case AREGTYPE: // alternate code for regular file
    case REGTYPE:  // regular file
    case CONTTYPE: // continuous file
        file.set_type(file_type_t::FILE_TYPE_REGULAR);
        break;

    case DIRTYPE:
        file.set_type(file_type_t::FILE_TYPE_DIRECTORY);
        break;


    default:
        throw incompatible("file type in tarball not supported (we accept regular and directory files only)");

    }

    file.set_mode(read_int(&header[100],  8, 8));

    // TODO: see to add the nanonseconds (I think it's available?)
    //
    file.set_mtime(snapdev::timespec_ex(read_int(&header[136], 12, 8), 0));

    uid_t uid(read_int(&header[108],  8, 8));
    std::size_t const len_user(strnlen(&header[265], 32));
    file.set_user (std::string(&header[265], len_user), uid);

    gid_t gid(read_int(&header[116],  8, 8));
    std::size_t const len_group(strnlen(&header[265], 32));
    file.set_group(std::string(&header[297], len_group), gid);

    f_pos += 512;

    if(file.get_type() == file_type_t::FILE_TYPE_REGULAR)
    {
        uint32_t const size(read_int(&header[124], 12, 8));
        int const total_size((size + 511) & -512);
        if(f_pos + total_size > f_archive.size())
        {
            throw out_of_range("file data not available (archive too small)");
        }
        buffer_t data(f_archive.data() + f_pos, f_archive.data() + f_pos + size);
        file.set_data(data);

        f_pos += total_size;
    }

    return true;
}


void tar::rewind_file()
{
    f_pos = 0;
}


void tar::append_int(char * header, int value, unsigned int length, int base, char fill)
{
    // save the number (minimum 1 digit)
    //
    do
    {
        // base is 8 or 10
        //
        header[length] = static_cast<char>((value % base) + '0');
        value /= base;
        --length;
    }
    while(length > 0 && value != 0);

    // fill the left side with 'fill'
    //
    while(length > 0)
    {
        header[length] = fill;
        --length;
    }
}


std::uint32_t tar::read_int(char const * header, int length, int base) const
{
    std::uint32_t result(0);
    for(; length > 0 && *header != '\0' && *header != ' '; --length, ++header)
    {
        result = result * base + (*header - '0');
    }
    return result;
}


std::uint32_t tar::check_sum(char const * s) const
{
    std::uint32_t result = 8 * ' '; // the checksum field

    // name + mode + uid + gid + size + mtime = 148 bytes
    //
    for(int n(148); n > 0; --n, ++s)
    {
        result += *s;
    }

    s += 8; // skip the checksum field

    // everything after the checksum is another 356 bytes
    //
    for(int n(356); n > 0; --n, ++s)
    {
        result += *s;
    }

    return result;
}


tar g_tar; // declare statically



} // namespace edhttp
// vim: ts=4 sw=4 et
