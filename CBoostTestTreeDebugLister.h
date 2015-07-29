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

class CBoostTestTreeDebugLister : public CBoostTestTreeLister
{
public:
    typedef CBoostTestTreeLister TBase;

    explicit CBoostTestTreeDebugLister(const std::string& source);
    CBoostTestTreeDebugLister(const std::string& source, std::ostream* out);

    virtual ~CBoostTestTreeDebugLister();

    // test tree visitor interface
    virtual void visit(const ::boost::unit_test::test_case& testCase) override;
    virtual bool test_suite_start(const ::boost::unit_test::test_suite& testSuite) override;
    virtual void test_suite_finish(const ::boost::unit_test::test_suite& testSuite) override;

    const bool IsDebugInfoAvailable() const
    {
        return m_dllBase > 0;
    };

private:
    void Init();
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