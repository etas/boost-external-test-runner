//  (C) Copyright Gennadiy Rozental 2005-2007.
//  Distributed under the Boost Software License, Version 1.0.
//  (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org/libs/test for the library home page.

// Boost.Test
#ifndef BOOST_TEST_DYN_LINK
#define BOOST_TEST_DYN_LINK
#endif

//suppression of warnings related to 3rd party files
#pragma warning ( disable: 6031 )
#pragma warning ( disable: 6011 )
#pragma warning ( disable: 6001 )

#include <boost/test/unit_test.hpp>

// Boost.Runtime.Param
#include <boost/test/utils/runtime/cla/named_parameter.hpp>
#include <boost/test/utils/runtime/cla/parser.hpp>

//end suppression of warnings related to 3rd party files
#pragma warning ( default: 6031 )
#pragma warning ( default: 6011 )
#pragma warning ( default: 6001 )

namespace rt  = boost::runtime;
namespace cla = boost::runtime::cla;

// STL
#include <iostream>

#include <boost/cstdlib.hpp>    // for exit codes

#include <fstream>
#include "CBoostTestTreeLister.h"
#include "CBoostTestTreeDebugLister.h"

//_________________________________________________________________//

// System API

namespace dyn_lib
{
#if defined(BOOST_WINDOWS) && !defined(BOOST_DISABLE_WIN32) // WIN32 API

#include <windows.h>

typedef HINSTANCE handle;

/**
@brief Loads a dll library on the Windows OS
*/
inline handle
open(std::string const& file_name)
{
    return LoadLibrary(file_name.c_str());
}

//_________________________________________________________________//

/**
@brief Locates a method inside the loaded library on the Windows OS. The method needs to have C style interface. In case of troubles at this step it is
suggested the use of dependency walker (http://www.dependencywalker.com/) so that it is made sure that the mangling used when generating
the dll has been of correct type.
*/
template<typename TargType>
inline TargType
locate_symbol(handle h, std::string const& symbol)
{
    return reinterpret_cast<TargType>(GetProcAddress(h, symbol.c_str()));
}

//_________________________________________________________________//

/**
@brief  Unloads a library on the Windows OS
*/
inline void
close(handle h)
{
    if (h)
    {
        FreeLibrary(h);
    }
}

//_________________________________________________________________//

inline std::string
error()
{
    LPTSTR msg = NULL;

    FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
                  NULL,
                  GetLastError(),
                  MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                  (LPTSTR)&msg,
                  0, NULL);

    std::string res;

    if (msg)
    {
        res = msg;
        LocalFree(msg);
    }

    return res;
}

//_________________________________________________________________//

#elif defined(BOOST_HAS_UNISTD_H) // POSIX API

#include <dlfcn.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

typedef void* handle;

/**
*   @brief Loads a dll library on a POSIX run-time environment
*/
inline handle
open(std::string const& file_name)
{
    return dlopen(file_name.c_str(), RTLD_LOCAL | RTLD_LAZY);
}

//_________________________________________________________________//
/**
*    @brief Locates a method inside the loaded library on a POSIX run-time environment
*/
template<typename TargType>
inline TargType
locate_symbol(handle h, std::string const& symbol)
{
    return reinterpret_cast<TargType>(dlsym(h, symbol.c_str()));
}

//_________________________________________________________________//
/*
*   @brief Unloads a dll library on a POSIX run-time environment
*/
inline void
close(handle h)
{
    if (h)
    {
        dlclose(h);
    }
}

//_________________________________________________________________//

inline std::string
error()
{
    return dlerror();
}

//_________________________________________________________________//

#else

#error "Dynamic library API is unknown"

#endif
} // namespace dyn_lib

//____________________________________________________________________________//

static std::string test_lib_name;
static std::string init_func_name("init_unit_test");

dyn_lib::handle test_lib_handle;

typedef bool (*init_func_ptr)();

//____________________________________________________________________________//
/**
*   @brief Load the Boost UTF dll containing the tests, locate the initialization method
*          (either the default "init_unit_test" or the one supplied via parameter --init) and in case
*          the initialization method is found, call it.
*
*   @return returns true in case the initialization method has not been located (so that the enumeration can proceed anyway)
*            In case the initialization method has been found, the method is called and whatever status that method return returns
*           is returned by load_test_lib
*/
bool load_test_lib()
{
    init_func_ptr init_func;

    test_lib_handle = dyn_lib::open(test_lib_name);    //load the library via the relevant OS API

    if (!test_lib_handle)
        throw std::logic_error(std::string("Fail to load test library: ")
                               .append(dyn_lib::error()));

    init_func = dyn_lib::locate_symbol<init_func_ptr>(test_lib_handle, init_func_name);    //locate the initialization method inside the library

    /**
    @note It has been decided to suppress that the raise of an exception in case the initialization method is not method.  At any rate the user
    will know that his manually registered tests has not been listed because they will not appear in the XML file.
    */
    /*
    if( !init_func )
    throw std::logic_error( std::string("Can't locate test initialization function ")
    .append( init_func_name )
    .append( ": " )
    .append( dyn_lib::error() ) );
    */

    return (init_func == nullptr) || ((*init_func)());
}

//____________________________________________________________________________//
/**
*   @brief Generates an object of type ofstream so as to write to file
*
*   @param [in]  P   Reference to the object handling the command line parsing
*   @param [in]  arg Full filepath where to write the XML file to
*/
std::unique_ptr<std::ofstream> GetListOutputStream(const cla::parser& P, const std::string& arg)
{
    std::string listOut;
    assign_op(listOut, P.get(arg), 0);

    if (!listOut.empty())
    {
        return std::unique_ptr<std::ofstream>(new std::ofstream(listOut, (std::ios_base::out | std::ios_base::trunc)));
    }

    return std::unique_ptr<std::ofstream>();
}

//____________________________________________________________________________//

typedef std::unique_ptr<::etas::boost::unit_test::CBoostTestTreeLister> TBoostTestTreeListerPtr;

/*
*   @brief Factory method return the correct type of test enumerator(lister) depending on the verbosity required
*
*   @param [in]  arg      string containing either either "list" or "list-debug" on which the factory method will use to determine the type of lister to return
*   @param [in]  dll      library path containing the Boost UTF tests
*   @param [in]  out      pointer to an output stream class which the test enumerator will use to output the report
*   @return      object   of either type CBoostTestTreeLister or CBoostTestTreeDebugLister depending on the verbosity required as supplied by argument arg
*/
TBoostTestTreeListerPtr GetTestTreeLister(const std::string& arg, const std::string dll, std::ostream* out = nullptr)
{
    out = (out == nullptr) ? &std::cout : out;

    if (arg == "list")
    {
        return TBoostTestTreeListerPtr(new ::etas::boost::unit_test::CBoostTestTreeLister(dll, out)); //less detail
    }
    else
    {
        return TBoostTestTreeListerPtr(new ::etas::boost::unit_test::CBoostTestTreeDebugLister(dll, out)); //more detail
    }

}

//____________________________________________________________________________//
/**
*   @brief Method utilized to format and write any exceptions to the output stream
*
*   @param [in] out    reference to the output stream where to write the exception to
*   @param [in] dll    library path containing the Boost UTF tests
*   @param [in] error  exception detail
*/
std::ostream& WriteError(std::ostream& out, const std::string& dll, const std::string& error = std::string())
{
    out << "<![CDATA[Error: Could not load " << dll << '.';

    if (!error.empty())
    {
        out << " Detail: " << error;
    }

    out << "]]>";

    return out;
}

//____________________________________________________________________________//
/**
*   @brief Method handling the enumeration of the tests.
*
*   @param [in]  P   Reference to the object handling the command line parsing
*   @return          Returns either boost::exit_success or boost::exit_failure
*/
int ListTests(const cla::parser& P)
{
    int res = ::boost::exit_success;

    std::string arg = (P["list"]) ? "list" : "list-debug";

    std::unique_ptr<std::ofstream> out = GetListOutputStream(P, arg);
    TBoostTestTreeListerPtr lister = GetTestTreeLister(arg, test_lib_name, out.get());

    if (lister != nullptr)
    {
        std::ostream& listing = lister->WriteHeader();

        try
        {
            /**
            * Load library before enumerating test suite to ensure correct registration.
            * Execute initialization function since it may be the case that client code registers custom tests
            */
            if (load_test_lib())
            {
                //if the loading the library was successfull, then we proceed in enumerating the tests.
                ::boost::unit_test::traverse_test_tree(::boost::unit_test::framework::master_test_suite(), *lister);
            }
        }
        catch (std::exception& ex)
        {
            WriteError(listing, test_lib_name, ((ex.what() == nullptr) ? std::string() : ex.what()));
            res = ::boost::exit_failure;
        }
        catch (...)
        {
            WriteError(listing, test_lib_name);
            res = ::boost::exit_failure;
        }

        lister->WriteTrailer();
    }

    return res;
}

//____________________________________________________________________________//

/** @mainpage External Boost Test Runner Usage
*
*The External Boost Test Runner has been created so as to permit the discovery and execution of unit tests contained in a build
*   that is not compiled as an executable but is compiled as a dynamic-link library.
*
*   The command line options of the Boost External Test Runner are:
*
*   @par --init
*    used to define the name of the initialization function to call, which by default is the Boost UTF supplied <b>init_unit_test</b> function. The initialization function
*   needs to be exported in a <a href="http://www.tldp.org/HOWTO/C++-dlopen/thesolution.html">C style interface</a> otherwise the Boost External Test Runner will fail to find the entry point of the method.
*
*   @par --test
*   used to define the path and the name of the DLL containing the Boost UTF tests.
*
*   @par --list-debug
*   used to define the path of the output XML file that will contain the test suites, the respective tests contained in the test suite, the source file and the line number
*   where the test has been declared. A sample XML generated by the command directive <--list-debug> is shown here below
*
@code{.xml}
<?xml version="1.0" encoding="UTF-8" ?>
<BoostTestFramework source="D:\dev\svn\SampleBoostProject\Debug\TestProject.dll">
    <TestSuite id="1" name="Master Test Suite">
        <TestSuite id="2" name="ExampleTestSuite">
            <TestCase id="65536" name="NumberTestCaseA" file="d:\dev\svn\testproject\numbertest.cpp" line="39" />
            <TestCase id="65537" name="NumberTestCaseB" file="d:\dev\svn\testproject\numbertest.cpp" line="54" />
            <TestCase id="65538" name="NumberTestCaseC" file="d:\dev\svn\testproject\numbertest.cpp" line="66" />
            <TestCase id="65539" name="NumberTestCaseD" file="d:\dev\svn\testproject\numbertest.cpp" line="80" />
            <TestCase id="65540" name="NumberTestCaseE" file="d:\dev\svn\testproject\numbertest.cpp" line="92" />
            <TestCase id="65541" name="NumberTestCaseF" file="d:\dev\svn\testproject\numbertest.cpp" line="104" />
            <TestCase id="65542" name="NumberTestCaseG" file="d:\dev\svn\testproject\numbertest.cpp" line="118" />
            <TestCase id="65543" name="NumberTestCaseH" file="d:\dev\svn\testproject\numbertest.cpp" line="134" />
            <TestCase id="65544" name="NumberTestCaseI" file="d:\dev\svn\testproject\numbertest.cpp" line="156" />
            <TestCase id="65545" name="NumberTestCaseJ" file="d:\dev\svn\testproject\numbertest.cpp" line="168" />
            <TestCase id="65546" name="NumberTestCaseK" file="d:\dev\svn\testproject\numbertest.cpp" line="190" />
        </TestSuite>
    </TestSuite>
</BoostTestFramework>
@endcode

*@par --list
*   used to define the path of the output XML file similar to <b>--list-debug</b>, but will contain only the test suites and the names of the tests.
* A sample XML generated by the command directive <b>--list</b> is shown here below
*
~~~~~~~~~~~~~{.xml}
<?xml version="1.0" encoding="UTF-8" ?>
<BoostTestFramework source="D:\dev\svn\SampleBoostProject\Debug\TestProject.dll">
    <TestSuite id="1" name="Master Test Suite">
        <TestSuite id="2" name="ExampleTestSuite">
            <TestCase id="65536" name="NumberTestCaseA" />
            <TestCase id="65537" name="NumberTestCaseB" />
            <TestCase id="65538" name="NumberTestCaseC" />
            <TestCase id="65539" name="NumberTestCaseD" />
            <TestCase id="65540" name="NumberTestCaseE" />
            <TestCase id="65541" name="NumberTestCaseF" />
            <TestCase id="65542" name="NumberTestCaseG" />
            <TestCase id="65543" name="NumberTestCaseH" />
            <TestCase id="65544" name="NumberTestCaseI" />
            <TestCase id="65545" name="NumberTestCaseJ" />
            <TestCase id="65546" name="NumberTestCaseK" />
        </TestSuite>
    </TestSuite>
</BoostTestFramework>
~~~~~~~~~~~~~
*
*The typical command line usage of the Boost External Test Runner so as to enumerate tests is
*
*   <c>BoostExternalTestRunner.exe --test "{source}" --list-debug "{out}"</c>
*
*   where <c>{source}</c> would be replaced by the full or relative path along with the filename of the DLL containing the Boost UTF tests
*   and <c>{out}</c> would be replaced by the full or relative path along with filename of where the output XML file containing the enumeration
*   of tests would be generated at.
*
*   An example of the enumeration command is
*
*    <c>BoostExternalTestRunner.exe --test "D:\dev\svn\SampleBoostProject\Debug\TestProject.dll" --list-debug "D:\dev\svn\SampleBoostProject\Debug\TestProject.detailed.xml"</c>
*
*   The typical command line usage of the Boost External Test Runner so as to execute tests is
*
*   <c>BoostExternalTestRunner.exe --test "{source}" {boost-args}</c>
*
*   where <c>{source}</c> would be replaced by the full or relative path along with the filename of the dll containing the Boost UTF tests
*   and <c>{boost-args}</c> would contain Boost UTF directives as documented at http://www.boost.org/doc/libs/1_55_0/libs/test/doc/html/utf/user-guide.html or more specifically at
* http://www.boost.org/doc/libs/1_55_0/libs/test/doc/html/utf/user-guide/runtime-config/run-by-name.html
*
*   An example of a valid Boost External Test Runner command so as to run a specific test is <c>BoostExternalTestRunner.exe --test "D:\dev\svn\SampleBoostProject\Debug\TestProject.dll" --log_level=test_suite --run_test=ExampleTestSuite/NumberTestCaseA</c>.
*
*   Another example command where a user might want to execute all tests without specifying any Boost UTF options is
*   <c>BoostExternalTestRunner.exe --test "D:\dev\svn\SampleBoostProject\Debug\TestProject.dll"</c>.
*
*In order for the External Boost Test Runner to be able to successfully load, discover and execute tests of a DLL build containing the Boost Unit Tests
*   the following conditions must be observed:
*
*@par
*i) Both the External Test Runner and DLL must be built with same configuration type (<b>debug</b> or <b>release</b>).
*@par
*ii) Both the External Test Runner and DLL must be built with same target solution (<b>Win32</b> or <b>x64</b>).
*@par
*iii) Both the External Test Runner and dll must be built using the same version of Boost.
*@par
*iv) stdafx.h must contain the following \#define and the following \#include.
~~~~~~~~~~~~~{.cpp}
#ifndef BOOST_TEST_DYN_LINK
#define BOOST_TEST_DYN_LINK
#endif

#include <boost/test/unit_test.hpp>
~~~~~~~~~~~~~
*/
/*!
  \page License
  
##  Boost External Test Runner License
  
Boost External Test Runner is released under the <a href="http://www.boost.org/LICENSE_1_0.txt">Boost Software License - Version 1.0 - August 17th, 2003</a>.

##  Third party software credits

Boost External Test Runner makes use of a source file, namely console_test_runner.cpp, copyrighted by Gennadiy Rozental 2005 under the Boost Software License Version 1.0 which is supplied as part of the Boost sources under path libs\test\tools\console_test_runner\test\test_runner_test.cpp

*/

/**
*   @brief Entry point of the program
*
*   @param [in]  argc   argument count
*   @param [in]  argv   argument vector
*   @return             Indicates execution failure or success
*/
int main(int argc, char* argv[])
{
    try
    {
        cla::parser P;

        P - cla::ignore_mismatch
                << cla::named_parameter<rt::cstring>("test") - (cla::prefix = "--")
                << cla::named_parameter<rt::cstring>("list") - (cla::prefix = "--", cla::optional)
                << cla::named_parameter<rt::cstring>("list-debug") - (cla::prefix = "--", cla::optional)
                << cla::named_parameter<rt::cstring>("init") - (cla::prefix = "--", cla::optional);

        P.parse(argc, argv);

        assign_op(test_lib_name, P.get("test"), 0);

        if (P["init"])
        {
            assign_op(init_func_name, P.get("init"), 0);
        }

        int res = ::boost::exit_success;

        //if the list or the list-debug command line directives are present then just enumerate tests,
        //otherwise execute the tests according to the additional Boost UTF specific  command line options supplied
        if (P["list"] || P["list-debug"])
        {
            res = ListTests(P);
        }
        else
        {
            //run tests
            res = ::boost::unit_test::unit_test_main(&load_test_lib, argc, argv);
        }

        ::boost::unit_test::framework::clear();
        dyn_lib::close(test_lib_handle);    //unload the library

        return res;
    }
    catch (rt::logic_error const& ex)
    {
        std::cout << "Fail to parse command line arguments: " << ex.msg() << std::endl;
        return -1;
    }
}

//____________________________________________________________________________//

// EOF