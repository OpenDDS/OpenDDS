// -*- C++ -*-
//
// $Id$

#include "dds/DCPS/debug.h"
#include "Options.h"
#include "ace/Arg_Shifter.h"
#include "ace/Log_Priority.h"
#include "ace/Log_Msg.h"
#include "ace/OS_NS_stdlib.h"

#if !defined (__ACE_INLINE__)
# include "Options.inl"
#endif /* ! __ACE_INLINE__ */

#include <string>
#include <iostream>

namespace { // anonymous namespace for file scope.
  //
  // Default values.
  //
  const Options::TransportType DEFAULT_TRANSPORT_TYPE = Options::TCP;

  const struct TransportTypeArgMappings {
    std::string            optionName;
    Options::TransportType transportType;

  } transportTypeArgMappings[] = {
    { "tcp", Options::TCP },
    { "udp", Options::UDP },
    { "mc",  Options::MC },
    { "rmc", Options::RMC }
  };

} // end of anonymous namespace.

// Command line argument definitions.
const char* Options::TRANSPORT_TYPE_ARGUMENT = "-t";
const char* Options::PUBLISHER_ID_ARGUMENT   = "-i";

Options::~Options()
{
}

Options::Options( int argc, char** argv, char** /* envp */)
 : transportType_( DEFAULT_TRANSPORT_TYPE)
{
  ACE_Arg_Shifter parser( argc, argv);
  while( parser.is_anything_left()) {
    const char* currentArg = 0;
    if( 0 != (currentArg = parser.get_the_parameter( TRANSPORT_TYPE_ARGUMENT))) {
      this->transportType_ = NONE;
      for( unsigned int index = 0;
           index < sizeof(transportTypeArgMappings)/sizeof(transportTypeArgMappings[0]);
           ++index
         ) {
        if( transportTypeArgMappings[ index].optionName == currentArg) {
          this->transportType_ = transportTypeArgMappings[ index].transportType;
          break;
        }
      }
      if( this->transportType_ == NONE) {
        ACE_ERROR((LM_ERROR,
          ACE_TEXT("(%P|%t) ERROR: Options::Options() - ")
          ACE_TEXT("unrecognized transport type: %s.\n"),
          currentArg
        ));
        this->transportType_ = TCP;
      }
      parser.consume_arg();

    } else if( 0 != (currentArg = parser.get_the_parameter( PUBLISHER_ID_ARGUMENT))) {
      this->publisherId_ = ACE_OS::atoi( currentArg);
      parser.consume_arg();

    } else {
      parser.ignore_arg();
    }
  }
}

std::ostream&
operator<<( std::ostream& str, Options::TransportType value)
{
  switch( value) {
    case Options::TCP: return str << "TCP";
    case Options::UDP: return str << "UDP";
    case Options::MC:  return str << "MC";
    case Options::RMC: return str << "RMC";

    default:
    case Options::NONE: return str << "NONE";
  }
}

