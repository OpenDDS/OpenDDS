// -*- C++ -*-
//

#include "dds/DCPS/debug.h"
#include "Options.h"
#include "ace/Arg_Shifter.h"
#include "ace/Log_Priority.h"
#include "ace/Log_Msg.h"
#include "ace/OS_NS_stdlib.h"
#include "ace/Configuration.h"
#include "ace/Configuration_Import_Export.h"

#if !defined (__ACE_INLINE__)
# include "Options.inl"
#endif /* ! __ACE_INLINE__ */

#include <string>
#include <iostream>

namespace { // anonymous namespace for file scope.
  //
  // Default values.
  //
  enum { DEFAULT_TEST_DOMAIN     = 521};
  enum { DEFAULT_PUBLICATIONS    =   3};
  enum { DEFAULT_TRANSPORT_KEY   =   1};

  const char* DEFAULT_TEST_TOPICNAME = "TestTopic";

  // Command line argument definitions.
  const ACE_TCHAR* VERBOSE_ARGUMENT = ACE_TEXT("-v");
  const ACE_TCHAR* PUBLISHER_ARGUMENT = ACE_TEXT("-p");

} // end of anonymous namespace.

namespace Test {

Options::~Options()
{
}

Options::Options( int argc, ACE_TCHAR** argv, char** /* envp */)
 : valid_(        true),
   verbose_(      false),
   publisher_(    false),
   domain_(       DEFAULT_TEST_DOMAIN),
   topicName_(    DEFAULT_TEST_TOPICNAME),
   publications_( DEFAULT_PUBLICATIONS),
   transportKey_( DEFAULT_TRANSPORT_KEY)
{
  ACE_Arg_Shifter parser( argc, argv);
  parser.ignore_arg(); // Ignore the first arg, it's executable path and name.

  while( parser.is_anything_left()) {
    if( this->verbose()) {
      ACE_DEBUG((LM_DEBUG,
        ACE_TEXT("(%P|%t) Options::Options() - ")
        ACE_TEXT("processing argument: %s.\n"),
        parser.get_current()
      ));
    }
    if( 0 <= (parser.cur_arg_strncasecmp( VERBOSE_ARGUMENT))) {
      this->verbose_ = true;
      parser.consume_arg();

    } else if (0 <= (parser.cur_arg_strncasecmp(PUBLISHER_ARGUMENT))) {
      this->publisher_ = true;
      parser.consume_arg();

    } else {
      if( ::OpenDDS::DCPS::DCPS_debug_level > 0) {
        ACE_DEBUG((LM_WARNING,
          ACE_TEXT("(%P|%t) WARNING: Options::Options() - ")
          ACE_TEXT("ignoring argument: %s.\n"),
          parser.get_current()
        ));
      }
      parser.ignore_arg();
    }
  }
}

} // End of namespace Test

