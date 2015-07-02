// -*- C++ -*-
//
/*
*
*
* Distributed under the OpenDDS License.
* See: http://www.opendds.org/license.html
*/

#include "dds/DCPS/debug.h"
#include "dds/monitor/monitorC.h"
#include "Options.h"
#include "ace/Arg_Shifter.h"
#include "ace/Log_Priority.h"
#include "ace/Log_Msg.h"
#include "ace/OS_NS_stdlib.h"

#include <string>
#include <iostream>

namespace { // anonymous namespace for file scope.
  //
  // Default values.
  //
  enum { DEFAULT_DOMAINID =  OpenDDS::DCPS::MONITOR_DOMAIN_ID};

  // Command line argument definitions.
  const ACE_TCHAR* VERBOSE_ARGUMENT = ACE_TEXT("-v");

} // end of anonymous namespace.

Monitor::Options::~Options()
{
}

bool
Monitor::Options::verbose() const
{
  return this->verbose_;
}

bool
Monitor::Options::configured() const
{
  return this->configured_;
}

long
Monitor::Options::domain() const
{
  return this->domain_;
}

Monitor::Options::Options( int argc, ACE_TCHAR** argv, char** /* envp */)
 : verbose_(           false),
   configured_(        false),
   domain_( DEFAULT_DOMAINID)
{
  ACE_Arg_Shifter parser( argc, argv);
  while( parser.is_anything_left()) {
    if( this->verbose()) {
      ACE_DEBUG((LM_DEBUG,
        ACE_TEXT("(%P|%t) Options::Options() - ")
        ACE_TEXT("processing argument: %s.\n"),
        parser.get_current()
      ));
    }
    // const ACE_TCHAR* currentArg = 0;

    if( 0 <= (parser.cur_arg_strncasecmp( VERBOSE_ARGUMENT))) {
      this->verbose_ = true;
      if( this->verbose()) {
        ACE_DEBUG((LM_DEBUG,
          ACE_TEXT("(%P|%t) Options::Options() - ")
          ACE_TEXT("Setting VERBOSE mode.\n")
        ));
      }
      parser.consume_arg();

    } else {
      parser.ignore_arg();
    }
  }
}


