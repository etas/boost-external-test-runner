#if !defined( _CBoostTestTreeLister_H_ )
#define _CBoostTestTreeLister_H_

#include <ostream>
#include <string>

#include <boost/noncopyable.hpp>
#include <boost/test/tree/visitor.hpp>

namespace etas
{
namespace boost
{
namespace unit_test
{

/**
 * @brief Boost test_tree_visitor implementation which traverses the test tree and 
 * 		  generates an xml file of available tests
 */
class CBoostTestTreeLister :
    public ::boost::unit_test::test_tree_visitor,
    private ::boost::noncopyable
{
public:
    typedef ::boost::unit_test::test_tree_visitor TBase;

    /**
     * @brief Constructor. Output stream defaults to std::out.
     *
     * @param[in] source file-path to the exe/dll module which contains a Boost test framework
     */
    explicit CBoostTestTreeLister(const std::string& source);
    
    /**
     * @brief Constructor
     *
     * @param[in] source file-path to the exe/dll module which contains a Boost test framework
     * @param[in] out a pointer to the output stream which will be used to output the xml result
     */
    CBoostTestTreeLister(const std::string& source, std::ostream* out);

    /**
     * @brief Getter for the source module file path which contains the boost test framework
     */
    inline const std::string& GetSource() const
    {
        return m_source;
    };

    /**
     * @brief States whether or not xml pretty printing is enabled
     */
    inline bool GetPrettyPrint() const
    {
        return m_prettyPrint;
    }

    /**
     * @brief Sets whether or not xml pretty printing is enabled
     * 
     * @param[in] prettyPrint true to enable pretty print; false to disable pretty printing
     */
    void SetPrettyPrint(bool prettyPrint)
    {
        m_prettyPrint = prettyPrint;
    }

    /**
     * @brief Destructor
     */
    virtual ~CBoostTestTreeLister();

    // test tree visitor interface
    
    /**
     * @brief Visitor method for a Boost test case. Describes the test is an xml element.
     */
    virtual void visit(const ::boost::unit_test::test_case& testCase) override;
    
    /**
     * @brief Visitor method for the start of a Boost test suite.
     */
    virtual bool test_suite_start(const ::boost::unit_test::test_suite& testSuite) override;
    
    /**
     * @brief Visitor method for the end of a Boost test suite.
     */
    virtual void test_suite_finish(const ::boost::unit_test::test_suite& testSuite) override;

    /**
     * @brief Writes the xml declaration and preamble
     * @return the output stream which we are writing to
     */
    std::ostream& WriteHeader();
    
    /**
     * @brief Writes the closing xml elements for the file
     * @return the output stream which we are writing to
     */
    std::ostream& WriteTrailer();

protected:
    /**
     * @brief Properly sets up tabulation characters if pretty-printing is enabled
     * @return the output stream which we are writing to
     */
    std::ostream& Tab();
    
    /**
     * @brief Getter for the current output stream.
     * @return the output stream which we are writing to
     */
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