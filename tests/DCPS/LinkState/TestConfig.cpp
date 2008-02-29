
#include "TestConfig.h"
#include "TestException.h"
#include "ace/Arg_Shifter.h"
#include "ace/OS_NS_stdlib.h"
// #include "ace/Log_Msg.h"

namespace { // anonymous namespace for file scope.
  //
  // Default values.
  //
  const bool  TC_DEFAULT_VERBOSE = false;
  const char* TC_DEFAULT_FILE    = "test.in";

} // end of anonymous namespace.

TestConfig::~TestConfig()
{
}

TestConfig::TestConfig( int argc, char** argv, char** envp)
 : verbose_( TC_DEFAULT_VERBOSE),
   fileName_( TC_DEFAULT_FILE)
{
  ACE_UNUSED_ARG(envp);

  ACE_Arg_Shifter parser( argc, argv);
  while( parser.is_anything_left()) {
    const char* currentArg = 0;

    if( 0 != (currentArg = parser.get_the_parameter("-FileName"))) {
      this->fileName_ = currentArg;
      parser.consume_arg();

    } else if( 0 != (currentArg = parser.get_the_parameter("-v"))) {
      this->verbose_ = true;

    } else {
      parser.ignore_arg();
    }
  }
}

bool&
TestConfig::verbose()
{
  return this->verbose_;
}

bool
TestConfig::verbose() const
{
  return this->verbose_;
}

std::string&
TestConfig::fileName()
{
  return this->fileName_;
}

std::string
TestConfig::fileName() const
{
  return this->fileName_;
}

