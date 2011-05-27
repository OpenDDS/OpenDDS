// -*- C++ -*-
//
// $Id$
#ifndef OPTIONS_H
#define OPTIONS_H

// Needed here to avoid the pragma below when necessary.
#include /**/ "ace/pre.h"
#include /**/ "ace/config-all.h"

#if !defined (ACE_LACKS_PRAGMA_ONCE)
#pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

#include /**/ "ace/OS.h"

#include "dds/DCPS/DataCollector_T.h"

#include <iosfwd>
#include <string>
#include <map>

class ACE_Configuration_Heap;
class ACE_Configuration_Section_Key;

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
 *   -c <seconds>
 *      Run test for <seconds> seconds.  If not specified, the test will
 *      not terminate until killed by an external signal.
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
 */
class Options  {
  public:
    /// Container type for participant profiles.
    typedef std::map< std::string, ParticipantProfile*> ParticipantMap;

    /// Container type for topic profiles.
    typedef std::map< std::string, TopicProfile*> TopicMap;

    /// Container type for publication profiles.
    typedef std::map< std::string, PublicationProfile*> PublicationMap;

    /// Container type for subscription profiles.
    typedef std::map< std::string, SubscriptionProfile*> SubscriptionMap;

    /// Default constructor.
    Options( int argc, char** argv, char** envp = 0);

    /// Virtual destructor.
    virtual ~Options();

    /// Indication of configuration status.
    operator bool() const;

    /// Test verbosity.
    protected: bool& verbose();
    public:    bool  verbose() const;

    /// Successful configuration state.
    protected: bool& configured();
    public:    bool  configured() const;

    /// Test duration.
    protected: long& duration();
    public:    long  duration() const;

    /// Raw data output file.
    protected: std::string& rawOutputFilename();
    public:    std::string  rawOutputFilename() const;

    /// Raw latency data buffer size.
    protected: unsigned int& raw_buffer_size();
    public:    unsigned int  raw_buffer_size() const;

    /// Raw latency data buffer type.
    protected: OpenDDS::DCPS::DataCollector< double>::OnFull& raw_buffer_type();
    public:    OpenDDS::DCPS::DataCollector< double>::OnFull  raw_buffer_type() const;

  public:
    /// Participant profile container.
    const ParticipantMap& participantMap() const;

    /// Topic profile container.
    const TopicMap& topicMap() const;

    /// Publication profile container.
    const PublicationMap& publicationMap() const;

    /// Subscription profile container.
    const SubscriptionMap& subscriptionMap() const;

  private:
    /// Configure scenario information from a file.
    void configureScenarios( const char* filename);

    /// Extract the DDS Entity information for the scenario.
    void configureEntities( ACE_Configuration_Heap& heap);

    /// Load specification for a DomainParticipant.
    void loadParticipant(
           ACE_Configuration_Heap& heap,
           ACE_Configuration_Section_Key& key,
           std::string name
         );

    /// Load specification for a Topic.
    void loadTopic(
           ACE_Configuration_Heap& heap,
           ACE_Configuration_Section_Key& key,
           std::string name
         );

    /// Load specification for a Publication.
    void loadPublication(
           ACE_Configuration_Heap& heap,
           ACE_Configuration_Section_Key& key,
           std::string name
         );

    /// Load specification for a Subscription.
    void loadSubscription(
           ACE_Configuration_Heap& heap,
           ACE_Configuration_Section_Key& key,
           std::string name
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
    unsigned int raw_buffer_size_;

    /// Raw latency data buffer type.
    OpenDDS::DCPS::DataCollector< double>::OnFull raw_buffer_type_;

    /// Participant profiles.
    ParticipantMap  participantMap_;

    /// Topic profiles.
    TopicMap        topicMap_;

    /// Publication profiles.
    PublicationMap  publicationMap_;

    /// Subscription profiles.
    SubscriptionMap subscriptionMap_;
};

} // End of namespace Test

#if defined (__ACE_INLINE__)
# include "Options.inl"
#endif  /* __ACE_INLINE__ */

#endif // OPTIONS_H

