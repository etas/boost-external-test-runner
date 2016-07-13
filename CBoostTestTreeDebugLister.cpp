#include "CBoostTestTreeDebugLister.h"

#include <stdexcept>

#define _NO_CVCONST_H
#include <DbgHelp.h>

//suppression of warnings related to 3rd party files
#pragma warning ( disable: 6001 )
#pragma warning ( disable: 6031 )
#include <boost/test/utils/xml_printer.hpp>
#pragma warning ( default: 6001 )
#pragma warning ( default: 6031 )
//end suppression of warnings related to 3rd party files

namespace etas
{
namespace boost
{
namespace unit_test
{

namespace
{

// Utility function which is used to simply avoid writing '::boost::unit_test::attr_value()'
::boost::unit_test::utils::attr_value attr_value()
{
    return ::boost::unit_test::utils::attr_value();
};

struct SSourceInfo
{
    SSourceInfo() :
        m_file(c_unkownLocation),
        m_lineNumber(0)
    {
    };

    std::string m_file;
    unsigned long m_lineNumber;

    static const std::string c_unkownLocation;
};

const std::string SSourceInfo::c_unkownLocation("unknown location");

struct SSearchRequest
{
    SSearchRequest(
        HANDLE handle,
        DWORD64 dllBase,
        const std::vector<const ::boost::unit_test::test_suite*>& suites,
        const ::boost::unit_test::test_case& test
    ) :
        m_handle(handle),
        m_dllBase(dllBase),
        m_suites(&suites),
        m_test(&test)
    {
    };

    HANDLE m_handle;
    DWORD64 m_dllBase;

    const std::vector<const ::boost::unit_test::test_suite*>* m_suites;
    const ::boost::unit_test::test_case* m_test;
};

struct SSearchContext
{
    SSearchContext(const SSearchRequest& request) :
        m_request(&request)
    {
    };

    const SSearchRequest* m_request;

    SSourceInfo m_testSuiteMatch;
    SSourceInfo m_freeMatch;
};

template <typename Iterator>
bool SubstringEqual(Iterator lhsBegin, Iterator lhsEnd, Iterator rhsBegin, Iterator rhsEnd)
{
    for (; (lhsBegin != lhsEnd) && (rhsBegin != rhsEnd); ++lhsBegin, ++rhsBegin)
    {
        if (*lhsBegin != *rhsBegin)
        {
            return false;
        }
    }

    return (rhsBegin == rhsEnd);
}

bool EndsWith(const std::string& base, const std::string& match)
{
    return SubstringEqual(base.rbegin(), base.rend(), match.rbegin(), match.rend());
}

std::string::const_iterator SubstringMatches(const std::string& base, const std::string::const_iterator begin, const std::string& match)
{
    if (SubstringEqual(begin, base.end(), match.begin(), match.end()))
    {
        return begin + match.length();
    }

    return base.end();
}

bool MatchesSearchRequest(const SSearchRequest& request, const std::string& name)
{
    // This is an auto registered test case, match the test suite as well.
    if (EndsWith(name, "::test_method"))
    {
        std::string::const_iterator index = name.begin();

        for (auto i = request.m_suites->begin(), end = request.m_suites->end(); i != end; ++i)
        {
            index = SubstringMatches(name, index, (*i)->p_name.value);

            if (index == name.end())
            {
                break;
            }

            index = SubstringMatches(name, index, "::");

            if (index == name.end())
            {
                break;
            }
        }

        if ((index != name.end()) && (SubstringMatches(name, index, request.m_test->p_name.value) != name.end()))
        {
            return true;
        }
    }

    return false;
}

const char* const GetName(SYMBOL_INFO& symInfo)
{
    return (symInfo.MaxNameLen > 0) ? symInfo.Name : nullptr;
};

BOOL CALLBACK EnumSymbolsCallback(PSYMBOL_INFO symInfo, ULONG symbolSize, PVOID userContext)
{
    SSearchContext* context = static_cast<SSearchContext*>(userContext);

    if (symInfo != NULL)
    {
        // Unfortunately, we do not make use the symInfo->Flags because it always returns 0 for some odd reason...

        const char* const name = GetName(*symInfo);

        if (name != nullptr)
        {
            SSourceInfo* info = nullptr;

            // Test for strong match, i.e. <test suite>::<test name>::test_method
            if (MatchesSearchRequest(*(context->m_request), name))
            {
                // This request satisfies the strong guarantee
                info = &context->m_testSuiteMatch;
            }
            // Test for lesser match, i.e. match name only.
            else if (context->m_request->m_test->p_name.value == name)
            {
                // This is a test case which was (most probably) programmatically registered
                info = &context->m_freeMatch;
            }

            // If either one of the guarantees is met, record line information
            if (info != nullptr)
            {
                IMAGEHLP_LINE64 line;
                DWORD displacement;

                if (SymGetLineFromAddr64(context->m_request->m_handle, symInfo->Address, &displacement, &line))
                {
                    info->m_file = line.FileName;
                    info->m_lineNumber = line.LineNumber;
                }
            }
        }
    }

    // Terminate enumeration if the strong guarantee is met.
    if (context->m_testSuiteMatch.m_file == SSourceInfo::c_unkownLocation)
    {
        // Source information has still not been found. Continue enumeration.
        return TRUE;
    }

    // Source information has been located. Terminate enumeration.
    return FALSE;
};

SSourceInfo GetSourceInfo(const SSearchRequest& request)
{
    if ((request.m_handle == NULL) || (request.m_dllBase == 0))
    {
        // No debug information can be retrieved since module has not been loaded correctly
        return SSourceInfo();
    }

    SSearchContext context(request);
    SymEnumSymbols(request.m_handle, request.m_dllBase, NULL, EnumSymbolsCallback, &context);

    // Prefer the auto registered case over the free registered case
    if (context.m_testSuiteMatch.m_file != SSourceInfo::c_unkownLocation)
    {
        return context.m_testSuiteMatch;
    }
    else if (context.m_freeMatch.m_file != SSourceInfo::c_unkownLocation)
    {
        return context.m_freeMatch;
    }

    // No source information is available
    return SSourceInfo();
};

} // namespace (anonymous)

CBoostTestTreeDebugLister::CBoostTestTreeDebugLister(const std::string& source) :
    TBase(source),
    m_handle(NULL),
    m_dllBase(-1)
{
    Init();
}

CBoostTestTreeDebugLister::CBoostTestTreeDebugLister(const std::string& source, std::ostream* out) :
    TBase(source, out),
    m_handle(NULL),
    m_dllBase(0)
{
    Init();
}

CBoostTestTreeDebugLister::~CBoostTestTreeDebugLister()
{
    if (m_handle != NULL)
    {
        if (m_dllBase > 0)
        {
            SymUnloadModule64(m_handle, m_dllBase);
        }

        SymCleanup(m_handle);
    }
}

void CBoostTestTreeDebugLister::Init()
{
    m_handle = GetCurrentProcess();

    if (SymInitialize(m_handle, NULL, FALSE))
    {
        SymSetOptions(SYMOPT_LOAD_LINES);

        m_dllBase = SymLoadModuleEx(m_handle, NULL, GetSource().c_str(), NULL, 0, 0, NULL, 0);
    }
    else
    {
        m_handle = NULL;
    }
}

void CBoostTestTreeDebugLister::visit(const ::boost::unit_test::test_case& testCase)
{
    SSourceInfo info = GetSourceInfo(SSearchRequest(m_handle, m_dllBase, m_suites, testCase));

    Tab() << "<TestCase"
          " id" << attr_value() << testCase.p_id <<
          " name" << attr_value() << testCase.p_name.value;

    // Avoid listing source information for unkown sources
    if (info.m_file != SSourceInfo::c_unkownLocation)
    {
        Out() << " file" << attr_value() << info.m_file <<
              " line" << attr_value() << info.m_lineNumber;
    }

    Out() << " />";

    if (GetPrettyPrint())
    {
        Out() << std::endl;
    }
}

bool CBoostTestTreeDebugLister::IsMasterTestSuite(const ::boost::unit_test::test_suite& testSuite) const
{
    return testSuite.p_id == 1;
}

bool CBoostTestTreeDebugLister::test_suite_start(const ::boost::unit_test::test_suite& testSuite)
{
    // Skip the master test suite
    if (!IsMasterTestSuite(testSuite))
    {
        m_suites.push_back(&testSuite);
    }

    return TBase::test_suite_start(testSuite);
}

void CBoostTestTreeDebugLister::test_suite_finish(const ::boost::unit_test::test_suite& testSuite)
{
    if (!IsMasterTestSuite(testSuite))
    {
        m_suites.pop_back();
    }

    return TBase::test_suite_finish(testSuite);
}

} // namespace unit_test
} // namespace boost
} // namespace etas