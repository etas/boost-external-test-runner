#include "CBoostTestTreeLister.h"

#include <iostream>

//suppression of warnings related to 3rd party files
#pragma warning ( disable: 6001 )
#pragma warning ( disable: 6031 )

#include <boost/test/utils/xml_printer.hpp>

//end suppression of warnings related to 3rd party files
#pragma warning ( default: 6001 )
#pragma warning ( default: 6031 )


namespace
{

std::ostream& noop(std::ostream& out)
{
    return out;
};

// Utility function which is used to simply avoid writing '::boost::unit_test::attr_value()'
::boost::unit_test::attr_value attr_value()
{
    return ::boost::unit_test::attr_value();
};

} // namespace anonymous

namespace etas
{
namespace boost
{
namespace unit_test
{

#define ENDLINE ((m_prettyPrint) ? static_cast<std::ostream& (*)(std::ostream&)>(std::endl) : noop)

CBoostTestTreeLister::CBoostTestTreeLister(const std::string& source) :
    m_out(&std::cout),
    m_source(source),
    m_level(0),
    m_prettyPrint(true)
{
    WriteHeader();
}

CBoostTestTreeLister::CBoostTestTreeLister(const std::string& source, std::ostream* out) :
    m_out(out),
    m_source(source),
    m_level(0),
    m_prettyPrint(true)
{
}

CBoostTestTreeLister::~CBoostTestTreeLister()
{
}

void CBoostTestTreeLister::visit(const ::boost::unit_test::test_case& testCase)
{
    Tab() << "<TestCase id" << attr_value() << testCase.p_id << " name" << attr_value() << testCase.p_name.value << " />" << ENDLINE;
}

bool CBoostTestTreeLister::test_suite_start(const ::boost::unit_test::test_suite& testSuite)
{
    Tab() << "<TestSuite id" << attr_value() << testSuite.p_id << " name" << attr_value() << testSuite.p_name.value << '>' << ENDLINE;
    ++m_level;

    return true;
}

void CBoostTestTreeLister::test_suite_finish(const ::boost::unit_test::test_suite& testSuite)
{
    m_level = (m_level == 0) ? 0 : (m_level - 1);
    Tab() << "</TestSuite>" << ENDLINE;
}

std::ostream& CBoostTestTreeLister::WriteHeader()
{
    Out() << "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>" << ENDLINE
          << "<BoostTestFramework source" << attr_value() << m_source << '>' << ENDLINE;

    ++m_level;

    return Out();
}

std::ostream& CBoostTestTreeLister::WriteTrailer()
{
    --m_level;

    return Out() << "</BoostTestFramework>" << ENDLINE;
}

std::ostream& CBoostTestTreeLister::Tab()
{
    if (m_prettyPrint)
    {
        for (std::size_t i = 0; i < m_level; ++i)
        {
            Out() << "    ";
        }
    }

    return Out();
}

std::ostream& CBoostTestTreeLister::Out()
{
    return *m_out;
}

} // namespace unit_test
} // namespace boost
} // namespace etas