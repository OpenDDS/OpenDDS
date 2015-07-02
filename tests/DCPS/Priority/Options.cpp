// -*- C++ -*-
//

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
  const Test::Options::TransportType
                      DEFAULT_TRANSPORT_TYPE = Test::Options::TCP;
  const unsigned int  DEFAULT_TRANSPORT_KEY  =   1;
  const long          DEFAULT_PUBLISHER_ID   =   1;
  const char*         DEFAULT_TEST_TOPICNAME = "TestTopic";

  // Map command line arguments to transport types and keys.
  const struct TransportTypeArgMappings {
    std::basic_string<ACE_TCHAR> optionName;
    std::pair< Test::Options::TransportType, unsigned int>
                transportInfo;

  } transportTypeArgMappings[] = {
    { ACE_TEXT("tcp"), std::make_pair( Test::Options::TCP, 1U) }, // [transport_impl_1]
    { ACE_TEXT("udp"), std::make_pair( Test::Options::UDP, 2U) }, // [transport_impl_2]
    { ACE_TEXT("mc"),  std::make_pair( Test::Options::MC,  3U) }  // [transport_impl_3]
  };

} // end of anonymous namespace.

namespace Test {

// Command line argument definitions.
const ACE_TCHAR* Options::TRANSPORT_TYPE_ARGUMENT = ACE_TEXT("-t");
const ACE_TCHAR* Options::PUBLISHER_ID_ARGUMENT   = ACE_TEXT("-i");
const ACE_TCHAR* Options::VERBOSE_ARGUMENT        = ACE_TEXT("-v");
const ACE_TCHAR* Options::PRIORITY_ARGUMENT       = ACE_TEXT("-p");
const ACE_TCHAR* Options::MULTI_ARGUMENT          = ACE_TEXT("-m");

Options::~Options()
{
}

Options::Options( int argc, ACE_TCHAR** argv, char** /* envp */)
 : verbose_(          false),
   domain_(           DEFAULT_TEST_DOMAIN),
   priority_(         DEFAULT_TEST_PRIORITY),
   transportType_(    DEFAULT_TRANSPORT_TYPE),
   transportKey_(     DEFAULT_TRANSPORT_KEY),
   publisherId_(      DEFAULT_PUBLISHER_ID),
   topicName_(        DEFAULT_TEST_TOPICNAME),
   multipleInstances_(false)
{
  ACE_Arg_Shifter parser( argc, argv);
  while( parser.is_anything_left()) {
    const ACE_TCHAR* currentArg = 0;
    if( 0 != (currentArg = parser.get_the_parameter( TRANSPORT_TYPE_ARGUMENT))) {
      this->transportType_ = TRANSPORT_NONE;
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
      if( this->transportType_ == TRANSPORT_NONE) {
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

    } else if( 0 != (currentArg = parser.get_the_parameter( PUBLISHER_ID_ARGUMENT))) {
      this->publisherId_ = ACE_OS::atoi( currentArg);
      parser.consume_arg();

    } else if( 0 <= (parser.cur_arg_strncasecmp( MULTI_ARGUMENT))) {
      this->multipleInstances_ = true;
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

    default:
    case Test::Options::TRANSPORT_NONE: return str << "NONE";
  }
}

} // End of namespace Test

