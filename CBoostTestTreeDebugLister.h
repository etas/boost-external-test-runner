#if !defined( _CBoostTestTreeDebugLister_H_ )
#define _CBoostTestTreeDebugLister_H_

#include <vector>

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include "CBoostTestTreeLister.h"

namespace etas
{
namespace boost
{
namespace unit_test
{

/**
 * @brief CBoostTestTreeLister extension which additionally serializes debug information for test cases.
 */
class CBoostTestTreeDebugLister : public CBoostTestTreeLister
{
public:
    typedef CBoostTestTreeLister TBase;

    /**
     * @brief Constructor. Output stream defaults to std::out.
     *
     * @param[in] source file-path to the exe/dll module which contains a Boost test framework
     */
    explicit CBoostTestTreeDebugLister(const std::string& source);
    
    /**
     * @brief Constructor
     *
     * @param[in] source file-path to the exe/dll module which contains a Boost test framework
     * @param[in] out a pointer to the output stream which will be used to output the xml result
     */
    CBoostTestTreeDebugLister(const std::string& source, std::ostream* out);

    virtual ~CBoostTestTreeDebugLister();

    // test tree visitor interface
    virtual void visit(const ::boost::unit_test::test_case& testCase) override;
    virtual bool test_suite_start(const ::boost::unit_test::test_suite& testSuite) override;
    virtual void test_suite_finish(const ::boost::unit_test::test_suite& testSuite) override;

    /**
     * @brief States whether or not debug information is available for the requested source module
     */
    const bool IsDebugInfoAvailable() const
    {
        return m_dllBase > 0;
    };

private:
    /**
     * @brief Common initialisation logic shared between constructors
     */
    void Init();
    
    /**
     * @brief States whether the provided test suite is the master test suite
     * @param[in] testSuite the test suite to test
     * @brief States whether the provided test suite is the master test suite
     */
    bool IsMasterTestSuite(const ::boost::unit_test::test_suite& testSuite) const;

private:
    HANDLE m_handle;
    DWORD64 m_dllBase;

    typedef const ::boost::unit_test::test_suite* TConstTestSuitePtr;
    std::vector<TConstTestSuitePtr> m_suites;
};

} // namespace unit_test
} // namespace boost
} // namespace etas

#endif // _CBoostTestTreeDebugLister_H_