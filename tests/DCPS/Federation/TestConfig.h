#ifndef TESTCONFIG_H
#define TESTCONFIG_H

#include "ace/config-lite.h"
#include "ace/ace_wchar.h"
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
     *   -Samples <number>      - number of samples to emit into the subsystems
     */
    TestConfig( int argc, ACE_TCHAR** argv, char** envp);

    virtual ~TestConfig();

    /// Spew indicator.
    bool& verbose();
    bool  verbose() const;

    /// Number of samples to emit.
    int& samples();
    int  samples() const;

    /// Interval between each sample sending.
    int& sample_interval ();
    int  sample_interval() const;

    /// Domain
    int& domain();
    int  domain() const;

    /// Type name
    std::string& typeName();
    std::string  typeName() const;

    /// Topic name
    std::string& topicName();
    std::string  topicName() const;

  private:
    /// Be chatty while executing.
    bool verbose_;

    /// Number of samples to emit into the subsystems.
    int samples_;

    /// Interval in seconds between sample sending.
    int sample_interval_;

    /// Domain value.
    int domain_;

    /// Type name.
    std::string typeName_;

    /// Topic name.
    std::string topicName_;
};

#endif // TESTCONFIG_H

