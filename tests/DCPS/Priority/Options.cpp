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

  const unsigned long DEFAULT_TEST_DOMAIN    = 521;
  const unsigned long DEFAULT_TEST_PRIORITY  =   0;
  const unsigned int  DEFAULT_SAMPLE_COUNT   =  10;
  const Test::Options::TransportType
                      DEFAULT_TRANSPORT_TYPE = Test::Options::TCP;
  const unsigned int  DEFAULT_TRANSPORT_KEY  =   1;
  const long          DEFAULT_PUBLISHER_ID   =   1;
  const char*         DEFAULT_TEST_TOPICNAME = "TestTopic";

  // Map command line arguments to transport types and keys.
  const struct TransportTypeArgMappings {
    std::string optionName;
    std::pair< Test::Options::TransportType, unsigned int>
                transportInfo;

  } transportTypeArgMappings[] = {
    { "tcp", std::make_pair( Test::Options::TCP, 1) }, // [transport_impl_1]
    { "udp", std::make_pair( Test::Options::UDP, 2) }, // [transport_impl_2]
    { "mc",  std::make_pair( Test::Options::MC,  3) }, // [transport_impl_3]
    { "rmc", std::make_pair( Test::Options::RMC, 4) }  // [transport_impl_4]
  };

} // end of anonymous namespace.

namespace Test {

// Command line argument definitions.
const char* Options::TRANSPORT_TYPE_ARGUMENT = "-t";
const char* Options::PUBLISHER_ID_ARGUMENT   = "-i";
const char* Options::VERBOSE_ARGUMENT        = "-v";
const char* Options::PRIORITY_ARGUMENT       = "-p";
const char* Options::COUNT_ARGUMENT          = "-c";

Options::~Options()
{
}

Options::Options( int argc, char** argv, char** /* envp */)
 : verbose_(       false),
   domain_(        DEFAULT_TEST_DOMAIN),
   priority_(      DEFAULT_TEST_PRIORITY),
   count_(         DEFAULT_SAMPLE_COUNT),
   transportType_( DEFAULT_TRANSPORT_TYPE),
   transportKey_(  DEFAULT_TRANSPORT_KEY),
   publisherId_(   DEFAULT_PUBLISHER_ID),
   topicName_(     DEFAULT_TEST_TOPICNAME)
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
          this->transportType_ = transportTypeArgMappings[ index].transportInfo.first;
          this->transportKey_  = transportTypeArgMappings[ index].transportInfo.second;
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

    } else if( 0 != (currentArg = parser.get_the_parameter( PRIORITY_ARGUMENT))) {
      this->priority_ = ACE_OS::atoi( currentArg);
      parser.consume_arg();

    } else if( 0 != (currentArg = parser.get_the_parameter( COUNT_ARGUMENT))) {
      this->count_ = ACE_OS::atoi( currentArg);
      parser.consume_arg();

    } else if( 0 != (currentArg = parser.get_the_parameter( PUBLISHER_ID_ARGUMENT))) {
      this->publisherId_ = ACE_OS::atoi( currentArg);
      parser.consume_arg();

    } else if( 0 <= (parser.cur_arg_strncasecmp( VERBOSE_ARGUMENT))) {
      this->verbose_ = true;
      parser.consume_arg();

    } else {
      parser.ignore_arg();
    }
  }
}

std::ostream&
operator<<( std::ostream& str, Test::Options::TransportType value)
{
  switch( value) {
    case Test::Options::TCP: return str << "TCP";
    case Test::Options::UDP: return str << "UDP";
    case Test::Options::MC:  return str << "MC";
    case Test::Options::RMC: return str << "RMC";

    default:
    case Test::Options::NONE: return str << "NONE";
  }
}

} // End of namespace Test

