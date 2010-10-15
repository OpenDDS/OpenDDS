// -*- C++ -*-
//
// $Id$
#ifndef CONFIG_H
#define CONFIG_H

// Needed here to avoid the pragma below when necessary.
#include /**/ "ace/pre.h"
#include /**/ "ace/config-all.h"

#if !defined (ACE_LACKS_PRAGMA_ONCE)
#pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

#include <string>
#include <map>

class ACE_Configuration_Heap;
class ACE_Configuration_Section_Key;

namespace OpenDDS { namespace Model {

struct ParticipantProfile;
struct TopicProfile;
struct PublisherProfile;
struct WriterProfile;
struct SubscriberProfile;
struct ReaderProfile;

/**
 * @class Config
 *
 * @brief Initialize DDS Middleware from command line and configuration file.
 *
 * This class configures DDS middleware from the contents of a file.  The
 * command line is parsed for the filename and for a verbosity indication.
 *
 * Command line options extracted by this class are:
 *
 *   -v
 *      Be verbose when executing.
 *
 *   -DDSAppConfig <file>
 *
 *      Extract detailed application parameters from <file>.  The format
 *      of the file is a set of KeyValue pairs organized into sections.
 *      There is a separate subsection for each DDS Entity to be
 *      instantiated within the application.
 *
 *      Subsections are named as:
 *
 *        [participant/<name>]
 *        [topic/<name>]
 *        [publisher/<name>]
 *        [writer/<name>]
 *        [subscriber/<name>]
 *        [reader/<name>]
 */
class Config  {
  public:
    /// Container type for participant profiles.
    typedef std::map< std::string, ParticipantProfile*> ParticipantProfileMap;

    /// Container type for topic profiles.
    typedef std::map< std::string, TopicProfile*> TopicProfileMap;

    /// Container type for publisher profiles.
    typedef std::map< std::string, PublisherProfile*> PublisherProfileMap;

    /// Container type for writer profiles.
    typedef std::map< std::string, WriterProfile*> WriterProfileMap;

    /// Container type for subscriber profiles.
    typedef std::map< std::string, SubscriberProfile*> SubscriberProfileMap;

    /// Container type for reader profiles.
    typedef std::map< std::string, ReaderProfile*> ReaderProfileMap;

    Config();

    virtual ~Config();

    /// Defered initialization.
    void init( int argc, ACE_TCHAR** argv);

    /// Indication of configuration status.
    operator bool() const;

    /// @name Test verbosity.
    /// @{
    protected: bool& verbose();
    public:    bool  verbose() const;
    /// @}

  public:
    /// Participant profile container.
    const ParticipantProfileMap& participantProfileMap() const;

    /// Topic profile container.
    const TopicProfileMap& topicProfileMap() const;

    /// Publisher profile container.
    const PublisherProfileMap& publisherProfileMap() const;

    /// Writer profile container.
    const WriterProfileMap& writerProfileMap() const;

    /// Subscriber profile container.
    const SubscriberProfileMap& subscriberProfileMap() const;

    /// Reader profile container.
    const ReaderProfileMap& readerProfileMap() const;

  private:
    /// Extract the DDS Entity information for the application.
    void configureEntities( const ACE_TCHAR* filename);

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

    /// Load specification for a Publisher.
    void loadPublisher(
           ACE_Configuration_Heap& heap,
           ACE_Configuration_Section_Key& key,
           std::basic_string<ACE_TCHAR> name
         );

    /// Load specification for a Writer.
    void loadWriter(
           ACE_Configuration_Heap& heap,
           ACE_Configuration_Section_Key& key,
           std::basic_string<ACE_TCHAR> name
         );

    /// Load specification for a Subscriber.
    void loadSubscriber(
           ACE_Configuration_Heap& heap,
           ACE_Configuration_Section_Key& key,
           std::basic_string<ACE_TCHAR> name
         );

    /// Load specification for a Reader.
    void loadReader(
           ACE_Configuration_Heap& heap,
           ACE_Configuration_Section_Key& key,
           std::basic_string<ACE_TCHAR> name
         );

    /// Chattiness.
    bool verbose_;

    /// Success of configuration steps.
    bool configured_;

    /// Participant profiles.
    ParticipantProfileMap  participantProfileMap_;

    /// Topic profiles.
    TopicProfileMap        topicProfileMap_;

    /// Publication profiles.
    PublisherProfileMap  publisherProfileMap_;

    /// Publication profiles.
    WriterProfileMap  writerProfileMap_;

    /// Subscription profiles.
    SubscriberProfileMap subscriberProfileMap_;

    /// Subscription profiles.
    ReaderProfileMap readerProfileMap_;
};

} } // End of namespace OpenDDS::Model

#if defined (__ACE_INLINE__)
# include "Config.inl"
#endif  /* __ACE_INLINE__ */

#endif // CONFIG_H

