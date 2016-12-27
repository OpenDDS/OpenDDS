// -*- C++ -*-
//
#ifndef OPTIONS_H
#define OPTIONS_H

// Needed here to avoid the pragma below when necessary.
#include /**/ "ace/pre.h"
#include /**/ "ace/config-all.h"

#if !defined (ACE_LACKS_PRAGMA_ONCE)
#pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

#include "dds/DCPS/DataCollector_T.h"

#include <iosfwd>
#include <string>
#include <map>

ACE_BEGIN_VERSIONED_NAMESPACE_DECL
class ACE_Configuration_Heap;
class ACE_Configuration_Section_Key;
ACE_END_VERSIONED_NAMESPACE_DECL

namespace Test {

  struct ParticipantProfile;
  struct TopicProfile;
  struct PublicationProfile;
  struct SubscriptionProfile;

/**
 * @class Options
 *
 * @brief manage execution options
 *
 * This class extracts option information from the command line and makes
 * it available to the process.
 *
 * Options extracted by this class are:
 *
 *   -v
 *      Be verbose when executing.
 *
 *   -d <seconds>
 *      Run test for <seconds> seconds.  If not specified, the test will
 *      not terminate until killed by an external signal.
 *
 *   -t <type> (one of "unbounded", "newest" or "oldest")
 *      Configure the raw latency data gathering by configured DataReader
 *      entities to be of the specified type.
 *
 *   -s <size>
 *      Gather <size> amount of raw latency data from any configured
 *      DataReader entities.  If this is specified when the type is as
 *      "unbounded" it is used as a preallocation hint.
 *
 *   -r <file>
 *      Place raw latency data from configured DataReader entities into
 *      this file.
 *
 *   -f <file>
 *      Extract detailed scenario parameters from <file>.  The format of
 *      the file is a set of KeyValue pairs organized into sections.
 *      There is a common section that can be used to define options
 *      common to the entire process and a separate subsection for each
 *      publication to be instantiated for the scenario.
 *
 *      The key/value pairs which can be specified outside of all
 *      sections include:
 *
 *        TestDuration = <seconds>
 *
 *      Where the test will continue for <seconds> and then terminate.
 *      If not specified, the test will continue until an external signal
 *      terminates it.
 *
 *      Other subsections are named as:
 *
 *        [participant/<name>]
 *        [topic/<name>]
 *        [publication/<name>]
 *        [subscription/<name>]
 *
 * NOTE: The order on the command line determines the final value of a
 *       configurable element.  For example: if more than one value is
 *       given on the command line for test duration ('-d 5 -d 2') then
 *       the rightmost value will be used.  Likewise if a value is
 *       configured on both the command line and in a configuration file,
 *       the processing order is honored.  This means that it is
 *       preferable to specify the configuration file first on the
 *       command line so that the remaining values can override the
 *       values from the command line.
 */
class Options  {
  public:
    /// Container type for participant profiles.
    typedef std::map< std::string, ParticipantProfile*> ParticipantProfileMap;

    /// Container type for topic profiles.
    typedef std::map< std::string, TopicProfile*> TopicProfileMap;

    /// Container type for publication profiles.
    typedef std::map< std::string, PublicationProfile*> PublicationProfileMap;

    /// Container type for subscription profiles.
    typedef std::map< std::string, SubscriptionProfile*> SubscriptionProfileMap;

    /// Default constructor.
    Options( int argc, ACE_TCHAR** argv, char** envp = 0);

    virtual ~Options();

    /// Indication of configuration status.
    operator bool() const;

    /// @name Test verbosity.
    /// @{
    protected: bool& verbose();
    public:    bool  verbose() const;
    /// @}

    /// @name Successful configuration state.
    /// @{
    protected: bool& configured();
    public:    bool  configured() const;
    /// @}

    /// @name Test duration.
    /// @{
    protected: long& duration();
    public:    long  duration() const;
    /// @}

    /// @name Raw data output file.
    /// @{
    protected: std::string& rawOutputFilename();
    public:    std::string  rawOutputFilename() const;
    /// @}

    /// @name Raw latency data buffer size.
    /// @{
    protected: unsigned int& rawBufferSize();
    public:    unsigned int  rawBufferSize() const;
    /// @}

    /// @name Raw latency data buffer type.
    /// @{
    protected: OpenDDS::DCPS::DataCollector< double>::OnFull& rawBufferType();
    public:    OpenDDS::DCPS::DataCollector< double>::OnFull  rawBufferType() const;
    /// @}

  public:
    /// Participant profile container.
    const ParticipantProfileMap& participantProfileMap() const;

    /// Topic profile container.
    const TopicProfileMap& topicProfileMap() const;

    /// Publication profile container.
    const PublicationProfileMap& publicationProfileMap() const;

    /// Subscription profile container.
    const SubscriptionProfileMap& subscriptionProfileMap() const;

  private:
    /// Configure scenario information from a file.
    void configureScenarios( const ACE_TCHAR* filename);

    /// Extract the DDS Entity information for the scenario.
    void configureEntities( ACE_Configuration_Heap& heap);

    /// Load specification for a DomainParticipant.
    void loadParticipant(
           ACE_Configuration_Heap& heap,
           ACE_Configuration_Section_Key& key,
           std::basic_string<ACE_TCHAR> name
         );

    /// Load specification for a Topic.
    void loadTopic(
           ACE_Configuration_Heap& heap,
           ACE_Configuration_Section_Key& key,
           std::basic_string<ACE_TCHAR> name
         );

    /// Load specification for a Publication.
    void loadPublication(
           ACE_Configuration_Heap& heap,
           ACE_Configuration_Section_Key& key,
           std::basic_string<ACE_TCHAR> name
         );

    /// Load specification for a Subscription.
    void loadSubscription(
           ACE_Configuration_Heap& heap,
           ACE_Configuration_Section_Key& key,
           std::basic_string<ACE_TCHAR> name
         );

    /// Test verbosity.
    bool verbose_;

    /// Success of configuration steps.
    bool configured_;

    /// Test duration in seconds: -1 indicates no timed termination.
    long duration_;

    /// Raw data output file.
    std::string rawOutputFilename_;

    /// Raw latency data buffer size.
    unsigned int rawBufferSize_;

    /// Raw latency data buffer type.
    OpenDDS::DCPS::DataCollector< double>::OnFull rawBufferType_;

    /// Participant profiles.
    ParticipantProfileMap  participantProfileMap_;

    /// Topic profiles.
    TopicProfileMap        topicProfileMap_;

    /// Publication profiles.
    PublicationProfileMap  publicationProfileMap_;

    /// Subscription profiles.
    SubscriptionProfileMap subscriptionProfileMap_;
};

} // End of namespace Test

#if defined (__ACE_INLINE__)
# include "Options.inl"
#endif  /* __ACE_INLINE__ */

#endif // OPTIONS_H

