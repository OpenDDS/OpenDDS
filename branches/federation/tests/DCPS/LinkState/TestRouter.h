#ifndef TESTROUTER_H
#define TESTROUTER_H

#include "TestConfig.h"
#include "DiagnosticManager.h"

/**
 * @brief Test subsystem for LinkStateManger
 */
class TestRouter {
  public:
    /// Construct from command line.
    TestRouter( int argc, char** argv, char** envp);

    /// Virtual destructor.
    virtual ~TestRouter();

    /// Main actions occur here.
    void run();

  private:
    /// Configuration information.
    TestConfig config_;

    /// LinkStateManger to test.
    DiagnosticManager manager_;
};

#endif // TESTROUTER_H

