#ifndef TESTCONFIG_H
#define TESTCONFIG_H

#include <vector>
#include <map>
#include <string>

class TestConfig {
  public:
    /**
     * @brief Construct from the command line.
     *
     * This constructor passes the command line to the DCPS subsystem,
     * which in turn passes the arguments to the ORB initialization, then
     * parses the remainder to determine subsystem behavior:
     *
     *   -v                     - verbose operation
     *   -FileName <string>     - name of test input file
     */
    TestConfig( int argc, char** argv, char** envp);

    /// Virtual destructor.
    virtual ~TestConfig();

    /// Spew indicator.
    bool& verbose();
    bool  verbose() const;

    /// File name.
    std::string& fileName();
    std::string  fileName() const;

  private:
    /// Be chatty while executing.
    bool verbose_;

    /// Data type name.
    std::string fileName_;
};

#endif // TESTCONFIG_H

