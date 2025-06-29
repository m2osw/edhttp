// Copyright (c) 2024-2025  Made to Order Software Corp.  All Rights Reserved
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
 * \brief Tool used to show the list of edhttp compressors.
 *
 * This tool lists the edhttp compressors. If you have just edhttp installed,
 * then only the internally supported compressors will be listed.
 *
 * Other projects may add their own compressor/decompressor as required and
 * this tool will automatically detect and display their name.
 */

// edhttp
//
#include    "edhttp/compression/archiver.h"
#include    "edhttp/compression/compressor.h"
#include    "edhttp/version.h"


// advgetopt
//
#include    <advgetopt/advgetopt.h>
#include    <advgetopt/conf_file.h>
#include    <advgetopt/exception.h>
#include    <advgetopt/options.h>


// libexcept
//
#include    <libexcept/file_inheritance.h>
#include    <libexcept/report_signal.h>


// snapdev
//
#include    <snapdev/stringize.h>


// C++
//
#include    <iostream>


// last include
//
#include    <snapdev/poison.h>



namespace
{



const advgetopt::option g_options[] =
{
    advgetopt::define_option(
          advgetopt::Name("archivers")
        , advgetopt::ShortName('a')
        , advgetopt::Flags(advgetopt::standalone_command_flags<
              advgetopt::GETOPT_FLAG_GROUP_COMMANDS>())
        , advgetopt::Help("list the archivers.")
    ),
    advgetopt::define_option(
          advgetopt::Name("compressors")
        , advgetopt::ShortName('c')
        , advgetopt::Flags(advgetopt::standalone_command_flags<
              advgetopt::GETOPT_FLAG_GROUP_COMMANDS>())
        , advgetopt::Help("list the compressors (this is the default if nothing else is specified).")
    ),
    advgetopt::define_option(
          advgetopt::Name("headers")
        , advgetopt::ShortName('H')
        , advgetopt::Flags(advgetopt::standalone_command_flags<
              advgetopt::GETOPT_FLAG_GROUP_OPTIONS>())
        , advgetopt::Help("show headers before the list.")
    ),
    advgetopt::define_option(
          advgetopt::Name("verbose")
        , advgetopt::ShortName('v')
        , advgetopt::Flags(advgetopt::standalone_command_flags<
              advgetopt::GETOPT_FLAG_GROUP_OPTIONS>())
        , advgetopt::Help("show all the available data.")
    ),
    advgetopt::end_options()
};

advgetopt::group_description const g_group_descriptions[] =
{
    advgetopt::define_group(
          advgetopt::GroupNumber(advgetopt::GETOPT_FLAG_GROUP_COMMANDS)
        , advgetopt::GroupName("command")
        , advgetopt::GroupDescription("Commands:")
    ),
    advgetopt::define_group(
          advgetopt::GroupNumber(advgetopt::GETOPT_FLAG_GROUP_OPTIONS)
        , advgetopt::GroupName("option")
        , advgetopt::GroupDescription("Options:")
    ),
    advgetopt::end_groups()
};

constexpr char const * const g_configuration_files[] =
{
    "/etc/edhttp/edhttp-list-compressors.conf",
    nullptr
};

advgetopt::options_environment const g_options_environment =
{
    .f_project_name = "edhttp-list-compressors",
    .f_group_name = "edhttp",
    .f_options = g_options,
    .f_options_files_directory = nullptr,
    .f_environment_variable_name = "EDHTTP_LIST_COMPRESSORS",
    .f_environment_variable_intro = "EDHTTP_LIST_COMPRESSORS",
    .f_section_variables_name = nullptr,
    .f_configuration_files = g_configuration_files,
    .f_configuration_filename = nullptr,
    .f_configuration_directories = nullptr,
    .f_environment_flags = advgetopt::GETOPT_ENVIRONMENT_FLAG_PROCESS_SYSTEM_PARAMETERS,
    .f_help_header = "Usage: %p [-<opt>]\n"
                     "where -<opt> is one or more of:",
    .f_help_footer = "Try `man edhttp-list-compressors` for more info.\n%c",
    .f_version = EDHTTP_VERSION_STRING,
    .f_license = "GPL v3 or newer",
    .f_copyright = "Copyright (c) 2024-"
                   SNAPDEV_STRINGIZE(UTC_BUILD_YEAR)
                   "  Made to Order Software Corporation",
    .f_build_date = UTC_BUILD_DATE,
    .f_build_time = UTC_BUILD_TIME,
    .f_groups = g_group_descriptions
};






class edhttp_list_compressor
{
public:
                            edhttp_list_compressor(int argc, char * argv[]);

    int                     run();

private:
    int                     get_path();
    void                    list_compressors();
    void                    list_archivers();

    advgetopt::getopt       f_opt;
    bool                    f_verbose = false;
    bool                    f_headers = false;
};


edhttp_list_compressor::edhttp_list_compressor(int argc, char * argv[])
    : f_opt(g_options_environment, argc, argv)
{
}


int edhttp_list_compressor::run()
{
    if(f_opt.is_defined("verbose"))
    {
        f_verbose = true;
    }

    bool compressors(false);
    bool archivers(false);

    if(f_opt.is_defined("compressors"))
    {
        compressors = true;
    }
    if(f_opt.is_defined("archivers"))
    {
        archivers = true;
    }

    // force the default if nothing else was selected
    //
    if(!archivers)
    {
        compressors = true;
    }

    // if we have more than one list, then we want headers
    if((compressors ? 1 : 0)
     + (archivers ? 1 : 0)
            > 1)
    {
        f_headers = true;
    }

    if(compressors)
    {
        list_compressors();
    }

    if(archivers)
    {
        if(compressors)
        {
            std::cout << '\n';
        }
        list_archivers();
    }

    return 0;
}


void edhttp_list_compressor::list_compressors()
{
    advgetopt::string_list_t list(edhttp::compressor_list());

    if(f_headers)
    {
        std::cout << " Compressor\n"
                     "------------\n";
    }

    for(auto const & c : list)
    {
        std::cout << c << '\n';
    }
}


void edhttp_list_compressor::list_archivers()
{
    advgetopt::string_list_t list(edhttp::archiver_list());

    if(f_headers)
    {
        std::cout << " Archiver\n"
                     "----------\n";
    }

    for(auto const & a : list)
    {
        std::cout << a << '\n';
    }
}



}
// no name namespace



int main(int argc, char * argv[])
{
    libexcept::init_report_signal();
    libexcept::verify_inherited_files();

    try
    {
        edhttp_list_compressor l(argc, argv);
        return l.run();
    }
    catch(advgetopt::getopt_exit const & e)
    {
        return e.code();
    }
    catch(libexcept::exception_t const & e)
    {
        std::cerr
            << "error: a libexcept exception occurred: \""
            << e.what()
            << "\".\n";
    }
    catch(std::exception const & e)
    {
        std::cerr
            << "error: a standard exception occurred: \""
            << e.what()
            << "\".\n";
    }
    catch(...)
    {
        std::cerr << "error: an unknown exception occurred.\n";
    }

    return 1;
}



// vim: ts=4 sw=4 et
