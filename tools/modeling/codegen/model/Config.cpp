// -*- C++ -*-
//

#include "dds/DCPS/debug.h"
#include "EntityProfiles.h"
#include "Config.h"
#include "ace/Arg_Shifter.h"
#include "ace/Log_Priority.h"
#include "ace/Log_Msg.h"
#include "ace/OS_NS_stdlib.h"
#include "ace/Configuration.h"
#include "ace/Configuration_Import_Export.h"

#if !defined (__ACE_INLINE__)
# include "Config.inl"
#endif /* ! __ACE_INLINE__ */

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace { // anonymous namespace for file scope.
  //
  // Default values.
  //
  enum { DEFAULT_DOMAINID =  0};

  // Command line argument definitions.
  const ACE_TCHAR* ENTITYFILE_ARGUMENT = ACE_TEXT("-DDSAppConfig");

  // Entity configuration file section names.
  const ACE_TCHAR* PARTICIPANT_SECTIONNAME  = ACE_TEXT("participant");
  const ACE_TCHAR* TOPIC_SECTIONNAME        = ACE_TEXT("topic");
  const ACE_TCHAR* PUBLISHER_SECTIONNAME    = ACE_TEXT("publisher");
  const ACE_TCHAR* WRITER_SECTIONNAME       = ACE_TEXT("writer");
  const ACE_TCHAR* SUBSCRIBER_SECTIONNAME   = ACE_TEXT("subscriber");
  const ACE_TCHAR* READER_SECTIONNAME       = ACE_TEXT("reader");

  // Entity configuration file Key values.
  const ACE_TCHAR* DOMAINID_KEYNAME                            = ACE_TEXT("DomainId");
  const ACE_TCHAR* USERDATA_KEYNAME                            = ACE_TEXT("UserData");
  const ACE_TCHAR* ENTITYFACTORY_KEYNAME                       = ACE_TEXT("EntityFactory");
  const ACE_TCHAR* TOPICDATA_KEYNAME                           = ACE_TEXT("TopicData");
  const ACE_TCHAR* DURABILITY_KEYNAME                          = ACE_TEXT("Durability");
  const ACE_TCHAR* DURABILITYSERVICEDURATION_KEYNAME           = ACE_TEXT("DurabilityServiceDuration");
  const ACE_TCHAR* DURABILITYSERVICEHISTORYKIND_KEYNAME        = ACE_TEXT("DurabilityServiceHistoryKind");
  const ACE_TCHAR* DURABILITYSERVICEHISTORYDEPTH_KEYNAME       = ACE_TEXT("DurabilityServiceHistoryDepth");
  const ACE_TCHAR* DURABILITYSERVICESAMPLES_KEYNAME            = ACE_TEXT("DurabilityServiceSamples");
  const ACE_TCHAR* DURABILITYSERVICEINSTANCES_KEYNAME          = ACE_TEXT("DurabilityServiceInstances");
  const ACE_TCHAR* DURABILITYSERVICESAMPLESPERINSTANCE_KEYNAME = ACE_TEXT("DurabilityServiceSamplesPerInstance");
  const ACE_TCHAR* DEADLINE_KEYNAME                            = ACE_TEXT("Deadline");
  const ACE_TCHAR* LATENCYBUDGET_KEYNAME                       = ACE_TEXT("LatencyBudget");
  const ACE_TCHAR* LIVELINESSKIND_KEYNAME                      = ACE_TEXT("LivelinessKind");
  const ACE_TCHAR* LIVELINESSDURATION_KEYNAME                  = ACE_TEXT("LivelinessDuration");
  const ACE_TCHAR* RELIABILITYKIND_KEYNAME                     = ACE_TEXT("ReliabilityKind");
  const ACE_TCHAR* RELIABILITYMAXBLOCKING_KEYNAME              = ACE_TEXT("ReliabilityMaxBlocking");
  const ACE_TCHAR* DESTINATIONORDER_KEYNAME                    = ACE_TEXT("DestinationOrder");
  const ACE_TCHAR* HISTORYKIND_KEYNAME                         = ACE_TEXT("HistoryKind");
  const ACE_TCHAR* HISTORYDEPTH_KEYNAME                        = ACE_TEXT("HistoryDepth");
  const ACE_TCHAR* RESOURCEMAXSAMPLES_KEYNAME                  = ACE_TEXT("ResourceMaxSamples");
  const ACE_TCHAR* RESOURCEMAXINSTANCES_KEYNAME                = ACE_TEXT("ResourceMaxInstances");
  const ACE_TCHAR* RESOURCEMAXSAMPLESPERINSTANCE_KEYNAME       = ACE_TEXT("ResourceMaxSamplesPerInstance");
  const ACE_TCHAR* TRANSPORTPRIORITY_KEYNAME                   = ACE_TEXT("TransportPriority");
  const ACE_TCHAR* LIFESPANDURATION_KEYNAME                    = ACE_TEXT("LifespanDuration");
  const ACE_TCHAR* OWNERSHIPKIND_KEYNAME                       = ACE_TEXT("OwnershipKind");
#ifndef OPENDDS_NO_OWNERSHIP_KIND_EXCLUSIVE
  const ACE_TCHAR* OWNERSHIPSTRENGTH_KEYNAME                   = ACE_TEXT("OwnershipStrength");
#endif
  const ACE_TCHAR* PRESENTATION_KEYNAME                        = ACE_TEXT("Presentation");
  const ACE_TCHAR* PRESENTATIONCOHERENT_KEYNAME                = ACE_TEXT("PresentationCoherent");
  const ACE_TCHAR* PRESENTATIONORDERED_KEYNAME                 = ACE_TEXT("PresentationOrdered");
  const ACE_TCHAR* GROUPDATA_KEYNAME                           = ACE_TEXT("GroupData");
  const ACE_TCHAR* PARTITION_KEYNAME                           = ACE_TEXT("Partition");
  const ACE_TCHAR* TIMEBASEDFILTER_KEYNAME                     = ACE_TEXT("TimeBasedFilter");
  const ACE_TCHAR* WRITERDATALIFECYCLE_KEYNAME                 = ACE_TEXT("WriterDataLifecycle");
  const ACE_TCHAR* READERDATALIFECYCLE_KEYNAME                 = ACE_TEXT("ReaderDataLifecycle");

  const ACE_TCHAR* PARTICIPANT_KEYNAME                         = ACE_TEXT("Participant");
  const ACE_TCHAR* TOPIC_KEYNAME                               = ACE_TEXT("Topic");
  const ACE_TCHAR* PUBLISHER_KEYNAME                           = ACE_TEXT("Publisher");
  const ACE_TCHAR* SUBSCRIBER_KEYNAME                          = ACE_TEXT("Subscriber");
  const ACE_TCHAR* TYPE_KEYNAME                                = ACE_TEXT("Type");
  const ACE_TCHAR* TRANSPORTINDEX_KEYNAME                      = ACE_TEXT("TransportIndex");

} // end of anonymous namespace.

namespace OpenDDS { namespace Model {

Config::~Config()
{
  for( ParticipantProfileMap::iterator current = this->participantProfileMap_.begin();
       current != this->participantProfileMap_.end();
       ++current
     ) {
    delete current->second;
  }
  this->participantProfileMap_.clear();

  for( TopicProfileMap::iterator current = this->topicProfileMap_.begin();
       current != this->topicProfileMap_.end();
       ++current
     ) {
    delete current->second;
  }
  this->topicProfileMap_.clear();

  for( PublisherProfileMap::iterator current = this->publisherProfileMap_.begin();
       current != this->publisherProfileMap_.end();
       ++current
     ) {
    delete current->second;
  }
  this->publisherProfileMap_.clear();

  for( WriterProfileMap::iterator current = this->writerProfileMap_.begin();
       current != this->writerProfileMap_.end();
       ++current
     ) {
    delete current->second;
  }
  this->writerProfileMap_.clear();

  for( SubscriberProfileMap::iterator current = this->subscriberProfileMap_.begin();
       current != this->subscriberProfileMap_.end();
       ++current
     ) {
    delete current->second;
  }
  this->subscriberProfileMap_.clear();

  for( ReaderProfileMap::iterator current = this->readerProfileMap_.begin();
       current != this->readerProfileMap_.end();
       ++current
     ) {
    delete current->second;
  }
  this->readerProfileMap_.clear();
}

Config::Config(int& argc, ACE_TCHAR** argv)
  : configured_(true)
{
  ACE_Arg_Shifter parser( argc, argv);
  while( parser.is_anything_left()) {
    if( OpenDDS::DCPS::DCPS_debug_level>1) {
      ACE_DEBUG((LM_DEBUG,
        ACE_TEXT("(%P|%t) Config::Config() - ")
        ACE_TEXT("processing argument: %s.\n"),
        parser.get_current()
      ));
    }
    const ACE_TCHAR* currentArg = 0;
    if( 0 != (currentArg = parser.get_the_parameter( ENTITYFILE_ARGUMENT))) {
      this->configureEntities( currentArg);
      parser.consume_arg();

    } else {
      parser.ignore_arg();
    }
  }
}

void
Config::configureEntities( const ACE_TCHAR* filename)
{
  if( OpenDDS::DCPS::DCPS_debug_level>1) {
    ACE_DEBUG((LM_DEBUG,
      ACE_TEXT("(%P|%t) Config::configureEntities() - ")
      ACE_TEXT("configuring using file: %s.\n"),
      filename
    ));
  }

  ACE_Configuration_Heap heap;
  if( 0 != heap.open()) {
    ACE_ERROR((LM_ERROR,
      ACE_TEXT("(%P|%t) ERROR: Config::configureEntities() - ")
      ACE_TEXT("failed to open() configuration heap.\n")
    ));
    this->configured_ = false;
    return;
  }

  ACE_Ini_ImpExp import( heap);
  if( 0 != import.import_config( filename)) {
    ACE_ERROR((LM_ERROR,
      ACE_TEXT("(%P|%t) ERROR: Config::configureEntities() - ")
      ACE_TEXT("failed to import configuration file.\n")
    ));
    this->configured_ = false;
    return;
  }

  // Find all of the [participant] sections.
  ACE_Configuration_Section_Key participantKey;
  if( 0 == heap.open_section( heap.root_section(), PARTICIPANT_SECTIONNAME, 0, participantKey)) {
    // Process each [participant] subsection.
    ACE_TString sectionName;
    for( int index = 0;
         (0 == heap.enumerate_sections( participantKey, index, sectionName));
         ++index
       ) {
      if( OpenDDS::DCPS::DCPS_debug_level>1) {
        ACE_DEBUG((LM_DEBUG,
          ACE_TEXT("(%P|%t) Config::configureEntities() - ")
          ACE_TEXT("configuring participant %s.\n"),
          sectionName.c_str()
        ));
      }

      ACE_Configuration_Section_Key sectionKey;
      if( 0 == heap.open_section( participantKey, sectionName.c_str(), 0, sectionKey)) {
        loadParticipant( heap, sectionKey, sectionName.c_str());

      } else {
        ACE_ERROR((LM_ERROR,
          ACE_TEXT("(%P|%t) ERROR: Config::configureEntities() - ")
          ACE_TEXT("unable to open section %s, skipping.\n"),
          sectionName.c_str()
        ));
        this->configured_ = false;
      }
    }

  } else {
    ACE_DEBUG((LM_DEBUG,
      ACE_TEXT("(%P|%t) Config::configureEntities() - ")
      ACE_TEXT("failed to find any participant definitions in ")
      ACE_TEXT("scenario definition file.\n")
    ));
    this->configured_ = false;
  }

  // Find all of the [topic] sections.
  ACE_Configuration_Section_Key topicKey;
  if( 0 == heap.open_section( heap.root_section(), TOPIC_SECTIONNAME, 0, topicKey)) {
    // Process each [topic] subsection.
    ACE_TString sectionName;
    for( int index = 0;
         (0 == heap.enumerate_sections( topicKey, index, sectionName));
         ++index
       ) {
      if( OpenDDS::DCPS::DCPS_debug_level>1) {
        ACE_DEBUG((LM_DEBUG,
          ACE_TEXT("(%P|%t) Config::configureEntities() - ")
          ACE_TEXT("configuring topic %s.\n"),
          sectionName.c_str()
        ));
      }

      ACE_Configuration_Section_Key sectionKey;
      if( 0 == heap.open_section( topicKey, sectionName.c_str(), 0, sectionKey)) {
        loadTopic( heap, sectionKey, sectionName.c_str());

      } else {
        ACE_ERROR((LM_ERROR,
          ACE_TEXT("(%P|%t) ERROR: Config::configureEntities() - ")
          ACE_TEXT("unable to open section %s, skipping.\n"),
          sectionName.c_str()
        ));
        this->configured_ = false;
      }
    }

  } else {
    ACE_DEBUG((LM_DEBUG,
      ACE_TEXT("(%P|%t) Config::configureEntities() - ")
      ACE_TEXT("failed to find any topic definitions in ")
      ACE_TEXT("scenario definition file.\n")
    ));
    this->configured_ = false;
  }

  // Find all of the [publisher] sections.
  ACE_Configuration_Section_Key publisherKey;
  if( 0 == heap.open_section( heap.root_section(), PUBLISHER_SECTIONNAME, 0, publisherKey)) {
    // Process each [publisher] subsection.
    ACE_TString sectionName;
    for( int index = 0;
         (0 == heap.enumerate_sections( publisherKey, index, sectionName));
         ++index
       ) {
      if( OpenDDS::DCPS::DCPS_debug_level>1) {
        ACE_DEBUG((LM_DEBUG,
          ACE_TEXT("(%P|%t) Config::configureEntities() - ")
          ACE_TEXT("configuring publisher %s.\n"),
          sectionName.c_str()
        ));
      }

      ACE_Configuration_Section_Key sectionKey;
      if( 0 == heap.open_section( publisherKey, sectionName.c_str(), 0, sectionKey)) {
        loadPublisher( heap, sectionKey, sectionName.c_str());

      } else {
        ACE_ERROR((LM_ERROR,
          ACE_TEXT("(%P|%t) ERROR: Config::configureEntities() - ")
          ACE_TEXT("unable to open section %s, skipping.\n"),
          sectionName.c_str()
        ));
        this->configured_ = false;
      }
    }
  }

  // Find all of the [writer] sections.
  ACE_Configuration_Section_Key writerKey;
  if( 0 == heap.open_section( heap.root_section(), WRITER_SECTIONNAME, 0, writerKey)) {
    // Process each [writer] subsection.
    ACE_TString sectionName;
    for( int index = 0;
         (0 == heap.enumerate_sections( writerKey, index, sectionName));
         ++index
       ) {
      if( OpenDDS::DCPS::DCPS_debug_level>1) {
        ACE_DEBUG((LM_DEBUG,
          ACE_TEXT("(%P|%t) Config::configureEntities() - ")
          ACE_TEXT("configuring writer %s.\n"),
          sectionName.c_str()
        ));
      }

      ACE_Configuration_Section_Key sectionKey;
      if( 0 == heap.open_section( writerKey, sectionName.c_str(), 0, sectionKey)) {
        loadWriter( heap, sectionKey, sectionName.c_str());

      } else {
        ACE_ERROR((LM_ERROR,
          ACE_TEXT("(%P|%t) ERROR: Config::configureEntities() - ")
          ACE_TEXT("unable to open section %s, skipping.\n"),
          sectionName.c_str()
        ));
        this->configured_ = false;
      }
    }
  }

  // Find all of the [subscriber] sections.
  ACE_Configuration_Section_Key subscriberKey;
  if( 0 == heap.open_section( heap.root_section(), SUBSCRIBER_SECTIONNAME, 0, subscriberKey)) {
    // Process each [subscriber] subsection.
    ACE_TString sectionName;
    for( int index = 0;
         (0 == heap.enumerate_sections( subscriberKey, index, sectionName));
         ++index
       ) {
      if( OpenDDS::DCPS::DCPS_debug_level>1) {
        ACE_DEBUG((LM_DEBUG,
          ACE_TEXT("(%P|%t) Config::configureEntities() - ")
          ACE_TEXT("configuring subscriber %s.\n"),
          sectionName.c_str()
        ));
      }

      ACE_Configuration_Section_Key sectionKey;
      if( 0 == heap.open_section( subscriberKey, sectionName.c_str(), 0, sectionKey)) {
        loadSubscriber( heap, sectionKey, sectionName.c_str());

      } else {
        ACE_ERROR((LM_ERROR,
          ACE_TEXT("(%P|%t) ERROR: Config::configureEntities() - ")
          ACE_TEXT("unable to open section %s, skipping.\n"),
          sectionName.c_str()
        ));
      }
    }
  }

  // Find all of the [reader] sections.
  ACE_Configuration_Section_Key readerKey;
  if( 0 == heap.open_section( heap.root_section(), READER_SECTIONNAME, 0, readerKey)) {
    // Process each [reader] subsection.
    ACE_TString sectionName;
    for( int index = 0;
         (0 == heap.enumerate_sections( readerKey, index, sectionName));
         ++index
       ) {
      if( OpenDDS::DCPS::DCPS_debug_level>1) {
        ACE_DEBUG((LM_DEBUG,
          ACE_TEXT("(%P|%t) Config::configureEntities() - ")
          ACE_TEXT("configuring reader %s.\n"),
          sectionName.c_str()
        ));
      }

      ACE_Configuration_Section_Key sectionKey;
      if( 0 == heap.open_section( readerKey, sectionName.c_str(), 0, sectionKey)) {
        loadReader( heap, sectionKey, sectionName.c_str());

      } else {
        ACE_ERROR((LM_ERROR,
          ACE_TEXT("(%P|%t) ERROR: Config::configureEntities() - ")
          ACE_TEXT("unable to open section %s, skipping.\n"),
          sectionName.c_str()
        ));
      }
    }
  }

}

void
Config::loadParticipant(
  ACE_Configuration_Heap& heap,
  ACE_Configuration_Section_Key& sectionKey,
  std::basic_string<ACE_TCHAR> sectionName
)
{
  /**
   * [participant/<name>]
   *   # Participant Qos Policy values
   *   UserData      = <string>
   *   EntityFactory = <bool>
   *
   *   DomainId      = <number>
   */

  // Note that this requires that the Service Participant already be
  // initialized before we configure from the scenario file.  Also,
  // since we have not created any Entities yet, we go to the initial
  // default values rather than to the containing Entity.
  ParticipantProfile* profile = new ParticipantProfile();
  profile->qos = TheServiceParticipant->initial_DomainParticipantQos();
  ACE_TString valueString;

  // DomainId = <number>
  profile->domainId = DEFAULT_DOMAINID;
  if( 0 == heap.get_string_value( sectionKey, DOMAINID_KEYNAME, valueString)) {
    profile->domainId = ACE_OS::atoi( valueString.c_str());
  }
  if( OpenDDS::DCPS::DCPS_debug_level>1) {
    ACE_DEBUG((LM_DEBUG,
      ACE_TEXT("(%P|%t) Config::loadParticipant() - ")
      ACE_TEXT("  [participant/%s] %s == %d.\n"),
      sectionName.c_str(),
      DOMAINID_KEYNAME,
      profile->domainId
    ));
  }

  // UserData      = <string>     OPTIONAL
  valueString.clear();
  heap.get_string_value( sectionKey, USERDATA_KEYNAME, valueString);
  if (valueString.length() > 0) {
    profile->qos.user_data.value.replace(
      static_cast<CORBA::ULong>(valueString.length()),
      static_cast<CORBA::ULong>(valueString.length()),
      const_cast<CORBA::Octet*>(
        reinterpret_cast<const CORBA::Octet*>( valueString.c_str())
      )
    );
    if( OpenDDS::DCPS::DCPS_debug_level>1) {
      ACE_DEBUG((LM_DEBUG,
        ACE_TEXT("(%P|%t) Config::loadParticipant() - ")
        ACE_TEXT("  [participant/%s] %s == %s.\n"),
        sectionName.c_str(),
        USERDATA_KEYNAME,
        valueString.c_str()
      ));
    }
  }

  // EntityFactory = <bool>       OPTIONAL
  valueString.clear();
  if( 0 == heap.get_string_value( sectionKey, ENTITYFACTORY_KEYNAME, valueString)) {
    profile->qos.entity_factory.autoenable_created_entities
      = ACE_OS::atoi( valueString.c_str());
    if( OpenDDS::DCPS::DCPS_debug_level>1) {
      ACE_DEBUG((LM_DEBUG,
        ACE_TEXT("(%P|%t) Config::loadParticipant() - ")
        ACE_TEXT("  [participant/%s] %s == %d.\n"),
        sectionName.c_str(),
        ENTITYFACTORY_KEYNAME,
        profile->qos.entity_factory.autoenable_created_entities
      ));
    }
  }

  // Store the profile for the current participant.
  this->participantProfileMap_[ACE_TEXT_ALWAYS_CHAR(sectionName.c_str())] = profile;
}

void
Config::loadTopic(
  ACE_Configuration_Heap& heap,
  ACE_Configuration_Section_Key& sectionKey,
  std::basic_string<ACE_TCHAR> sectionName
)
{
  /**
   * [topic/<name>]
   *   # Topic Qos Policy values
   *   TopicData                           = <string>
   *   Durability                          = <string> # One of VOLATILE, LOCAL, TRANSIENT, PERSISTENT
   *   DurabilityServiceDuration           = <number>
   *   DurabilityServiceHistoryKind        = <string> # One of ALL, LAST
   *   DurabilityServiceHistoryDepth       = <number>
   *   DurabilityServiceSamples            = <number>
   *   DurabilityServiceInstances          = <number>
   *   DurabilityServiceSamplesPerInstance = <number>
   *   Deadline                            = <number>
   *   LatencyBudget                       = <number>
   *   LivelinessKind                      = <string> # One of AUTOMATIC, PARTICIPANT, TOPIC
   *   LivelinessDuration                  = <number>
   *   ReliabilityKind                     = <string> # One of BEST_EFFORT, RELIABLE
   *   ReliabilityMaxBlocking              = <number>
   *   DestinationOrder                    = <string> # One of SOURCE, RECEPTION
   *   HistoryKind                         = <string> # One of ALL, LAST
   *   HistoryDepth                        = <number>
   *   ResourceMaxSamples                  = <number>
   *   ResourceMaxInstances                = <number>
   *   ResourceMaxSamplesPerInstance       = <number>
   *   TransportPriority                   = <number>
   *   LifespanDuration                    = <number>
   *   OwnershipKind                       = <string> # One of SHARED, EXCLUSIVE
   *
   *   Participant                         = <string> # One of participant <name>
   *   Type                                = <string> # Name for a registered datatype.
   *   Topic                               = <string> # Name for topic
   */

  // Note that this requires that the Service Participant already be
  // initialized before we configure from the file.  Also, since we have
  // not created any Entities yet, we go to the initial default values
  // rather than to the containing Entity.
  TopicProfile* profile = new TopicProfile();
  profile->qos = TheServiceParticipant->initial_TopicQos();
  ACE_TString valueString;

  // TopicData                           = <string>
  heap.get_string_value( sectionKey, TOPICDATA_KEYNAME, valueString);
  if (valueString.length() > 0) {
    profile->qos.topic_data.value.replace(
      static_cast<CORBA::ULong>(valueString.length()),
      static_cast<CORBA::ULong>(valueString.length()),
      const_cast<CORBA::Octet*>(
        reinterpret_cast<const CORBA::Octet*>( valueString.c_str())
      )
    );
    if( OpenDDS::DCPS::DCPS_debug_level>1) {
      ACE_DEBUG((LM_DEBUG,
        ACE_TEXT("(%P|%t) Config::loadTopic() - ")
        ACE_TEXT("  [topic/%s] %s == %s.\n"),
        sectionName.c_str(),
        TOPICDATA_KEYNAME,
        valueString.c_str()
      ));
    }
  }

  // Durability = <string> # One of VOLATILE, LOCAL, TRANSIENT, PERSISTENT
  valueString.clear();
  heap.get_string_value( sectionKey, DURABILITY_KEYNAME, valueString);
  if (valueString.length() > 0) {
    if( OpenDDS::DCPS::DCPS_debug_level>1) {
      ACE_DEBUG((LM_DEBUG,
        ACE_TEXT("(%P|%t) Config::loadTopic() - ")
        ACE_TEXT("  [topic/%s] %s == %s.\n"),
        sectionName.c_str(),
        DURABILITY_KEYNAME,
        valueString.c_str()
      ));
    }
    if( valueString == ACE_TEXT("VOLATILE")) {
      profile->qos.durability.kind = ::DDS::VOLATILE_DURABILITY_QOS;

    } else if( valueString == ACE_TEXT("LOCAL")) {
      profile->qos.durability.kind = ::DDS::TRANSIENT_LOCAL_DURABILITY_QOS;

    } else if( valueString == ACE_TEXT("TRANSIENT")) {
      profile->qos.durability.kind = ::DDS::TRANSIENT_DURABILITY_QOS;

    } else if( valueString == ACE_TEXT("PERSISTENT")) {
      profile->qos.durability.kind = ::DDS::PERSISTENT_DURABILITY_QOS;

    } else {
      ACE_DEBUG((LM_WARNING,
        ACE_TEXT("(%P|%t) loadTopic() - ")
        ACE_TEXT("unrecognized value for %s: %s - ")
        ACE_TEXT("not assigning a value.\n"),
        DURABILITY_KEYNAME,
        valueString.c_str()
      ));
    }
  }

  // DurabilityServiceDuration           = <number>
  valueString.clear();
  heap.get_string_value( sectionKey, DURABILITYSERVICEDURATION_KEYNAME, valueString);
  if (valueString.length() > 0) {
    profile->qos.durability_service.service_cleanup_delay.nanosec = 0;
    profile->qos.durability_service.service_cleanup_delay.sec
      = ACE_OS::atoi( valueString.c_str());
    if( OpenDDS::DCPS::DCPS_debug_level>1) {
      ACE_DEBUG((LM_DEBUG,
        ACE_TEXT("(%P|%t) Config::loadTopic() - ")
        ACE_TEXT("  [topic/%s] %s == %d.\n"),
        sectionName.c_str(),
        DURABILITYSERVICEDURATION_KEYNAME,
        profile->qos.durability_service.service_cleanup_delay.sec
      ));
    }
  }

  // DurabilityServiceHistoryKind        = <string> # One of ALL, LAST
  valueString.clear();
  heap.get_string_value( sectionKey, DURABILITYSERVICEHISTORYKIND_KEYNAME, valueString);
  if (valueString.length() > 0) {
    if( OpenDDS::DCPS::DCPS_debug_level>1) {
      ACE_DEBUG((LM_DEBUG,
        ACE_TEXT("(%P|%t) Config::loadTopic() - ")
        ACE_TEXT("  [topic/%s] %s == %s.\n"),
        sectionName.c_str(),
        DURABILITYSERVICEHISTORYKIND_KEYNAME,
        valueString.c_str()
      ));
    }
    if( valueString == ACE_TEXT("ALL")) {
      profile->qos.durability_service.history_kind = ::DDS::KEEP_ALL_HISTORY_QOS;

    } else if( valueString == ACE_TEXT("LAST")) {
      profile->qos.durability_service.history_kind = ::DDS::KEEP_LAST_HISTORY_QOS;

    } else {
      ACE_DEBUG((LM_WARNING,
        ACE_TEXT("(%P|%t) loadTopic() - ")
        ACE_TEXT("unrecognized value for %s: %s - ")
        ACE_TEXT("not assigning a value.\n"),
        DURABILITYSERVICEHISTORYKIND_KEYNAME,
        valueString.c_str()
      ));
    }
  }

  // DurabilityServiceHistoryDepth       = <number>
  valueString.clear();
  heap.get_string_value( sectionKey, DURABILITYSERVICEHISTORYDEPTH_KEYNAME, valueString);
  if (valueString.length() > 0) {
    profile->qos.durability_service.history_depth = ACE_OS::atoi( valueString.c_str());
    if( OpenDDS::DCPS::DCPS_debug_level>1) {
      ACE_DEBUG((LM_DEBUG,
        ACE_TEXT("(%P|%t) Config::loadTopic() - ")
        ACE_TEXT("  [topic/%s] %s == %d.\n"),
        sectionName.c_str(),
        DURABILITYSERVICEHISTORYDEPTH_KEYNAME,
        profile->qos.durability_service.history_depth
      ));
    }
  }

  // DurabilityServiceSamples            = <number>
  valueString.clear();
  heap.get_string_value( sectionKey, DURABILITYSERVICESAMPLES_KEYNAME, valueString);
  if (valueString.length() > 0) {
    profile->qos.durability_service.max_samples = ACE_OS::atoi( valueString.c_str());
    if( OpenDDS::DCPS::DCPS_debug_level>1) {
      ACE_DEBUG((LM_DEBUG,
        ACE_TEXT("(%P|%t) Config::loadTopic() - ")
        ACE_TEXT("  [topic/%s] %s == %d.\n"),
        sectionName.c_str(),
        DURABILITYSERVICESAMPLES_KEYNAME,
        profile->qos.durability_service.max_samples
      ));
    }
  }

  // DurabilityServiceInstances          = <number>
  valueString.clear();
  heap.get_string_value( sectionKey, DURABILITYSERVICEINSTANCES_KEYNAME, valueString);
  if (valueString.length() > 0) {
    profile->qos.durability_service.max_instances = ACE_OS::atoi( valueString.c_str());
    if( OpenDDS::DCPS::DCPS_debug_level>1) {
      ACE_DEBUG((LM_DEBUG,
        ACE_TEXT("(%P|%t) Config::loadTopic() - ")
        ACE_TEXT("  [topic/%s] %s == %d.\n"),
        sectionName.c_str(),
        DURABILITYSERVICEINSTANCES_KEYNAME,
        profile->qos.durability_service.max_instances
      ));
    }
  }

  // DurabilityServiceSamplesPerInstance = <number>
  valueString.clear();
  heap.get_string_value( sectionKey, DURABILITYSERVICESAMPLESPERINSTANCE_KEYNAME, valueString);
  if (valueString.length() > 0) {
    profile->qos.durability_service.max_samples_per_instance
      = ACE_OS::atoi( valueString.c_str());
    if( OpenDDS::DCPS::DCPS_debug_level>1) {
      ACE_DEBUG((LM_DEBUG,
        ACE_TEXT("(%P|%t) Config::loadTopic() - ")
        ACE_TEXT("  [topic/%s] %s == %s.\n"),
        sectionName.c_str(),
        DURABILITYSERVICESAMPLESPERINSTANCE_KEYNAME,
        profile->qos.durability_service.max_samples_per_instance
      ));
    }
  }

  // Deadline                            = <number>
  valueString.clear();
  heap.get_string_value( sectionKey, DEADLINE_KEYNAME, valueString);
  if (valueString.length() > 0) {
    profile->qos.deadline.period.nanosec = 0;
    profile->qos.deadline.period.sec = ACE_OS::atoi( valueString.c_str());
    if( OpenDDS::DCPS::DCPS_debug_level>1) {
      ACE_DEBUG((LM_DEBUG,
        ACE_TEXT("(%P|%t) Config::loadTopic() - ")
        ACE_TEXT("  [topic/%s] %s == %d.\n"),
        sectionName.c_str(),
        DEADLINE_KEYNAME,
        profile->qos.deadline.period.sec
      ));
    }
  }

  // LatencyBudget                       = <number>
  valueString.clear();
  heap.get_string_value( sectionKey, LATENCYBUDGET_KEYNAME, valueString);
  if (valueString.length() > 0) {
    profile->qos.latency_budget.duration.nanosec = 0;
    profile->qos.latency_budget.duration.sec = ACE_OS::atoi( valueString.c_str());
    if( OpenDDS::DCPS::DCPS_debug_level>1) {
      ACE_DEBUG((LM_DEBUG,
        ACE_TEXT("(%P|%t) Config::loadTopic() - ")
        ACE_TEXT("  [topic/%s] %s == %d.\n"),
        sectionName.c_str(),
        LATENCYBUDGET_KEYNAME,
        profile->qos.latency_budget.duration.sec
      ));
    }
  }

  // LivelinessKind                      = <string> # One of AUTOMATIC, PARTICIPANT, TOPIC
  valueString.clear();
  heap.get_string_value( sectionKey, LIVELINESSKIND_KEYNAME, valueString);
  if (valueString.length() > 0) {
    if( OpenDDS::DCPS::DCPS_debug_level>1) {
      ACE_DEBUG((LM_DEBUG,
        ACE_TEXT("(%P|%t) Config::loadTopic() - ")
        ACE_TEXT("  [topic/%s] %s == %s.\n"),
        sectionName.c_str(),
        LIVELINESSKIND_KEYNAME,
        valueString.c_str()
      ));
    }
    if( valueString == ACE_TEXT("AUTOMATIC")) {
      profile->qos.liveliness.kind = ::DDS::AUTOMATIC_LIVELINESS_QOS;

    } else if( valueString == ACE_TEXT("PARTICIPANT")) {
      profile->qos.liveliness.kind = ::DDS::MANUAL_BY_PARTICIPANT_LIVELINESS_QOS;

    } else if( valueString == ACE_TEXT("TOPIC")) {
      profile->qos.liveliness.kind = ::DDS::MANUAL_BY_TOPIC_LIVELINESS_QOS;

    } else {
      ACE_DEBUG((LM_WARNING,
        ACE_TEXT("(%P|%t) loadTopic() - ")
        ACE_TEXT("unrecognized value for %s: %s - ")
        ACE_TEXT("not assigning a value.\n"),
        LIVELINESSKIND_KEYNAME,
        valueString.c_str()
      ));
    }
  }

  // LivelinessDuration                  = <number>
  valueString.clear();
  heap.get_string_value( sectionKey, LIVELINESSDURATION_KEYNAME, valueString);
  if (valueString.length() > 0) {
    profile->qos.liveliness.lease_duration.nanosec = 0;
    profile->qos.liveliness.lease_duration.sec = ACE_OS::atoi( valueString.c_str());
    if( OpenDDS::DCPS::DCPS_debug_level>1) {
      ACE_DEBUG((LM_DEBUG,
        ACE_TEXT("(%P|%t) Config::loadTopic() - ")
        ACE_TEXT("  [topic/%s] %s == %d.\n"),
        sectionName.c_str(),
        LIVELINESSDURATION_KEYNAME,
        profile->qos.liveliness.lease_duration.sec
      ));
    }
  }

  // ReliabilityKind                     = <string> # One of BEST_EFFORT, RELIABLE
  valueString.clear();
  heap.get_string_value( sectionKey, RELIABILITYKIND_KEYNAME, valueString);
  if (valueString.length() > 0) {
    if( OpenDDS::DCPS::DCPS_debug_level>1) {
      ACE_DEBUG((LM_DEBUG,
        ACE_TEXT("(%P|%t) Config::loadTopic() - ")
        ACE_TEXT("  [topic/%s] %s == %s.\n"),
        sectionName.c_str(),
        RELIABILITYKIND_KEYNAME,
        valueString.c_str()
      ));
    }
    if( valueString == ACE_TEXT("BEST_EFFORT")) {
      profile->qos.reliability.kind = ::DDS::BEST_EFFORT_RELIABILITY_QOS;

    } else if( valueString == ACE_TEXT("RELIABLE")) {
      profile->qos.reliability.kind = ::DDS::RELIABLE_RELIABILITY_QOS;

    } else {
      ACE_DEBUG((LM_WARNING,
        ACE_TEXT("(%P|%t) loadTopic() - ")
        ACE_TEXT("unrecognized value for %s: %s - ")
        ACE_TEXT("not assigning a value.\n"),
        RELIABILITYKIND_KEYNAME,
        valueString.c_str()
      ));
    }
  }

  // ReliabilityMaxBlocking              = <number>
  valueString.clear();
  heap.get_string_value( sectionKey, RELIABILITYMAXBLOCKING_KEYNAME, valueString);
  if (valueString.length() > 0) {
    profile->qos.reliability.max_blocking_time.nanosec = 0;
    profile->qos.reliability.max_blocking_time.sec = ACE_OS::atoi( valueString.c_str());
    if( OpenDDS::DCPS::DCPS_debug_level>1) {
      ACE_DEBUG((LM_DEBUG,
        ACE_TEXT("(%P|%t) Config::loadTopic() - ")
        ACE_TEXT("  [topic/%s] %s == %d.\n"),
        sectionName.c_str(),
        RELIABILITYMAXBLOCKING_KEYNAME,
        profile->qos.reliability.max_blocking_time.sec
      ));
    }
  }

  // DestinationOrder                    = <string> # One of SOURCE, RECEPTION
  valueString.clear();
  heap.get_string_value( sectionKey, DESTINATIONORDER_KEYNAME, valueString);
  if (valueString.length() > 0) {
    if( OpenDDS::DCPS::DCPS_debug_level>1) {
      ACE_DEBUG((LM_DEBUG,
        ACE_TEXT("(%P|%t) Config::loadTopic() - ")
        ACE_TEXT("  [topic/%s] %s == %s.\n"),
        sectionName.c_str(),
        DESTINATIONORDER_KEYNAME,
        valueString.c_str()
      ));
    }
    if( valueString == ACE_TEXT("SOURCE")) {
      profile->qos.destination_order.kind = ::DDS::BY_SOURCE_TIMESTAMP_DESTINATIONORDER_QOS;

    } else if( valueString == ACE_TEXT("RECEPTION")) {
      profile->qos.destination_order.kind = ::DDS::BY_RECEPTION_TIMESTAMP_DESTINATIONORDER_QOS;

    } else {
      ACE_DEBUG((LM_WARNING,
        ACE_TEXT("(%P|%t) loadTopic() - ")
        ACE_TEXT("unrecognized value for %s: %s - ")
        ACE_TEXT("not assigning a value.\n"),
        DESTINATIONORDER_KEYNAME,
        valueString.c_str()
      ));
    }
  }

  // HistoryKind                         = <string> # One of ALL, LAST
  valueString.clear();
  heap.get_string_value( sectionKey, HISTORYKIND_KEYNAME, valueString);
  if (valueString.length() > 0) {
    if( OpenDDS::DCPS::DCPS_debug_level>1) {
      ACE_DEBUG((LM_DEBUG,
        ACE_TEXT("(%P|%t) Config::loadTopic() - ")
        ACE_TEXT("  [topic/%s] %s == %s.\n"),
        sectionName.c_str(),
        HISTORYKIND_KEYNAME,
        valueString.c_str()
      ));
    }
    if( valueString == ACE_TEXT("ALL")) {
      profile->qos.history.kind = ::DDS::KEEP_ALL_HISTORY_QOS;

    } else if( valueString == ACE_TEXT("LAST")) {
      profile->qos.history.kind = ::DDS::KEEP_LAST_HISTORY_QOS;

    } else {
      ACE_DEBUG((LM_WARNING,
        ACE_TEXT("(%P|%t) loadTopic() - ")
        ACE_TEXT("unrecognized value for %s: %s - ")
        ACE_TEXT("not assigning a value.\n"),
        HISTORYKIND_KEYNAME,
        valueString.c_str()
      ));
    }
  }

  // HistoryDepth                        = <number>
  valueString.clear();
  heap.get_string_value( sectionKey, HISTORYDEPTH_KEYNAME, valueString);
  if (valueString.length() > 0) {
    profile->qos.history.depth = ACE_OS::atoi( valueString.c_str());
    if( OpenDDS::DCPS::DCPS_debug_level>1) {
      ACE_DEBUG((LM_DEBUG,
        ACE_TEXT("(%P|%t) Config::loadTopic() - ")
        ACE_TEXT("  [topic/%s] %s == %d.\n"),
        sectionName.c_str(),
        HISTORYDEPTH_KEYNAME,
        profile->qos.history.depth
      ));
    }
  }

  // ResourceMaxSamples                  = <number>
  valueString.clear();
  heap.get_string_value( sectionKey, RESOURCEMAXSAMPLES_KEYNAME, valueString);
  if (valueString.length() > 0) {
    profile->qos.resource_limits.max_samples = ACE_OS::atoi( valueString.c_str());
    if( OpenDDS::DCPS::DCPS_debug_level>1) {
      ACE_DEBUG((LM_DEBUG,
        ACE_TEXT("(%P|%t) Config::loadTopic() - ")
        ACE_TEXT("  [topic/%s] %s == %d.\n"),
        sectionName.c_str(),
        RESOURCEMAXSAMPLES_KEYNAME,
        profile->qos.resource_limits.max_samples
      ));
    }
  }

  // ResourceMaxInstances                = <number>
  valueString.clear();
  heap.get_string_value( sectionKey, RESOURCEMAXINSTANCES_KEYNAME, valueString);
  if (valueString.length() > 0) {
    profile->qos.resource_limits.max_instances = ACE_OS::atoi( valueString.c_str());
    if( OpenDDS::DCPS::DCPS_debug_level>1) {
      ACE_DEBUG((LM_DEBUG,
        ACE_TEXT("(%P|%t) Config::loadTopic() - ")
        ACE_TEXT("  [topic/%s] %s == %d.\n"),
        sectionName.c_str(),
        RESOURCEMAXINSTANCES_KEYNAME,
        profile->qos.resource_limits.max_instances
      ));
    }
  }

  // ResourceMaxSamplesPerInstance       = <number>
  valueString.clear();
  heap.get_string_value( sectionKey, RESOURCEMAXSAMPLESPERINSTANCE_KEYNAME, valueString);
  if (valueString.length() > 0) {
    profile->qos.resource_limits.max_samples_per_instance = ACE_OS::atoi( valueString.c_str());
    if( OpenDDS::DCPS::DCPS_debug_level>1) {
      ACE_DEBUG((LM_DEBUG,
        ACE_TEXT("(%P|%t) Config::loadTopic() - ")
        ACE_TEXT("  [topic/%s] %s == %d.\n"),
        sectionName.c_str(),
        RESOURCEMAXSAMPLESPERINSTANCE_KEYNAME,
        profile->qos.resource_limits.max_samples_per_instance
      ));
    }
  }

  // TransportPriority                   = <number>
  valueString.clear();
  heap.get_string_value( sectionKey, TRANSPORTPRIORITY_KEYNAME, valueString);
  if (valueString.length() > 0) {
    profile->qos.transport_priority.value = ACE_OS::atoi( valueString.c_str());
    if( OpenDDS::DCPS::DCPS_debug_level>1) {
      ACE_DEBUG((LM_DEBUG,
        ACE_TEXT("(%P|%t) Config::loadTopic() - ")
        ACE_TEXT("  [topic/%s] %s == %d.\n"),
        sectionName.c_str(),
        TRANSPORTPRIORITY_KEYNAME,
        profile->qos.transport_priority.value
      ));
    }
  }

  // LifespanDuration                    = <number>
  valueString.clear();
  heap.get_string_value( sectionKey, LIFESPANDURATION_KEYNAME, valueString);
  if (valueString.length() > 0) {
    profile->qos.lifespan.duration.nanosec = 0;
    profile->qos.lifespan.duration.sec = ACE_OS::atoi( valueString.c_str());
    if( OpenDDS::DCPS::DCPS_debug_level>1) {
      ACE_DEBUG((LM_DEBUG,
        ACE_TEXT("(%P|%t) Config::loadTopic() - ")
        ACE_TEXT("  [topic/%s] %s == %d.\n"),
        sectionName.c_str(),
        LIFESPANDURATION_KEYNAME,
        profile->qos.lifespan.duration.sec
      ));
    }
  }

  // OwnershipKind                       = <string> # One of SHARED, EXCLUSIVE
  valueString.clear();
  heap.get_string_value( sectionKey, OWNERSHIPKIND_KEYNAME, valueString);
  if (valueString.length() > 0) {
    if( OpenDDS::DCPS::DCPS_debug_level>1) {
      ACE_DEBUG((LM_DEBUG,
        ACE_TEXT("(%P|%t) Config::loadTopic() - ")
        ACE_TEXT("  [topic/%s] %s == %s.\n"),
        sectionName.c_str(),
        OWNERSHIPKIND_KEYNAME,
        valueString.c_str()
      ));
    }
    if( valueString == ACE_TEXT("SHARED")) {
      profile->qos.ownership.kind = ::DDS::SHARED_OWNERSHIP_QOS;

#ifndef OPENDDS_NO_OWNERSHIP_KIND_EXCLUSIVE
    } else if( valueString == ACE_TEXT("EXCLUSIVE")) {
      profile->qos.ownership.kind = ::DDS::EXCLUSIVE_OWNERSHIP_QOS;
#endif

    } else {
      ACE_DEBUG((LM_WARNING,
        ACE_TEXT("(%P|%t) loadTopic() - ")
        ACE_TEXT("unrecognized value for %s: %s - ")
        ACE_TEXT("not assigning a value.\n"),
        OWNERSHIPKIND_KEYNAME,
        valueString.c_str()
      ));
    }
  }

  // Participant      = <string> # One of participant <name>
  valueString.clear();
  heap.get_string_value( sectionKey, PARTICIPANT_KEYNAME, valueString);
  if (valueString.length() > 0) {
    profile->participant = ACE_TEXT_ALWAYS_CHAR(valueString.c_str());
    if( OpenDDS::DCPS::DCPS_debug_level>1) {
      ACE_DEBUG((LM_DEBUG,
        ACE_TEXT("(%P|%t) Config::loadTopic() - ")
        ACE_TEXT("  [topic/%s] %s == %s.\n"),
        sectionName.c_str(),
        PARTICIPANT_KEYNAME,
        profile->participant.c_str()
      ));
    }
  }

  // Type             = <string> # Name for a registered datatype.
  valueString.clear();
  heap.get_string_value( sectionKey, TYPE_KEYNAME, valueString);
  if (valueString.length() > 0) {
    profile->type = ACE_TEXT_ALWAYS_CHAR(valueString.c_str());
    if( OpenDDS::DCPS::DCPS_debug_level>1) {
      ACE_DEBUG((LM_DEBUG,
        ACE_TEXT("(%P|%t) Config::loadTopic() - ")
        ACE_TEXT("  [topic/%s] %s == %s.\n"),
        sectionName.c_str(),
        TYPE_KEYNAME,
        profile->type.c_str()
      ));
    }
  }

  // Store the profile for the current participant.
  this->topicProfileMap_[ACE_TEXT_ALWAYS_CHAR(sectionName.c_str())] = profile;
}

void
Config::loadPublisher(
  ACE_Configuration_Heap& heap,
  ACE_Configuration_Section_Key& sectionKey,
  std::basic_string<ACE_TCHAR> sectionName
)
{
  /**
   * [publisher/<name>]
   *   # Publisher Qos Policy values
   *   Presentation                        = <string> # One of INSTANCE, TOPIC, GROUP
   *   PresentationCoherent                = <bool> # Boolean: numeric 0 or 1
   *   PresentationOrdered                 = <bool> # Boolean: numeric 0 or 1
   *   Partition                           = <string> # Only single value supported
   *   GroupData                           = <string>
   *   EntityFactory                       = <bool> # Boolean: numeric 0 or 1
   *
   *   Participant       = <string> # One of participant <name>
   *   TransportIndex    = <number> # Index into transport configurations
   */

  // Note that this requires that the Service Participant already be
  // initialized before we configure from the file.  Also, since we have
  // not created any Entities yet, we go to the initial default values
  // rather than to the containing Entity.
  PublisherProfile* profile = new PublisherProfile();
  profile->qos = TheServiceParticipant->initial_PublisherQos();
  ACE_TString valueString;

  // Presentation                        = <string> # One of INSTANCE, TOPIC, GROUP
  valueString.clear();
  heap.get_string_value( sectionKey, PRESENTATION_KEYNAME, valueString);
  if (valueString.length() > 0) {
    if( OpenDDS::DCPS::DCPS_debug_level>1) {
      ACE_DEBUG((LM_DEBUG,
        ACE_TEXT("(%P|%t) Config::loadPublisher() - ")
        ACE_TEXT("  [publisher/%s] %s == %s.\n"),
        sectionName.c_str(),
        PRESENTATION_KEYNAME,
        valueString.c_str()
      ));
    }
    if( valueString == ACE_TEXT("INSTANCE")) {
      profile->qos.presentation.access_scope = ::DDS::INSTANCE_PRESENTATION_QOS;

    } else if( valueString == ACE_TEXT("TOPIC")) {
      profile->qos.presentation.access_scope = ::DDS::TOPIC_PRESENTATION_QOS;

    } else if( valueString == ACE_TEXT("GROUP")) {
      profile->qos.presentation.access_scope = ::DDS::GROUP_PRESENTATION_QOS;

    } else {
      ACE_DEBUG((LM_WARNING,
        ACE_TEXT("(%P|%t) loadPublisher() - ")
        ACE_TEXT("unrecognized value for %s: %s - ")
        ACE_TEXT("not assigning a value.\n"),
        PRESENTATION_KEYNAME,
        valueString.c_str()
      ));
    }
  }

  // PresentationCoherent                = <bool> # Boolean: numeric 0 or 1
  valueString.clear();
  if( 0 == heap.get_string_value( sectionKey, PRESENTATIONCOHERENT_KEYNAME, valueString)) {
    profile->qos.presentation.coherent_access
      = ACE_OS::atoi( valueString.c_str());
    if( OpenDDS::DCPS::DCPS_debug_level>1) {
      ACE_DEBUG((LM_DEBUG,
        ACE_TEXT("(%P|%t) Config::loadPublisher() - ")
        ACE_TEXT("  [publisher/%s] %s == %d.\n"),
        sectionName.c_str(),
        PRESENTATIONCOHERENT_KEYNAME,
        profile->qos.presentation.coherent_access
      ));
    }
  }

  // PresentationOrdered                 = <bool> # Boolean: numeric 0 or 1
  valueString.clear();
  if( 0 == heap.get_string_value( sectionKey, PRESENTATIONORDERED_KEYNAME, valueString)) {
    profile->qos.presentation.ordered_access
      = ACE_OS::atoi( valueString.c_str());
    if( OpenDDS::DCPS::DCPS_debug_level>1) {
      ACE_DEBUG((LM_DEBUG,
        ACE_TEXT("(%P|%t) Config::loadPublisher() - ")
        ACE_TEXT("  [publisher/%s] %s == %d.\n"),
        sectionName.c_str(),
        PRESENTATIONORDERED_KEYNAME,
        profile->qos.presentation.ordered_access
      ));
    }
  }

  // Partition                           = <string> # Only single value supported
  valueString.clear();
  heap.get_string_value( sectionKey, PARTITION_KEYNAME, valueString);
  if (valueString.length() > 0) {
    profile->qos.partition.name.length( 1);
    profile->qos.partition.name[0] = ACE_TEXT_ALWAYS_CHAR(valueString.c_str());
    if( OpenDDS::DCPS::DCPS_debug_level>1) {
      ACE_DEBUG((LM_DEBUG,
        ACE_TEXT("(%P|%t) Config::loadPublisher() - ")
        ACE_TEXT("  [publisher/%s] %s == %s.\n"),
        sectionName.c_str(),
        PARTITION_KEYNAME,
        valueString.c_str()
      ));
    }
  }

  // GroupData                           = <string>
  valueString.clear();
  heap.get_string_value( sectionKey, GROUPDATA_KEYNAME, valueString);
  if (valueString.length() > 0) {
    profile->qos.group_data.value.replace(
      static_cast<CORBA::ULong>(valueString.length()),
      static_cast<CORBA::ULong>(valueString.length()),
      const_cast<CORBA::Octet*>(
        reinterpret_cast<const CORBA::Octet*>( valueString.c_str())
      )
    );
    if( OpenDDS::DCPS::DCPS_debug_level>1) {
      ACE_DEBUG((LM_DEBUG,
        ACE_TEXT("(%P|%t) Config::loadPublisher() - ")
        ACE_TEXT("  [publisher/%s] %s == %s.\n"),
        sectionName.c_str(),
        GROUPDATA_KEYNAME,
        valueString.c_str()
      ));
    }
  }

  // EntityFactory                       = <bool> # Boolean: numeric 0 or 1
  valueString.clear();
  if( 0 == heap.get_string_value( sectionKey, ENTITYFACTORY_KEYNAME, valueString)) {
    profile->qos.entity_factory.autoenable_created_entities
      = ACE_OS::atoi( valueString.c_str());
    if( OpenDDS::DCPS::DCPS_debug_level>1) {
      ACE_DEBUG((LM_DEBUG,
        ACE_TEXT("(%P|%t) Config::loadPublisher() - ")
        ACE_TEXT("  [publisher/%s] %s == %d.\n"),
        sectionName.c_str(),
        ENTITYFACTORY_KEYNAME,
        profile->qos.entity_factory.autoenable_created_entities
      ));
    }
  }

  // Participant      = <string> # One of participant <name>
  valueString.clear();
  heap.get_string_value( sectionKey, PARTICIPANT_KEYNAME, valueString);
  if (valueString.length() > 0) {
    profile->participant = ACE_TEXT_ALWAYS_CHAR(valueString.c_str());
    if( OpenDDS::DCPS::DCPS_debug_level>1) {
      ACE_DEBUG((LM_DEBUG,
        ACE_TEXT("(%P|%t) Config::loadPublisher() - ")
        ACE_TEXT("  [publisher/%s] %s == %s.\n"),
        sectionName.c_str(),
        PARTICIPANT_KEYNAME,
        profile->participant.c_str()
      ));
    }
  }

  // TransportIndex   = <number> # Index into transport configurations
  valueString.clear();
  heap.get_string_value( sectionKey, TRANSPORTINDEX_KEYNAME, valueString);
  if (valueString.length() > 0) {
    profile->transport = ACE_OS::atoi( valueString.c_str());
    if( OpenDDS::DCPS::DCPS_debug_level>1) {
      ACE_DEBUG((LM_DEBUG,
        ACE_TEXT("(%P|%t) Config::loadPublisher() - ")
        ACE_TEXT("  [publisher/%s] %s == %d.\n"),
        sectionName.c_str(),
        TRANSPORTINDEX_KEYNAME,
        profile->transport
      ));
    }
  }

  // Store the profile for the current participant.
  this->publisherProfileMap_[ACE_TEXT_ALWAYS_CHAR(sectionName.c_str())] = profile;
}

void
Config::loadWriter(
  ACE_Configuration_Heap& heap,
  ACE_Configuration_Section_Key& sectionKey,
  std::basic_string<ACE_TCHAR> sectionName
)
{
  /**
   * [writer/<name>]
   *   # DataWriter Qos Policy values
   *   Durability                          = <string> # One of VOLATILE, LOCAL, TRANSIENT, PERSISTENT
   *   DurabilityServiceDuration           = <number>
   *   DurabilityServiceHistoryKind        = <string> # One of ALL, LAST
   *   DurabilityServiceHistoryDepth       = <number>
   *   DurabilityServiceSamples            = <number>
   *   DurabilityServiceInstances          = <number>
   *   DurabilityServiceSamplesPerInstance = <number>
   *   Deadline                            = <number>
   *   LatencyBudget                       = <number>
   *   LivelinessKind                      = <string> # One of AUTOMATIC, PARTICIPANT, TOPIC
   *   LivelinessDuration                  = <number>
   *   ReliabilityKind                     = <string> # One of BEST_EFFORT, RELIABLE
   *   ReliabilityMaxBlocking              = <number>
   *   DestinationOrder                    = <string> # One of SOURCE, RECEPTION
   *   HistoryKind                         = <string> # One of ALL, LAST
   *   HistoryDepth                        = <number>
   *   ResourceMaxSamples                  = <number>
   *   ResourceMaxInstances                = <number>
   *   ResourceMaxSamplesPerInstance       = <number>
   *   TransportPriority                   = <number>
   *   Lifespan                            = <number>
   *   UserData                            = <string>
   *   OwnershipKind                       = <string> # One of SHARED, EXCLUSIVE
   *   OwnershipStrength                   = <number>
   *   WriterDataLifecycle                 = <bool> # Boolean: numeric 0 or 1
   *
   *   Publisher         = <string> # One of publisher <name>
   *   Topic             = <string> # One of topic <name>
   */

  // Note that this requires that the Service Participant already be
  // initialized before we configure from the file.  Also, since we have
  // not created any Entities yet, we go to the initial default values
  // rather than to the containing Entity.
  WriterProfile* profile = new WriterProfile();
  profile->qos  = TheServiceParticipant->initial_DataWriterQos();
  profile->mask = 0;
  ACE_TString valueString;

  // Durability = <string> # One of VOLATILE, LOCAL, TRANSIENT, PERSISTENT
  valueString.clear();
  heap.get_string_value( sectionKey, DURABILITY_KEYNAME, valueString);
  if (valueString.length() > 0) {
    if( OpenDDS::DCPS::DCPS_debug_level>1) {
      ACE_DEBUG((LM_DEBUG,
        ACE_TEXT("(%P|%t) Config::loadWriter() - ")
        ACE_TEXT("  [writer/%s] %s == %s.\n"),
        sectionName.c_str(),
        DURABILITY_KEYNAME,
        valueString.c_str()
      ));
    }
    if( valueString == ACE_TEXT("VOLATILE")) {
      profile->qos.durability.kind = ::DDS::VOLATILE_DURABILITY_QOS;
      profile->mask |= SetDurabilityQos;

    } else if( valueString == ACE_TEXT("LOCAL")) {
      profile->qos.durability.kind = ::DDS::TRANSIENT_LOCAL_DURABILITY_QOS;
      profile->mask |= SetDurabilityQos;

    } else if( valueString == ACE_TEXT("TRANSIENT")) {
      profile->qos.durability.kind = ::DDS::TRANSIENT_DURABILITY_QOS;
      profile->mask |= SetDurabilityQos;

    } else if( valueString == ACE_TEXT("PERSISTENT")) {
      profile->qos.durability.kind = ::DDS::PERSISTENT_DURABILITY_QOS;
      profile->mask |= SetDurabilityQos;

    } else {
      ACE_DEBUG((LM_WARNING,
        ACE_TEXT("(%P|%t) loadWriter() - ")
        ACE_TEXT("unrecognized value for %s: %s - ")
        ACE_TEXT("not assigning a value.\n"),
        DURABILITY_KEYNAME,
        valueString.c_str()
      ));
    }
  }

  // DurabilityServiceDuration           = <number>
  valueString.clear();
  heap.get_string_value( sectionKey, DURABILITYSERVICEDURATION_KEYNAME, valueString);
  if (valueString.length() > 0) {
    profile->qos.durability_service.service_cleanup_delay.nanosec = 0;
    profile->qos.durability_service.service_cleanup_delay.sec
      = ACE_OS::atoi( valueString.c_str());
    profile->mask |= SetDurabilityServiceDurationQos;
    if( OpenDDS::DCPS::DCPS_debug_level>1) {
      ACE_DEBUG((LM_DEBUG,
        ACE_TEXT("(%P|%t) Config::loadWriter() - ")
        ACE_TEXT("  [writer/%s] %s == %d.\n"),
        sectionName.c_str(),
        DURABILITYSERVICEDURATION_KEYNAME,
        profile->qos.durability_service.service_cleanup_delay.sec
      ));
    }
  }

  // DurabilityServiceHistoryKind        = <string> # One of ALL, LAST
  valueString.clear();
  heap.get_string_value( sectionKey, DURABILITYSERVICEHISTORYKIND_KEYNAME, valueString);
  if (valueString.length() > 0) {
    if( OpenDDS::DCPS::DCPS_debug_level>1) {
      ACE_DEBUG((LM_DEBUG,
        ACE_TEXT("(%P|%t) Config::loadWriter() - ")
        ACE_TEXT("  [writer/%s] %s == %s.\n"),
        sectionName.c_str(),
        DURABILITYSERVICEHISTORYKIND_KEYNAME,
        valueString.c_str()
      ));
    }
    if( valueString == ACE_TEXT("ALL")) {
      profile->qos.durability_service.history_kind = ::DDS::KEEP_ALL_HISTORY_QOS;
      profile->mask |= SetDurabilityServiceHistoryKindQos;

    } else if( valueString == ACE_TEXT("LAST")) {
      profile->qos.durability_service.history_kind = ::DDS::KEEP_LAST_HISTORY_QOS;
      profile->mask |= SetDurabilityServiceHistoryKindQos;

    } else {
      ACE_DEBUG((LM_WARNING,
        ACE_TEXT("(%P|%t) loadWriter() - ")
        ACE_TEXT("unrecognized value for %s: %s - ")
        ACE_TEXT("not assigning a value.\n"),
        DURABILITYSERVICEHISTORYKIND_KEYNAME,
        valueString.c_str()
      ));
    }
  }

  // DurabilityServiceHistoryDepth       = <number>
  valueString.clear();
  heap.get_string_value( sectionKey, DURABILITYSERVICEHISTORYDEPTH_KEYNAME, valueString);
  if (valueString.length() > 0) {
    profile->qos.durability_service.history_depth = ACE_OS::atoi( valueString.c_str());
    profile->mask |= SetDurabilityServiceHistoryDepthQos;
    if( OpenDDS::DCPS::DCPS_debug_level>1) {
      ACE_DEBUG((LM_DEBUG,
        ACE_TEXT("(%P|%t) Config::loadWriter() - ")
        ACE_TEXT("  [writer/%s] %s == %d.\n"),
        sectionName.c_str(),
        DURABILITYSERVICEHISTORYDEPTH_KEYNAME,
        profile->qos.durability_service.history_depth
      ));
    }
  }

  // DurabilityServiceSamples            = <number>
  valueString.clear();
  heap.get_string_value( sectionKey, DURABILITYSERVICESAMPLES_KEYNAME, valueString);
  if (valueString.length() > 0) {
    profile->qos.durability_service.max_samples = ACE_OS::atoi( valueString.c_str());
    profile->mask |= SetDurabilityServiceSamplesQos;
    if( OpenDDS::DCPS::DCPS_debug_level>1) {
      ACE_DEBUG((LM_DEBUG,
        ACE_TEXT("(%P|%t) Config::loadWriter() - ")
        ACE_TEXT("  [writer/%s] %s == %d.\n"),
        sectionName.c_str(),
        DURABILITYSERVICESAMPLES_KEYNAME,
        profile->qos.durability_service.max_samples
      ));
    }
  }

  // DurabilityServiceInstances          = <number>
  valueString.clear();
  heap.get_string_value( sectionKey, DURABILITYSERVICEINSTANCES_KEYNAME, valueString);
  if (valueString.length() > 0) {
    profile->qos.durability_service.max_instances = ACE_OS::atoi( valueString.c_str());
    profile->mask |= SetDurabilityServiceInstancesQos;
    if( OpenDDS::DCPS::DCPS_debug_level>1) {
      ACE_DEBUG((LM_DEBUG,
        ACE_TEXT("(%P|%t) Config::loadWriter() - ")
        ACE_TEXT("  [writer/%s] %s == %d.\n"),
        sectionName.c_str(),
        DURABILITYSERVICEINSTANCES_KEYNAME,
        profile->qos.durability_service.max_instances
      ));
    }
  }

  // DurabilityServiceSamplesPerInstance = <number>
  valueString.clear();
  heap.get_string_value( sectionKey, DURABILITYSERVICESAMPLESPERINSTANCE_KEYNAME, valueString);
  if (valueString.length() > 0) {
    profile->qos.durability_service.max_samples_per_instance
      = ACE_OS::atoi( valueString.c_str());
    profile->mask |= SetDurabilityServiceSamplesPerInstanceQos;
    if( OpenDDS::DCPS::DCPS_debug_level>1) {
      ACE_DEBUG((LM_DEBUG,
        ACE_TEXT("(%P|%t) Config::loadWriter() - ")
        ACE_TEXT("  [writer/%s] %s == %s.\n"),
        sectionName.c_str(),
        DURABILITYSERVICESAMPLESPERINSTANCE_KEYNAME,
        profile->qos.durability_service.max_samples_per_instance
      ));
    }
  }

  // Deadline                            = <number>
  valueString.clear();
  heap.get_string_value( sectionKey, DEADLINE_KEYNAME, valueString);
  if (valueString.length() > 0) {
    profile->qos.deadline.period.nanosec = 0;
    profile->qos.deadline.period.sec = ACE_OS::atoi( valueString.c_str());
    profile->mask |= SetDeadlineQos;
    if( OpenDDS::DCPS::DCPS_debug_level>1) {
      ACE_DEBUG((LM_DEBUG,
        ACE_TEXT("(%P|%t) Config::loadWriter() - ")
        ACE_TEXT("  [writer/%s] %s == %d.\n"),
        sectionName.c_str(),
        DEADLINE_KEYNAME,
        profile->qos.deadline.period.sec
      ));
    }
  }

  // LatencyBudget                       = <number>
  valueString.clear();
  heap.get_string_value( sectionKey, LATENCYBUDGET_KEYNAME, valueString);
  if (valueString.length() > 0) {
    profile->qos.latency_budget.duration.nanosec = 0;
    profile->qos.latency_budget.duration.sec = ACE_OS::atoi( valueString.c_str());
    profile->mask |= SetLatencyBudgetQos;
    if( OpenDDS::DCPS::DCPS_debug_level>1) {
      ACE_DEBUG((LM_DEBUG,
        ACE_TEXT("(%P|%t) Config::loadWriter() - ")
        ACE_TEXT("  [writer/%s] %s == %d.\n"),
        sectionName.c_str(),
        LATENCYBUDGET_KEYNAME,
        profile->qos.latency_budget.duration.sec
      ));
    }
  }

  // LivelinessKind                      = <string> # One of AUTOMATIC, PARTICIPANT, TOPIC
  valueString.clear();
  heap.get_string_value( sectionKey, LIVELINESSKIND_KEYNAME, valueString);
  if (valueString.length() > 0) {
    if( OpenDDS::DCPS::DCPS_debug_level>1) {
      ACE_DEBUG((LM_DEBUG,
        ACE_TEXT("(%P|%t) Config::loadWriter() - ")
        ACE_TEXT("  [writer/%s] %s == %s.\n"),
        sectionName.c_str(),
        LIVELINESSKIND_KEYNAME,
        valueString.c_str()
      ));
    }
    if( valueString == ACE_TEXT("AUTOMATIC")) {
      profile->qos.liveliness.kind = ::DDS::AUTOMATIC_LIVELINESS_QOS;
      profile->mask |= SetLivelinessKindQos;

    } else if( valueString == ACE_TEXT("PARTICIPANT")) {
      profile->qos.liveliness.kind = ::DDS::MANUAL_BY_PARTICIPANT_LIVELINESS_QOS;
      profile->mask |= SetLivelinessKindQos;

    } else if( valueString == ACE_TEXT("TOPIC")) {
      profile->qos.liveliness.kind = ::DDS::MANUAL_BY_TOPIC_LIVELINESS_QOS;
      profile->mask |= SetLivelinessKindQos;

    } else {
      ACE_DEBUG((LM_WARNING,
        ACE_TEXT("(%P|%t) loadWriter() - ")
        ACE_TEXT("unrecognized value for %s: %s - ")
        ACE_TEXT("not assigning a value.\n"),
        LIVELINESSKIND_KEYNAME,
        valueString.c_str()
      ));
    }
  }

  // LivelinessDuration                  = <number>
  valueString.clear();
  heap.get_string_value( sectionKey, LIVELINESSDURATION_KEYNAME, valueString);
  if (valueString.length() > 0) {
    profile->qos.liveliness.lease_duration.nanosec = 0;
    profile->qos.liveliness.lease_duration.sec = ACE_OS::atoi( valueString.c_str());
    profile->mask |= SetLivelinessDurationQos;
    if( OpenDDS::DCPS::DCPS_debug_level>1) {
      ACE_DEBUG((LM_DEBUG,
        ACE_TEXT("(%P|%t) Config::loadWriter() - ")
        ACE_TEXT("  [writer/%s] %s == %d.\n"),
        sectionName.c_str(),
        LIVELINESSDURATION_KEYNAME,
        profile->qos.liveliness.lease_duration.sec
      ));
    }
  }

  // ReliabilityKind                     = <string> # One of BEST_EFFORT, RELIABLE
  valueString.clear();
  heap.get_string_value( sectionKey, RELIABILITYKIND_KEYNAME, valueString);
  if (valueString.length() > 0) {
    if( OpenDDS::DCPS::DCPS_debug_level>1) {
      ACE_DEBUG((LM_DEBUG,
        ACE_TEXT("(%P|%t) Config::loadWriter() - ")
        ACE_TEXT("  [writer/%s] %s == %s.\n"),
        sectionName.c_str(),
        RELIABILITYKIND_KEYNAME,
        valueString.c_str()
      ));
    }
    if( valueString == ACE_TEXT("BEST_EFFORT")) {
      profile->qos.reliability.kind = ::DDS::BEST_EFFORT_RELIABILITY_QOS;
      profile->mask |= SetReliabilityKindQos;

    } else if( valueString == ACE_TEXT("RELIABLE")) {
      profile->qos.reliability.kind = ::DDS::RELIABLE_RELIABILITY_QOS;
      profile->mask |= SetReliabilityKindQos;

    } else {
      ACE_DEBUG((LM_WARNING,
        ACE_TEXT("(%P|%t) loadWriter() - ")
        ACE_TEXT("unrecognized value for %s: %s - ")
        ACE_TEXT("not assigning a value.\n"),
        RELIABILITYKIND_KEYNAME,
        valueString.c_str()
      ));
    }
  }

  // ReliabilityMaxBlocking              = <number>
  valueString.clear();
  heap.get_string_value( sectionKey, RELIABILITYMAXBLOCKING_KEYNAME, valueString);
  if (valueString.length() > 0) {
    profile->qos.reliability.max_blocking_time.nanosec = 0;
    profile->qos.reliability.max_blocking_time.sec = ACE_OS::atoi( valueString.c_str());
    profile->mask |= SetReliabilityMaxBlockingQos;
    if( OpenDDS::DCPS::DCPS_debug_level>1) {
      ACE_DEBUG((LM_DEBUG,
        ACE_TEXT("(%P|%t) Config::loadWriter() - ")
        ACE_TEXT("  [writer/%s] %s == %d.\n"),
        sectionName.c_str(),
        RELIABILITYMAXBLOCKING_KEYNAME,
        profile->qos.reliability.max_blocking_time.sec
      ));
    }
  }

  // DestinationOrder                    = <string> # One of SOURCE, RECEPTION
  valueString.clear();
  heap.get_string_value( sectionKey, DESTINATIONORDER_KEYNAME, valueString);
  if (valueString.length() > 0) {
    if( OpenDDS::DCPS::DCPS_debug_level>1) {
      ACE_DEBUG((LM_DEBUG,
        ACE_TEXT("(%P|%t) Config::loadWriter() - ")
        ACE_TEXT("  [writer/%s] %s == %s.\n"),
        sectionName.c_str(),
        DESTINATIONORDER_KEYNAME,
        valueString.c_str()
      ));
    }
    if( valueString == ACE_TEXT("SOURCE")) {
      profile->qos.destination_order.kind = ::DDS::BY_SOURCE_TIMESTAMP_DESTINATIONORDER_QOS;
      profile->mask |= SetDestinationOrderQos;

    } else if( valueString == ACE_TEXT("RECEPTION")) {
      profile->qos.destination_order.kind = ::DDS::BY_RECEPTION_TIMESTAMP_DESTINATIONORDER_QOS;
      profile->mask |= SetDestinationOrderQos;

    } else {
      ACE_DEBUG((LM_WARNING,
        ACE_TEXT("(%P|%t) loadWriter() - ")
        ACE_TEXT("unrecognized value for %s: %s - ")
        ACE_TEXT("not assigning a value.\n"),
        DESTINATIONORDER_KEYNAME,
        valueString.c_str()
      ));
    }
  }

  // HistoryKind                         = <string> # One of ALL, LAST
  valueString.clear();
  heap.get_string_value( sectionKey, HISTORYKIND_KEYNAME, valueString);
  if (valueString.length() > 0) {
    if( OpenDDS::DCPS::DCPS_debug_level>1) {
      ACE_DEBUG((LM_DEBUG,
        ACE_TEXT("(%P|%t) Config::loadWriter() - ")
        ACE_TEXT("  [writer/%s] %s == %s.\n"),
        sectionName.c_str(),
        HISTORYKIND_KEYNAME,
        valueString.c_str()
      ));
    }
    if( valueString == ACE_TEXT("ALL")) {
      profile->qos.history.kind = ::DDS::KEEP_ALL_HISTORY_QOS;
      profile->mask |= SetHistoryKindQos;

    } else if( valueString == ACE_TEXT("LAST")) {
      profile->qos.history.kind = ::DDS::KEEP_LAST_HISTORY_QOS;
      profile->mask |= SetHistoryKindQos;

    } else {
      ACE_DEBUG((LM_WARNING,
        ACE_TEXT("(%P|%t) loadWriter() - ")
        ACE_TEXT("unrecognized value for %s: %s - ")
        ACE_TEXT("not assigning a value.\n"),
        HISTORYKIND_KEYNAME,
        valueString.c_str()
      ));
    }
  }

  // HistoryDepth                        = <number>
  valueString.clear();
  heap.get_string_value( sectionKey, HISTORYDEPTH_KEYNAME, valueString);
  if (valueString.length() > 0) {
    profile->qos.history.depth = ACE_OS::atoi( valueString.c_str());
    profile->mask |= SetHistoryDepthQos;
    if( OpenDDS::DCPS::DCPS_debug_level>1) {
      ACE_DEBUG((LM_DEBUG,
        ACE_TEXT("(%P|%t) Config::loadWriter() - ")
        ACE_TEXT("  [writer/%s] %s == %d.\n"),
        sectionName.c_str(),
        HISTORYDEPTH_KEYNAME,
        profile->qos.history.depth
      ));
    }
  }

  // ResourceMaxSamples                  = <number>
  valueString.clear();
  heap.get_string_value( sectionKey, RESOURCEMAXSAMPLES_KEYNAME, valueString);
  if (valueString.length() > 0) {
    profile->qos.resource_limits.max_samples = ACE_OS::atoi( valueString.c_str());
    profile->mask |= SetResourceMaxSamplesQos;
    if( OpenDDS::DCPS::DCPS_debug_level>1) {
      ACE_DEBUG((LM_DEBUG,
        ACE_TEXT("(%P|%t) Config::loadWriter() - ")
        ACE_TEXT("  [writer/%s] %s == %d.\n"),
        sectionName.c_str(),
        RESOURCEMAXSAMPLES_KEYNAME,
        profile->qos.resource_limits.max_samples
      ));
    }
  }

  // ResourceMaxInstances                = <number>
  valueString.clear();
  heap.get_string_value( sectionKey, RESOURCEMAXINSTANCES_KEYNAME, valueString);
  if (valueString.length() > 0) {
    profile->qos.resource_limits.max_instances = ACE_OS::atoi( valueString.c_str());
    profile->mask |= SetResourceMaxInstancesQos;
    if( OpenDDS::DCPS::DCPS_debug_level>1) {
      ACE_DEBUG((LM_DEBUG,
        ACE_TEXT("(%P|%t) Config::loadWriter() - ")
        ACE_TEXT("  [writer/%s] %s == %d.\n"),
        sectionName.c_str(),
        RESOURCEMAXINSTANCES_KEYNAME,
        profile->qos.resource_limits.max_instances
      ));
    }
  }

  // ResourceMaxSamplesPerInstance       = <number>
  valueString.clear();
  heap.get_string_value( sectionKey, RESOURCEMAXSAMPLESPERINSTANCE_KEYNAME, valueString);
  if (valueString.length() > 0) {
    profile->qos.resource_limits.max_samples_per_instance = ACE_OS::atoi( valueString.c_str());
    profile->mask |= SetResourceMaxSamplesPerInstanceQos;
    if( OpenDDS::DCPS::DCPS_debug_level>1) {
      ACE_DEBUG((LM_DEBUG,
        ACE_TEXT("(%P|%t) Config::loadWriter() - ")
        ACE_TEXT("  [writer/%s] %s == %d.\n"),
        sectionName.c_str(),
        RESOURCEMAXSAMPLESPERINSTANCE_KEYNAME,
        profile->qos.resource_limits.max_samples_per_instance
      ));
    }
  }

  // TransportPriority                   = <number>
  valueString.clear();
  heap.get_string_value( sectionKey, TRANSPORTPRIORITY_KEYNAME, valueString);
  if (valueString.length() > 0) {
    profile->qos.transport_priority.value = ACE_OS::atoi( valueString.c_str());
    profile->mask |= SetTransportPriorityQos;
    if( OpenDDS::DCPS::DCPS_debug_level>1) {
      ACE_DEBUG((LM_DEBUG,
        ACE_TEXT("(%P|%t) Config::loadWriter() - ")
        ACE_TEXT("  [writer/%s] %s == %d.\n"),
        sectionName.c_str(),
        TRANSPORTPRIORITY_KEYNAME,
        profile->qos.transport_priority.value
      ));
    }
  }

  // Lifespan                            = <number>
  valueString.clear();
  heap.get_string_value( sectionKey, LIFESPANDURATION_KEYNAME, valueString);
  if (valueString.length() > 0) {
    profile->qos.lifespan.duration.nanosec = 0;
    profile->qos.lifespan.duration.sec = ACE_OS::atoi( valueString.c_str());
    profile->mask |= SetLifespanQos;
    if( OpenDDS::DCPS::DCPS_debug_level>1) {
      ACE_DEBUG((LM_DEBUG,
        ACE_TEXT("(%P|%t) Config::loadWriter() - ")
        ACE_TEXT("  [writer/%s] %s == %d.\n"),
        sectionName.c_str(),
        LIFESPANDURATION_KEYNAME,
        profile->qos.lifespan.duration.sec
      ));
    }
  }

  // UserData                            = <string>
  valueString.clear();
  heap.get_string_value( sectionKey, USERDATA_KEYNAME, valueString);
  if (valueString.length() > 0) {
    profile->mask |= SetUserDataQos;
    profile->qos.user_data.value.replace(
      static_cast<CORBA::ULong>(valueString.length()),
      static_cast<CORBA::ULong>(valueString.length()),
      const_cast<CORBA::Octet*>(
        reinterpret_cast<const CORBA::Octet*>( valueString.c_str())
      )
    );
    if( OpenDDS::DCPS::DCPS_debug_level>1) {
      ACE_DEBUG((LM_DEBUG,
        ACE_TEXT("(%P|%t) Config::loadWriter() - ")
        ACE_TEXT("  [writer/%s] %s == %s.\n"),
        sectionName.c_str(),
        USERDATA_KEYNAME,
        valueString.c_str()
      ));
    }
  }

  // OwnershipKind                       = <string> # One of SHARED, EXCLUSIVE
  valueString.clear();
  heap.get_string_value( sectionKey, OWNERSHIPKIND_KEYNAME, valueString);
  if (valueString.length() > 0) {
    if( OpenDDS::DCPS::DCPS_debug_level>1) {
      ACE_DEBUG((LM_DEBUG,
        ACE_TEXT("(%P|%t) Config::loadWriter() - ")
        ACE_TEXT("  [topic/%s] %s == %s.\n"),
        sectionName.c_str(),
        OWNERSHIPKIND_KEYNAME,
        valueString.c_str()
      ));
    }
    if( valueString == ACE_TEXT("SHARED")) {
      profile->qos.ownership.kind = ::DDS::SHARED_OWNERSHIP_QOS;
      profile->mask |= SetOwnershipKindQos;

#ifndef OPENDDS_NO_OWNERSHIP_KIND_EXCLUSIVE
    } else if( valueString == ACE_TEXT("EXCLUSIVE")) {
      profile->qos.ownership.kind = ::DDS::EXCLUSIVE_OWNERSHIP_QOS;
      profile->mask |= SetOwnershipKindQos;
#endif

    } else {
      ACE_DEBUG((LM_WARNING,
        ACE_TEXT("(%P|%t) loadWriter() - ")
        ACE_TEXT("unrecognized value for %s: %s - ")
        ACE_TEXT("not assigning a value.\n"),
        OWNERSHIPKIND_KEYNAME,
        valueString.c_str()
      ));
    }
  }

#ifndef OPENDDS_NO_OWNERSHIP_KIND_EXCLUSIVE
  // OwnershipStrength                   = <number>
  valueString.clear();
  heap.get_string_value( sectionKey, OWNERSHIPSTRENGTH_KEYNAME, valueString);
  if (valueString.length() > 0) {
    profile->qos.ownership_strength.value = ACE_OS::atoi( valueString.c_str());
    profile->mask |= SetOwnershipStrengthQos;
    if( OpenDDS::DCPS::DCPS_debug_level>1) {
      ACE_DEBUG((LM_DEBUG,
        ACE_TEXT("(%P|%t) Config::loadWriter() - ")
        ACE_TEXT("  [writer/%s] %s == %d.\n"),
        sectionName.c_str(),
        OWNERSHIPSTRENGTH_KEYNAME,
        profile->qos.ownership_strength.value
      ));
    }
  }
#endif

  // WriterDataLifecycle                 = <bool> # Boolean: numeric 0 or 1
  valueString.clear();
  if( 0 == heap.get_string_value( sectionKey, WRITERDATALIFECYCLE_KEYNAME, valueString)) {
    profile->qos.writer_data_lifecycle.autodispose_unregistered_instances
      = ACE_OS::atoi( valueString.c_str());
    profile->mask |= SetWriterDataLifecycleQos;
    if( OpenDDS::DCPS::DCPS_debug_level>1) {
      ACE_DEBUG((LM_DEBUG,
        ACE_TEXT("(%P|%t) Config::loadWriter() - ")
        ACE_TEXT("  [writer/%s] %s == %d.\n"),
        sectionName.c_str(),
        WRITERDATALIFECYCLE_KEYNAME,
        profile->qos.writer_data_lifecycle.autodispose_unregistered_instances
      ));
    }
  }

  // Publisher            = <string> # One of publisher <name>
  valueString.clear();
  heap.get_string_value( sectionKey, PUBLISHER_KEYNAME, valueString);
  if (valueString.length() > 0) {
    profile->publisher = ACE_TEXT_ALWAYS_CHAR(valueString.c_str());
    if( OpenDDS::DCPS::DCPS_debug_level>1) {
      ACE_DEBUG((LM_DEBUG,
        ACE_TEXT("(%P|%t) Config::loadWriter() - ")
        ACE_TEXT("  [writer/%s] %s == %s.\n"),
        sectionName.c_str(),
        PUBLISHER_KEYNAME,
        profile->publisher.c_str()
      ));
    }
  }

  // Topic            = <string> # One of topic <name>
  valueString.clear();
  heap.get_string_value( sectionKey, TOPIC_KEYNAME, valueString);
  if (valueString.length() > 0) {
    profile->topic = ACE_TEXT_ALWAYS_CHAR(valueString.c_str());
    if( OpenDDS::DCPS::DCPS_debug_level>1) {
      ACE_DEBUG((LM_DEBUG,
        ACE_TEXT("(%P|%t) Config::loadWriter() - ")
        ACE_TEXT("  [writer/%s] %s == %s.\n"),
        sectionName.c_str(),
        TOPIC_KEYNAME,
        profile->topic.c_str()
      ));
    }
  }

  // Store the profile for the current writer.
  this->writerProfileMap_[ACE_TEXT_ALWAYS_CHAR(sectionName.c_str())] = profile;
}

void
Config::loadSubscriber(
  ACE_Configuration_Heap& heap,
  ACE_Configuration_Section_Key& sectionKey,
  std::basic_string<ACE_TCHAR> sectionName
)
{
  /**
   * [subscriber/<name>]
   *   # Subscriber Qos Policy values
   *   Presentation                  = <string> # One of INSTANCE, TOPIC, GROUP
   *   PresentationCoherent          = <bool> # Boolean: numeric 0 or 1
   *   PresentationOrdered           = <bool> # Boolean: numeric 0 or 1
   *   Partition                     = <string> # Only single value supported
   *   GroupData                     = <string>
   *   EntityFactory                 = <bool> # Boolean: numeric 0 or 1
   *
   *   Participant                   = <string> # One of participant <name>
   *   TransportIndex                = <number> # Index into transport configurations
   */

  // Note that this requires that the Service Participant already be
  // initialized before we configure from the file.  Also, since we have
  // not created any Entities yet, we go to the initial default values
  // rather than to the containing Entity.
  SubscriberProfile* profile = new SubscriberProfile();
  profile->qos = TheServiceParticipant->initial_SubscriberQos();
  ACE_TString valueString;

  // Presentation                        = <string> # One of INSTANCE, TOPIC, GROUP
  valueString.clear();
  heap.get_string_value( sectionKey, PRESENTATION_KEYNAME, valueString);
  if (valueString.length() > 0) {
    if( OpenDDS::DCPS::DCPS_debug_level>1) {
      ACE_DEBUG((LM_DEBUG,
        ACE_TEXT("(%P|%t) Config::loadSubscriber() - ")
        ACE_TEXT("  [subscriber/%s] %s == %s.\n"),
        sectionName.c_str(),
        PRESENTATION_KEYNAME,
        valueString.c_str()
      ));
    }
    if( valueString == ACE_TEXT("INSTANCE")) {
      profile->qos.presentation.access_scope = ::DDS::INSTANCE_PRESENTATION_QOS;

    } else if( valueString == ACE_TEXT("TOPIC")) {
      profile->qos.presentation.access_scope = ::DDS::TOPIC_PRESENTATION_QOS;

    } else if( valueString == ACE_TEXT("GROUP")) {
      profile->qos.presentation.access_scope = ::DDS::GROUP_PRESENTATION_QOS;

    } else {
      ACE_DEBUG((LM_WARNING,
        ACE_TEXT("(%P|%t) loadSubscriber() - ")
        ACE_TEXT("unrecognized value for %s: %s - ")
        ACE_TEXT("not assigning a value.\n"),
        PRESENTATION_KEYNAME,
        valueString.c_str()
      ));
    }
  }

  // PresentationCoherent                = <bool> # Boolean: numeric 0 or 1
  valueString.clear();
  if( 0 == heap.get_string_value( sectionKey, PRESENTATIONCOHERENT_KEYNAME, valueString)) {
    profile->qos.presentation.coherent_access
      = ACE_OS::atoi( valueString.c_str());
    if( OpenDDS::DCPS::DCPS_debug_level>1) {
      ACE_DEBUG((LM_DEBUG,
        ACE_TEXT("(%P|%t) Config::loadSubscriber() - ")
        ACE_TEXT("  [subscriber/%s] %s == %d.\n"),
        sectionName.c_str(),
        PRESENTATIONCOHERENT_KEYNAME,
        profile->qos.presentation.coherent_access
      ));
    }
  }

  // PresentationOrdered                 = <bool> # Boolean: numeric 0 or 1
  valueString.clear();
  if( 0 == heap.get_string_value( sectionKey, PRESENTATIONORDERED_KEYNAME, valueString)) {
    profile->qos.presentation.ordered_access
      = ACE_OS::atoi( valueString.c_str());
    if( OpenDDS::DCPS::DCPS_debug_level>1) {
      ACE_DEBUG((LM_DEBUG,
        ACE_TEXT("(%P|%t) Config::loadSubscriber() - ")
        ACE_TEXT("  [subscriber/%s] %s == %d.\n"),
        sectionName.c_str(),
        PRESENTATIONORDERED_KEYNAME,
        profile->qos.presentation.ordered_access
      ));
    }
  }

  // Partition                           = <string> # Only single value supported
  valueString.clear();
  heap.get_string_value( sectionKey, PARTITION_KEYNAME, valueString);
  if (valueString.length() > 0) {
    profile->qos.partition.name.length( 1);
    profile->qos.partition.name[0] = ACE_TEXT_ALWAYS_CHAR(valueString.c_str());
    if( OpenDDS::DCPS::DCPS_debug_level>1) {
      ACE_DEBUG((LM_DEBUG,
        ACE_TEXT("(%P|%t) Config::loadSubscriber() - ")
        ACE_TEXT("  [subscriber/%s] %s == %s.\n"),
        sectionName.c_str(),
        PARTITION_KEYNAME,
        valueString.c_str()
      ));
    }
  }

  // GroupData                           = <string>
  valueString.clear();
  heap.get_string_value( sectionKey, GROUPDATA_KEYNAME, valueString);
  if (valueString.length() > 0) {
    profile->qos.group_data.value.replace(
      static_cast<CORBA::ULong>(valueString.length()),
      static_cast<CORBA::ULong>(valueString.length()),
      const_cast<CORBA::Octet*>(
        reinterpret_cast<const CORBA::Octet*>( valueString.c_str())
      )
    );
    if( OpenDDS::DCPS::DCPS_debug_level>1) {
      ACE_DEBUG((LM_DEBUG,
        ACE_TEXT("(%P|%t) Config::loadSubscriber() - ")
        ACE_TEXT("  [subscriber/%s] %s == %s.\n"),
        sectionName.c_str(),
        GROUPDATA_KEYNAME,
        valueString.c_str()
      ));
    }
  }

  // EntityFactory                       = <bool> # Boolean: numeric 0 or 1
  valueString.clear();
  if( 0 == heap.get_string_value( sectionKey, ENTITYFACTORY_KEYNAME, valueString)) {
    profile->qos.entity_factory.autoenable_created_entities
      = ACE_OS::atoi( valueString.c_str());
    if( OpenDDS::DCPS::DCPS_debug_level>1) {
      ACE_DEBUG((LM_DEBUG,
        ACE_TEXT("(%P|%t) Config::loadSubscriber() - ")
        ACE_TEXT("  [subscriber/%s] %s == %d.\n"),
        sectionName.c_str(),
        ENTITYFACTORY_KEYNAME,
        profile->qos.entity_factory.autoenable_created_entities
      ));
    }
  }

  // Participant      = <string> # One of participant <name>
  valueString.clear();
  heap.get_string_value( sectionKey, PARTICIPANT_KEYNAME, valueString);
  if (valueString.length() > 0) {
    profile->participant = ACE_TEXT_ALWAYS_CHAR(valueString.c_str());
    if( OpenDDS::DCPS::DCPS_debug_level>1) {
      ACE_DEBUG((LM_DEBUG,
        ACE_TEXT("(%P|%t) Config::loadSubscriber() - ")
        ACE_TEXT("  [publisher/%s] %s == %s.\n"),
        sectionName.c_str(),
        PARTICIPANT_KEYNAME,
        profile->participant.c_str()
      ));
    }
  }

  // TransportIndex   = <number> # Index into transport configurations
  valueString.clear();
  heap.get_string_value( sectionKey, TRANSPORTINDEX_KEYNAME, valueString);
  if (valueString.length() > 0) {
    profile->transport = ACE_OS::atoi( valueString.c_str());
    if( OpenDDS::DCPS::DCPS_debug_level>1) {
      ACE_DEBUG((LM_DEBUG,
        ACE_TEXT("(%P|%t) Config::loadSubscriber() - ")
        ACE_TEXT("  [subscriber/%s] %s == %d.\n"),
        sectionName.c_str(),
        TRANSPORTINDEX_KEYNAME,
        profile->transport
      ));
    }
  }

  // Store the profile for the current participant.
  this->subscriberProfileMap_[ACE_TEXT_ALWAYS_CHAR(sectionName.c_str())] = profile;
}

void
Config::loadReader(
  ACE_Configuration_Heap& heap,
  ACE_Configuration_Section_Key& sectionKey,
  std::basic_string<ACE_TCHAR> sectionName
)
{
  /**
   * [reader/<name>]
   *   # DataReader Qos Policy values
   *   Durability                    = <string> # One of VOLATILE, LOCAL, TRANSIENT, PERSISTENT
   *   Deadline                      = <number>
   *   LatencyBudget                 = <number>
   *   LivelinessKind                = <string> # One of AUTOMATIC, PARTICIPANT, TOPIC
   *   LivelinessDuration            = <number>
   *   ReliabilityKind               = <string> # One of BEST_EFFORT, RELIABLE
   *   ReliabilityMaxBlocking        = <number>
   *   DestinationOrder              = <string> # One of SOURCE, RECEPTION
   *   HistoryKind                   = <string> # One of ALL, LAST
   *   HistoryDepth                  = <number>
   *   ResourceMaxSamples            = <number>
   *   ResourceMaxInstances          = <number>
   *   ResourceMaxSamplesPerInstance = <number>
   *   UserData                      = <string>
   *   TimeBasedFilter               = <number>
   *   ReaderDataLifecycle           = <number>
   *
   *   Subscriber                    = <string> # One of subscriber <name>
   *   Topic                         = <string> # One of topic <name>
   */

  // Note that this requires that the Service Participant already be
  // initialized before we configure from the file.  Also, since we have
  // not created any Entities yet, we go to the initial default values
  // rather than to the containing Entity.
  ReaderProfile* profile = new ReaderProfile();
  profile->qos  = TheServiceParticipant->initial_DataReaderQos();
  profile->mask = 0;
  ACE_TString valueString;

  // Durability = <string> # One of VOLATILE, LOCAL, TRANSIENT, PERSISTENT
  valueString.clear();
  heap.get_string_value( sectionKey, DURABILITY_KEYNAME, valueString);
  if (valueString.length() > 0) {
    if( OpenDDS::DCPS::DCPS_debug_level>1) {
      ACE_DEBUG((LM_DEBUG,
        ACE_TEXT("(%P|%t) Config::loadReader() - ")
        ACE_TEXT("  [reader/%s] %s == %s.\n"),
        sectionName.c_str(),
        DURABILITY_KEYNAME,
        valueString.c_str()
      ));
    }
    if( valueString == ACE_TEXT("VOLATILE")) {
      profile->qos.durability.kind = ::DDS::VOLATILE_DURABILITY_QOS;
      profile->mask |= SetDurabilityQos;

    } else if( valueString == ACE_TEXT("LOCAL")) {
      profile->qos.durability.kind = ::DDS::TRANSIENT_LOCAL_DURABILITY_QOS;
      profile->mask |= SetDurabilityQos;

    } else if( valueString == ACE_TEXT("TRANSIENT")) {
      profile->qos.durability.kind = ::DDS::TRANSIENT_DURABILITY_QOS;
      profile->mask |= SetDurabilityQos;

    } else if( valueString == ACE_TEXT("PERSISTENT")) {
      profile->qos.durability.kind = ::DDS::PERSISTENT_DURABILITY_QOS;
      profile->mask |= SetDurabilityQos;

    } else {
      ACE_DEBUG((LM_WARNING,
        ACE_TEXT("(%P|%t) loadReader() - ")
        ACE_TEXT("unrecognized value for %s: %s - ")
        ACE_TEXT("not assigning a value.\n"),
        DURABILITY_KEYNAME,
        valueString.c_str()
      ));
    }
  }

  // Deadline                            = <number>
  valueString.clear();
  heap.get_string_value( sectionKey, DEADLINE_KEYNAME, valueString);
  if (valueString.length() > 0) {
    profile->qos.deadline.period.nanosec = 0;
    profile->qos.deadline.period.sec = ACE_OS::atoi( valueString.c_str());
    profile->mask |= SetDeadlineQos;
    if( OpenDDS::DCPS::DCPS_debug_level>1) {
      ACE_DEBUG((LM_DEBUG,
        ACE_TEXT("(%P|%t) Config::loadReader() - ")
        ACE_TEXT("  [reader/%s] %s == %d.\n"),
        sectionName.c_str(),
        DEADLINE_KEYNAME,
        profile->qos.deadline.period.sec
      ));
    }
  }

  // LatencyBudget                       = <number>
  valueString.clear();
  heap.get_string_value( sectionKey, LATENCYBUDGET_KEYNAME, valueString);
  if (valueString.length() > 0) {
    profile->qos.latency_budget.duration.nanosec = 0;
    profile->qos.latency_budget.duration.sec = ACE_OS::atoi( valueString.c_str());
    profile->mask |= SetLatencyBudgetQos;
    if( OpenDDS::DCPS::DCPS_debug_level>1) {
      ACE_DEBUG((LM_DEBUG,
        ACE_TEXT("(%P|%t) Config::loadReader() - ")
        ACE_TEXT("  [reader/%s] %s == %d.\n"),
        sectionName.c_str(),
        LATENCYBUDGET_KEYNAME,
        profile->qos.latency_budget.duration.sec
      ));
    }
  }

  // LivelinessKind                      = <string> # One of AUTOMATIC, PARTICIPANT, TOPIC
  valueString.clear();
  heap.get_string_value( sectionKey, LIVELINESSKIND_KEYNAME, valueString);
  if (valueString.length() > 0) {
    if( OpenDDS::DCPS::DCPS_debug_level>1) {
      ACE_DEBUG((LM_DEBUG,
        ACE_TEXT("(%P|%t) Config::loadReader() - ")
        ACE_TEXT("  [reader/%s] %s == %s.\n"),
        sectionName.c_str(),
        LIVELINESSKIND_KEYNAME,
        valueString.c_str()
      ));
    }
    if( valueString == ACE_TEXT("AUTOMATIC")) {
      profile->qos.liveliness.kind = ::DDS::AUTOMATIC_LIVELINESS_QOS;
      profile->mask |= SetLivelinessKindQos;

    } else if( valueString == ACE_TEXT("PARTICIPANT")) {
      profile->qos.liveliness.kind = ::DDS::MANUAL_BY_PARTICIPANT_LIVELINESS_QOS;
      profile->mask |= SetLivelinessKindQos;

    } else if( valueString == ACE_TEXT("TOPIC")) {
      profile->qos.liveliness.kind = ::DDS::MANUAL_BY_TOPIC_LIVELINESS_QOS;
      profile->mask |= SetLivelinessKindQos;

    } else {
      ACE_DEBUG((LM_WARNING,
        ACE_TEXT("(%P|%t) loadReader() - ")
        ACE_TEXT("unrecognized value for %s: %s - ")
        ACE_TEXT("not assigning a value.\n"),
        LIVELINESSKIND_KEYNAME,
        valueString.c_str()
      ));
    }
  }

  // LivelinessDuration                  = <number>
  valueString.clear();
  heap.get_string_value( sectionKey, LIVELINESSDURATION_KEYNAME, valueString);
  if (valueString.length() > 0) {
    profile->qos.liveliness.lease_duration.nanosec = 0;
    profile->qos.liveliness.lease_duration.sec = ACE_OS::atoi( valueString.c_str());
    profile->mask |= SetLivelinessDurationQos;
    if( OpenDDS::DCPS::DCPS_debug_level>1) {
      ACE_DEBUG((LM_DEBUG,
        ACE_TEXT("(%P|%t) Config::loadReader() - ")
        ACE_TEXT("  [reader/%s] %s == %d.\n"),
        sectionName.c_str(),
        LIVELINESSDURATION_KEYNAME,
        profile->qos.liveliness.lease_duration.sec
      ));
    }
  }

  // ReliabilityKind                     = <string> # One of BEST_EFFORT, RELIABLE
  valueString.clear();
  heap.get_string_value( sectionKey, RELIABILITYKIND_KEYNAME, valueString);
  if (valueString.length() > 0) {
    if( OpenDDS::DCPS::DCPS_debug_level>1) {
      ACE_DEBUG((LM_DEBUG,
        ACE_TEXT("(%P|%t) Config::loadReader() - ")
        ACE_TEXT("  [reader/%s] %s == %s.\n"),
        sectionName.c_str(),
        RELIABILITYKIND_KEYNAME,
        valueString.c_str()
      ));
    }
    if( valueString == ACE_TEXT("BEST_EFFORT")) {
      profile->qos.reliability.kind = ::DDS::BEST_EFFORT_RELIABILITY_QOS;
      profile->mask |= SetReliabilityKindQos;

    } else if( valueString == ACE_TEXT("RELIABLE")) {
      profile->qos.reliability.kind = ::DDS::RELIABLE_RELIABILITY_QOS;
      profile->mask |= SetReliabilityKindQos;

    } else {
      ACE_DEBUG((LM_WARNING,
        ACE_TEXT("(%P|%t) loadReader() - ")
        ACE_TEXT("unrecognized value for %s: %s - ")
        ACE_TEXT("not assigning a value.\n"),
        RELIABILITYKIND_KEYNAME,
        valueString.c_str()
      ));
    }
  }

  // ReliabilityMaxBlocking              = <number>
  valueString.clear();
  heap.get_string_value( sectionKey, RELIABILITYMAXBLOCKING_KEYNAME, valueString);
  if (valueString.length() > 0) {
    profile->qos.reliability.max_blocking_time.nanosec = 0;
    profile->qos.reliability.max_blocking_time.sec = ACE_OS::atoi( valueString.c_str());
    profile->mask |= SetReliabilityMaxBlockingQos;
    if( OpenDDS::DCPS::DCPS_debug_level>1) {
      ACE_DEBUG((LM_DEBUG,
        ACE_TEXT("(%P|%t) Config::loadReader() - ")
        ACE_TEXT("  [reader/%s] %s == %d.\n"),
        sectionName.c_str(),
        RELIABILITYMAXBLOCKING_KEYNAME,
        profile->qos.reliability.max_blocking_time.sec
      ));
    }
  }

  // DestinationOrder                    = <string> # One of SOURCE, RECEPTION
  valueString.clear();
  heap.get_string_value( sectionKey, DESTINATIONORDER_KEYNAME, valueString);
  if (valueString.length() > 0) {
    if( OpenDDS::DCPS::DCPS_debug_level>1) {
      ACE_DEBUG((LM_DEBUG,
        ACE_TEXT("(%P|%t) Config::loadReader() - ")
        ACE_TEXT("  [reader/%s] %s == %s.\n"),
        sectionName.c_str(),
        DESTINATIONORDER_KEYNAME,
        valueString.c_str()
      ));
    }
    if( valueString == ACE_TEXT("SOURCE")) {
      profile->qos.destination_order.kind = ::DDS::BY_SOURCE_TIMESTAMP_DESTINATIONORDER_QOS;
      profile->mask |= SetDestinationOrderQos;

    } else if( valueString == ACE_TEXT("RECEPTION")) {
      profile->qos.destination_order.kind = ::DDS::BY_RECEPTION_TIMESTAMP_DESTINATIONORDER_QOS;
      profile->mask |= SetDestinationOrderQos;

    } else {
      ACE_DEBUG((LM_WARNING,
        ACE_TEXT("(%P|%t) loadReader() - ")
        ACE_TEXT("unrecognized value for %s: %s - ")
        ACE_TEXT("not assigning a value.\n"),
        DESTINATIONORDER_KEYNAME,
        valueString.c_str()
      ));
    }
  }

  // HistoryKind                         = <string> # One of ALL, LAST
  valueString.clear();
  heap.get_string_value( sectionKey, HISTORYKIND_KEYNAME, valueString);
  if (valueString.length() > 0) {
    if( OpenDDS::DCPS::DCPS_debug_level>1) {
      ACE_DEBUG((LM_DEBUG,
        ACE_TEXT("(%P|%t) Config::loadReader() - ")
        ACE_TEXT("  [reader/%s] %s == %s.\n"),
        sectionName.c_str(),
        HISTORYKIND_KEYNAME,
        valueString.c_str()
      ));
    }
    if( valueString == ACE_TEXT("ALL")) {
      profile->qos.history.kind = ::DDS::KEEP_ALL_HISTORY_QOS;
      profile->mask |= SetHistoryKindQos;

    } else if( valueString == ACE_TEXT("LAST")) {
      profile->qos.history.kind = ::DDS::KEEP_LAST_HISTORY_QOS;
      profile->mask |= SetHistoryKindQos;

    } else {
      ACE_DEBUG((LM_WARNING,
        ACE_TEXT("(%P|%t) loadReader() - ")
        ACE_TEXT("unrecognized value for %s: %s - ")
        ACE_TEXT("not assigning a value.\n"),
        HISTORYKIND_KEYNAME,
        valueString.c_str()
      ));
    }
  }

  // HistoryDepth                        = <number>
  valueString.clear();
  heap.get_string_value( sectionKey, HISTORYDEPTH_KEYNAME, valueString);
  if (valueString.length() > 0) {
    profile->qos.history.depth = ACE_OS::atoi( valueString.c_str());
    profile->mask |= SetHistoryDepthQos;
    if( OpenDDS::DCPS::DCPS_debug_level>1) {
      ACE_DEBUG((LM_DEBUG,
        ACE_TEXT("(%P|%t) Config::loadReader() - ")
        ACE_TEXT("  [reader/%s] %s == %d.\n"),
        sectionName.c_str(),
        HISTORYDEPTH_KEYNAME,
        profile->qos.history.depth
      ));
    }
  }

  // ResourceMaxSamples                  = <number>
  valueString.clear();
  heap.get_string_value( sectionKey, RESOURCEMAXSAMPLES_KEYNAME, valueString);
  if (valueString.length() > 0) {
    profile->qos.resource_limits.max_samples = ACE_OS::atoi( valueString.c_str());
    profile->mask |= SetResourceMaxSamplesQos;
    if( OpenDDS::DCPS::DCPS_debug_level>1) {
      ACE_DEBUG((LM_DEBUG,
        ACE_TEXT("(%P|%t) Config::loadReader() - ")
        ACE_TEXT("  [reader/%s] %s == %d.\n"),
        sectionName.c_str(),
        RESOURCEMAXSAMPLES_KEYNAME,
        profile->qos.resource_limits.max_samples
      ));
    }
  }

  // ResourceMaxInstances                = <number>
  valueString.clear();
  heap.get_string_value( sectionKey, RESOURCEMAXINSTANCES_KEYNAME, valueString);
  if (valueString.length() > 0) {
    profile->qos.resource_limits.max_instances = ACE_OS::atoi( valueString.c_str());
    profile->mask |= SetResourceMaxInstancesQos;
    if( OpenDDS::DCPS::DCPS_debug_level>1) {
      ACE_DEBUG((LM_DEBUG,
        ACE_TEXT("(%P|%t) Config::loadReader() - ")
        ACE_TEXT("  [reader/%s] %s == %d.\n"),
        sectionName.c_str(),
        RESOURCEMAXINSTANCES_KEYNAME,
        profile->qos.resource_limits.max_instances
      ));
    }
  }

  // ResourceMaxSamplesPerInstance       = <number>
  valueString.clear();
  heap.get_string_value( sectionKey, RESOURCEMAXSAMPLESPERINSTANCE_KEYNAME, valueString);
  if (valueString.length() > 0) {
    profile->qos.resource_limits.max_samples_per_instance = ACE_OS::atoi( valueString.c_str());
    profile->mask |= SetResourceMaxSamplesPerInstanceQos;
    if( OpenDDS::DCPS::DCPS_debug_level>1) {
      ACE_DEBUG((LM_DEBUG,
        ACE_TEXT("(%P|%t) Config::loadReader() - ")
        ACE_TEXT("  [reader/%s] %s == %d.\n"),
        sectionName.c_str(),
        RESOURCEMAXSAMPLESPERINSTANCE_KEYNAME,
        profile->qos.resource_limits.max_samples_per_instance
      ));
    }
  }

  // UserData                            = <string>
  valueString.clear();
  heap.get_string_value( sectionKey, USERDATA_KEYNAME, valueString);
  if (valueString.length() > 0) {
    profile->mask |= SetUserDataQos;
    profile->qos.user_data.value.replace(
      static_cast<CORBA::ULong>(valueString.length()),
      static_cast<CORBA::ULong>(valueString.length()),
      const_cast<CORBA::Octet*>(
        reinterpret_cast<const CORBA::Octet*>( valueString.c_str())
      )
    );
    if( OpenDDS::DCPS::DCPS_debug_level>1) {
      ACE_DEBUG((LM_DEBUG,
        ACE_TEXT("(%P|%t) Config::loadReader() - ")
        ACE_TEXT("  [reader/%s] %s == %s.\n"),
        sectionName.c_str(),
        USERDATA_KEYNAME,
        valueString.c_str()
      ));
    }
  }

  // TimeBasedFilter               = <number>
  valueString.clear();
  heap.get_string_value( sectionKey, TIMEBASEDFILTER_KEYNAME, valueString);
  if (valueString.length() > 0) {
    profile->qos.time_based_filter.minimum_separation.nanosec = 0;
    profile->qos.time_based_filter.minimum_separation.sec
      = ACE_OS::atoi( valueString.c_str());
    profile->mask |= SetTimeBasedFilterQos;
    if( OpenDDS::DCPS::DCPS_debug_level>1) {
      ACE_DEBUG((LM_DEBUG,
        ACE_TEXT("(%P|%t) Config::loadReader() - ")
        ACE_TEXT("  [reader/%s] %s == %d.\n"),
        sectionName.c_str(),
        TIMEBASEDFILTER_KEYNAME,
        profile->qos.time_based_filter.minimum_separation.sec
      ));
    }
  }

  // ReaderDataLifecycle           = <number>
  valueString.clear();
  if( 0 == heap.get_string_value( sectionKey, READERDATALIFECYCLE_KEYNAME, valueString)) {
    profile->qos.reader_data_lifecycle.autopurge_nowriter_samples_delay.nanosec = 0;
    profile->qos.reader_data_lifecycle.autopurge_nowriter_samples_delay.sec
      = ACE_OS::atoi( valueString.c_str());
    // @TODO: what's right value for autopurge_disposed_samples_delay?
    profile->qos.reader_data_lifecycle.autopurge_disposed_samples_delay.nanosec = 0;
    profile->qos.reader_data_lifecycle.autopurge_disposed_samples_delay.sec
      = 0;
    profile->mask |= SetReaderDataLifecycleQos;
    if( OpenDDS::DCPS::DCPS_debug_level>1) {
      ACE_DEBUG((LM_DEBUG,
        ACE_TEXT("(%P|%t) Config::loadReader() - ")
        ACE_TEXT("  [reader/%s] %s == %d %d.\n"),
        sectionName.c_str(),
        READERDATALIFECYCLE_KEYNAME,
        profile->qos.reader_data_lifecycle.autopurge_nowriter_samples_delay.sec,
        profile->qos.reader_data_lifecycle.autopurge_disposed_samples_delay.sec
      ));
    }
  }

  // Subscriber       = <string> # One of subscriber <name>
  valueString.clear();
  heap.get_string_value( sectionKey, SUBSCRIBER_KEYNAME, valueString);
  if (valueString.length() > 0) {
    profile->subscriber = ACE_TEXT_ALWAYS_CHAR(valueString.c_str());
    if( OpenDDS::DCPS::DCPS_debug_level>1) {
      ACE_DEBUG((LM_DEBUG,
        ACE_TEXT("(%P|%t) Config::loadReader() - ")
        ACE_TEXT("  [reader/%s] %s == %s.\n"),
        sectionName.c_str(),
        SUBSCRIBER_KEYNAME,
        profile->subscriber.c_str()
      ));
    }
  }

  // Topic            = <string> # One of topic <name>
  valueString.clear();
  heap.get_string_value( sectionKey, TOPIC_KEYNAME, valueString);
  if (valueString.length() > 0) {
    profile->topic = ACE_TEXT_ALWAYS_CHAR(valueString.c_str());
    if( OpenDDS::DCPS::DCPS_debug_level>1) {
      ACE_DEBUG((LM_DEBUG,
        ACE_TEXT("(%P|%t) Config::loadReader() - ")
        ACE_TEXT("  [reader/%s] %s == %s.\n"),
        sectionName.c_str(),
        TOPIC_KEYNAME,
        profile->topic.c_str()
      ));
    }
  }

  // Store the profile for the current participant.
  this->readerProfileMap_[ACE_TEXT_ALWAYS_CHAR(sectionName.c_str())] = profile;
}

} } // End of namespace OpenDDS::Model

OPENDDS_END_VERSIONED_NAMESPACE_DECL
