// -*- C++ -*-
//

#include "dds/DCPS/debug.h"
#include "Options.h"
#include "PublicationProfile.h"
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
  enum { DEFAULT_ID              =  -1};
  enum { DEFAULT_TEST_DOMAIN     = 521};
  enum { DEFAULT_TEST_DURATION   =  60};
  const char DEFAULT_TRANSPORT_KEY[] = "tcp1";
  enum { DEFAULT_RAW_BUFFER_SIZE = 500};

  // const unsigned long DEFAULT_TEST_DOMAIN    = 521;
  // const unsigned long DEFAULT_TEST_DURATION  =  -1;
  const Test::Options::TransportType
                      DEFAULT_TRANSPORT_TYPE = Test::Options::TCP;
  // const unsigned int  DEFAULT_TRANSPORT_KEY  =   1;
  const char*         DEFAULT_TEST_TOPICNAME = "TestTopic";
  const std::string   DEFAULT_RAW_OUTPUT_FILENAME = std::string();
  const OpenDDS::DCPS::DataCollector< double>::OnFull
                      DEFAULT_RAW_BUFFER_TYPE
                        = OpenDDS::DCPS::DataCollector< double>::KeepNewest;
//                      = OpenDDS::DCPS::DataCollector< double>::KeepOldest;

  // Command line argument definitions.
  const ACE_TCHAR* TRANSPORT_TYPE_ARGUMENT = ACE_TEXT("-t");
  const ACE_TCHAR* VERBOSE_ARGUMENT        = ACE_TEXT("-v");
  const ACE_TCHAR* DURATION_ARGUMENT       = ACE_TEXT("-c");
  const ACE_TCHAR* SCENARIO_ARGUMENT       = ACE_TEXT("-f");
  const ACE_TCHAR* ID_ARGUMENT             = ACE_TEXT("-i");
  const ACE_TCHAR* RAW_DATA_FILENAME_ARGUMENT = ACE_TEXT("-r");

  // Scenario configuration file section names.
  const ACE_TCHAR* PUBLICATION_SECTION_NAME = ACE_TEXT("publication");

  // Scenario configuration file Key values.
  const ACE_TCHAR* TRANSPORT_KEY_NAME = ACE_TEXT("Transport");
  const ACE_TCHAR* DURATION_KEY_NAME  = ACE_TEXT("TestDuration");
  const ACE_TCHAR* PRIORITY_KEY_NAME  = ACE_TEXT("Priority");
  const ACE_TCHAR* MAX_KEY_NAME       = ACE_TEXT("MessageMax");
  const ACE_TCHAR* MIN_KEY_NAME       = ACE_TEXT("MessageMin");
  const ACE_TCHAR* SIZE_KEY_NAME      = ACE_TEXT("MessageSize");
  const ACE_TCHAR* DEVIATION_KEY_NAME = ACE_TEXT("MessageDeviation");
  const ACE_TCHAR* RATE_KEY_NAME      = ACE_TEXT("MessageRate");

  // Scenario configuration default values.
  enum { DEFAULT_PRIORITY  =    0};
  enum { DEFAULT_MAX       = 1450};
  enum { DEFAULT_MIN       =  800};
  enum { DEFAULT_SIZE      = 1000};
  enum { DEFAULT_DEVIATION =  300};
  enum { DEFAULT_RATE      =  100};

  // Map command line arguments to transport types and keys.
  const struct TransportTypeArgMappings {
    ACE_TString optionName;
    std::pair< Test::Options::TransportType, OPENDDS_STRING>
                transportInfo;

  } transportTypeArgMappings[] = {
    { ACE_TEXT("tcp"), std::make_pair( Test::Options::TCP, OPENDDS_STRING("tcp1")) },
    { ACE_TEXT("udp"), std::make_pair( Test::Options::UDP, OPENDDS_STRING("udp1")) },
    { ACE_TEXT("mc"),  std::make_pair( Test::Options::MC,  OPENDDS_STRING("mcast1")) }
  };

} // end of anonymous namespace.

namespace Test {

Options::~Options()
{
  for( unsigned int index = 0; index < this->publicationProfiles_.size(); ++index) {
    delete this->publicationProfiles_[ index];
  }
  this->publicationProfiles_.clear();
}

Options::Options(int argc, ACE_TCHAR** argv, char** /* envp */)
 : verbose_(           false),
   domain_(            DEFAULT_TEST_DOMAIN),
   id_(                DEFAULT_ID),
   duration_(          DEFAULT_TEST_DURATION),
   transportType_(     DEFAULT_TRANSPORT_TYPE),
   transportKey_(      DEFAULT_TRANSPORT_KEY),
   topicName_(         DEFAULT_TEST_TOPICNAME),
   rawOutputFilename_( DEFAULT_RAW_OUTPUT_FILENAME),
   raw_buffer_size_(   DEFAULT_RAW_BUFFER_SIZE),
   raw_buffer_type_(   DEFAULT_RAW_BUFFER_TYPE)
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
          ACE_TEXT("unrecognized transport type on command line: %s, ")
          ACE_TEXT("using default TCP.\n"),
          currentArg
        ));
        this->transportType_ = DEFAULT_TRANSPORT_TYPE;
        this->transportKey_  = DEFAULT_TRANSPORT_KEY;
      }
      parser.consume_arg();

    } else if( 0 != (currentArg = parser.get_the_parameter( DURATION_ARGUMENT))) {
      this->duration_ = ACE_OS::atoi( currentArg);
      parser.consume_arg();

    } else if( 0 != (currentArg = parser.get_the_parameter( ID_ARGUMENT))) {
      this->id_ = ACE_OS::atoi( currentArg);
      parser.consume_arg();

    } else if( 0 != (currentArg = parser.get_the_parameter( SCENARIO_ARGUMENT))) {
      this->configureScenarios( currentArg);
      parser.consume_arg();

    } else if( 0 != (currentArg = parser.get_the_parameter( RAW_DATA_FILENAME_ARGUMENT))) {
      this->rawOutputFilename_ = ACE_TEXT_ALWAYS_CHAR(currentArg);
      parser.consume_arg();

    } else if( 0 <= (parser.cur_arg_strncasecmp( VERBOSE_ARGUMENT))) {
      this->verbose_ = true;
      parser.consume_arg();

    } else {
      ACE_DEBUG((LM_WARNING,
        ACE_TEXT("(%P|%t) WARNING: Options::Options() - ")
        ACE_TEXT("ignoring argument: %s.\n"),
        parser.get_current()
      ));
      parser.ignore_arg();
    }
  }
}

void
Options::configureScenarios(const ACE_TCHAR* filename)
{
  if( this->verbose()) {
    ACE_DEBUG((LM_DEBUG,
      ACE_TEXT("(%P|%t) Options::configureScenarios() - ")
      ACE_TEXT("configuring using file: %s.\n"),
      filename
    ));
  }

  ACE_Configuration_Heap heap;
  if( 0 != heap.open()) {
    ACE_ERROR((LM_ERROR,
      ACE_TEXT("(%P|%t) ERROR: Options::configureScenarios() - ")
      ACE_TEXT("failed to open() configuration heap.\n")
    ));
    return;
  }

  ACE_Ini_ImpExp import( heap);
  if( 0 != import.import_config( filename)) {
    ACE_ERROR((LM_ERROR,
      ACE_TEXT("(%P|%t) ERROR: Options::configureScenarios() - ")
      ACE_TEXT("failed to import configuration file.\n")
    ));
    return;
  }

  // Process common (no section) data here.
  const ACE_Configuration_Section_Key& root = heap.root_section();

  // Transport = tcp | udp | mc | rmc              OPTIONAL
  ACE_TString transportString;
  if( 0 == heap.get_string_value( root, TRANSPORT_KEY_NAME, transportString)) {
    this->transportType_ = TRANSPORT_NONE;
    for( unsigned int index = 0;
         index < sizeof(transportTypeArgMappings)/sizeof(transportTypeArgMappings[0]);
         ++index
       ) {
      if( transportTypeArgMappings[ index].optionName == transportString.c_str()) {
        if( this->verbose()) {
          ACE_DEBUG((LM_DEBUG,
            ACE_TEXT("(%P|%t) Options::configureScenarios() - ")
            ACE_TEXT("setting transport type to: %s.\n"),
            transportString.c_str()
          ));
        }
        this->transportType_ = transportTypeArgMappings[ index].transportInfo.first;
        this->transportKey_  = transportTypeArgMappings[ index].transportInfo.second;
        break;
      }
    }
    if( this->transportType_ == TRANSPORT_NONE) {
      ACE_ERROR((LM_ERROR,
        ACE_TEXT("(%P|%t) ERROR: Options::configureScenarios() - ")
        ACE_TEXT("unrecognized transport type: %s, defaulting to TCP.\n"),
        transportString.c_str()
      ));
      this->transportType_ = DEFAULT_TRANSPORT_TYPE;
      this->transportKey_  = DEFAULT_TRANSPORT_KEY;
    }
  }

  // TestDuration = <seconds>                   OPTIONAL
  ACE_TString durationString;
  if( 0 == heap.get_string_value( root, DURATION_KEY_NAME, durationString)) {
    if( this->verbose()) {
      ACE_DEBUG((LM_DEBUG,
        ACE_TEXT("(%P|%t) Options::configureScenarios() - ")
        ACE_TEXT("setting test duration to: %d.\n"),
        durationString.c_str()
      ));
    }
    this->duration_ = ACE_OS::atoi( durationString.c_str());
  }

  // Find all of the [publication] sections.
  ACE_Configuration_Section_Key publicationKey;
  if( 0 != heap.open_section( root, PUBLICATION_SECTION_NAME, 0, publicationKey)) {
    ACE_ERROR((LM_ERROR,
      ACE_TEXT("(%P|%t) ERROR: Options::configureScenarios() - ")
      ACE_TEXT("failed to find any publication definitions in ")
      ACE_TEXT("scenario definition file.\n")
    ));
    return;
  }

  // Process each [publication] subsection.
  ACE_TString sectionName;
  for( int index = 0;
       (0 == heap.enumerate_sections( publicationKey, index, sectionName));
       ++index
     ) {
    if( this->verbose()) {
      ACE_DEBUG((LM_DEBUG,
        ACE_TEXT("(%P|%t) Options::configureScenarios() - ")
        ACE_TEXT("configuring publication %s.\n"),
        sectionName.c_str()
      ));
    }

    ACE_Configuration_Section_Key sectionKey;
    if( 0 != heap.open_section( publicationKey, sectionName.c_str(), 0, sectionKey)) {
      ACE_ERROR((LM_ERROR,
        ACE_TEXT("(%P|%t) ERROR: Options::configureScenarios() - ")
        ACE_TEXT("unable to open section %s, skipping.\n"),
        sectionName.c_str()
      ));
      continue;
    }

    // Priority = <number>
    ACE_TString priorityString;
    unsigned long priority = DEFAULT_PRIORITY;
    if( 0 == heap.get_string_value( sectionKey, PRIORITY_KEY_NAME, priorityString)) {
      priority = ACE_OS::atoi( priorityString.c_str());
    }

    // MessageMax = <bytes_per_message>
    ACE_TString maxString;
    unsigned long max = DEFAULT_MAX;
    if( 0 == heap.get_string_value( sectionKey, MAX_KEY_NAME, maxString)) {
      max = ACE_OS::atoi( maxString.c_str());
    }

    // MessageMin = <bytes_per_message>
    ACE_TString minString;
    unsigned long min = DEFAULT_MIN;
    if( 0 == heap.get_string_value( sectionKey, MIN_KEY_NAME, minString)) {
      min = ACE_OS::atoi( minString.c_str());
    }

    // MessageSize = <bytes_per_message>
    ACE_TString sizeString;
    unsigned long size = DEFAULT_SIZE;
    if( 0 == heap.get_string_value( sectionKey, SIZE_KEY_NAME, sizeString)) {
      size = ACE_OS::atoi( sizeString.c_str());
    }

    // MessageDeviation = <number>
    ACE_TString deviationString;
    unsigned long deviation = DEFAULT_DEVIATION;
    if( 0 == heap.get_string_value( sectionKey, DEVIATION_KEY_NAME, deviationString)) {
      deviation = ACE_OS::atoi( deviationString.c_str());
    }

    // MessageRate = <messages_per_second>
    ACE_TString rateString;
    unsigned long rate = DEFAULT_RATE;
    if( 0 == heap.get_string_value( sectionKey, RATE_KEY_NAME, rateString)) {
      rate = ACE_OS::atoi( rateString.c_str());
    }

    // Store the profile for the current publication.
    this->publicationProfiles_.push_back(
      new PublicationProfile(
            ACE_TEXT_ALWAYS_CHAR(sectionName.c_str()),
            priority,
            rate,
            size,
            deviation,
            max,
            min
          )
    );

  }
}

} // End of namespace Test

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


