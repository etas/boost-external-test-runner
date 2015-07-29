#if !defined( _CBoostTestTreeLister_H_ )
#define _CBoostTestTreeLister_H_

#include <ostream>
#include <string>

#include <boost/noncopyable.hpp>
#include <boost/test/unit_test_suite_impl.hpp>

namespace etas
{
namespace boost
{
namespace unit_test
{

class CBoostTestTreeLister :
    public ::boost::unit_test::test_tree_visitor,
    private ::boost::noncopyable
{
public:
    typedef ::boost::unit_test::test_tree_visitor TBase;

    explicit CBoostTestTreeLister(const std::string& source);
    CBoostTestTreeLister(const std::string& source, std::ostream* out);

    inline const std::string& GetSource() const
    {
        return m_source;
    };

    inline bool GetPrettyPrint() const
    {
        return m_prettyPrint;
    }

    void SetPrettyPrint(bool prettyPrint)
    {
        m_prettyPrint = prettyPrint;
    }

    virtual ~CBoostTestTreeLister();

    // test tree visitor interface
    virtual void visit(const ::boost::unit_test::test_case& testCase) override;
    virtual bool test_suite_start(const ::boost::unit_test::test_suite& testSuite) override;
    virtual void test_suite_finish(const ::boost::unit_test::test_suite& testSuite) override;

    std::ostream& WriteHeader();
    std::ostream& WriteTrailer();

protected:
    std::ostream& Tab();
    std::ostream& Out();

private:
    std::ostream* m_out;

    std::string m_source;

    std::size_t m_level;
    bool m_prettyPrint;
};

} // namespace unit_test
} // namespace boost
} // namespace etas

#endif // _CBoostTestTreeLister_H_