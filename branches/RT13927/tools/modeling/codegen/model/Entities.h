// -*- C++ -*-
//
// $Id$
#ifndef ENTITIES_H
#define ENTITIES_H

// Needed here to avoid the pragma below when necessary.
#include /**/ "ace/pre.h"
#include /**/ "ace/config-all.h"

#if !defined (ACE_LACKS_PRAGMA_ONCE)
#pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

#include "dds/DCPS/Service_Participant.h"
#include "Config.h"
#include "Delegate.h"

#include "model_export.h"

#include <string>
#include <map>
#include <queue>

namespace OpenDDS { namespace Model {

/**
 * @class Entities
 *
 * @brief manage DDS DCPS Entities for an application
 *
 * This class manages DDS support for an application from the command
 * line and the contents of a file.  It also allows application created
 * Entities to be stored here as well.
 *
 * To configure middleware from a file using this class, first
 * instantiate the class, then add the type support, then initialize the
 * class.  Access of the Entities will create the requested Entity and
 * any other Entities needed for containment.
 *
 * Example:
 *
 *   File:
 *      [participant/someParticipant]
 *      DomainId = 1066
 *
 *      [topic/someTopic]
 *      Participant = someParticipant
 *      Type = XXX
 *
 *      [publisher/somePublisher]
 *      Participant = someParticipant
 *      TransportIndex = 1
 *
 *      [writer/someWriter]
 *      Publisher = somePublisher
 *      Topic = someTopic
 *
 *   Code:
 *      #include "XXXTypeSupportImpl.h"
 *
 *      OpenDDS::Model::Entities middleware;
 *
 *      middleware.add< XXXTypeSupportImpl>( "XXX", "someParticipant");
 *
 *      middleware.init( argc, argv);
 *
 *      DDS::DataWriter_var writer = middleware.writer( "someWriter");
 */
class OpenDDS_Model_Export Entities  {
  public:
    Entities();

    virtual ~Entities();

    /// @name DDS API Entity accessors.
    /// @{
    DDS::DomainParticipant_var participant( const std::string& name);
    DDS::Topic_var             topic(       const std::string& name,
                                            const std::string& participant);
    DDS::Publisher_var         publisher(   const std::string& name);
    DDS::Subscriber_var        subscriber(  const std::string& name);
    DDS::DataWriter_var        writer(      const std::string& name);
    DDS::DataReader_var        reader(      const std::string& name);
    /// @}

    /// @name Add user created Entities to the storage.
    /// @{
    void add( const std::string& name, DDS::DomainParticipant_ptr participant);
    void add( const std::string& name,
              const std::string& participant, DDS::Topic_ptr      topic);
    void add( const std::string& name,        DDS::Publisher_ptr  publisher);
    void add( const std::string& name,        DDS::Subscriber_ptr subscriber);
    void add( const std::string& name,        DDS::DataWriter_ptr writer);
    void add( const std::string& name,        DDS::DataReader_ptr reader);

    /// Type support is special.
    template< typename TypeSupport>
    void add( const std::string& name,
              const std::string& participant);
    /// @}

    /// @name Lifetime management.
    /// @{
    void init( int& argc, ACE_TCHAR** argv);
    void fini();
    /// @}

  private:
    OpenDDS::DCPS::TransportImpl_rch
    transport( OpenDDS::DCPS::TransportIdType key);

    /// Deferred registration of type support for a participant.
    void registerTypes( const std::string& participant);

    /// The command line and file configuration information.
    Config config_;

  protected:
    /// Delegate for performing DDS specific actions.
    Delegate delegate_;

  private:
    /// Map of queues.
    typedef std::map< std::string, std::queue<DDS::TypeSupport_ptr> >
            TypeSupportByParticipantQueues;

    /// Store type support until it can be registered.
    TypeSupportByParticipantQueues typeSupport_;

    // Maps for access via string key names.

    typedef std::map< std::string, DDS::DomainParticipant_ptr>
            StringToParticipantMap;

    typedef std::map< std::string, std::string>
            StringToTypeNameMap;

    typedef std::map< std::string, DDS::Topic_ptr> StringToTopicMap;
    typedef std::map< std::string, StringToTopicMap>
            ParticipantToTopicMap;

    typedef std::map< std::string, DDS::Publisher_ptr>
            StringToPublisherMap;

    typedef std::map< std::string, DDS::Subscriber_ptr>
            StringToSubscriberMap;

    typedef std::map< std::string, DDS::DataWriter_ptr>
            StringToDataWriterMap;

    typedef std::map< std::string, DDS::DataReader_ptr>
            StringToDataReaderMap;

    typedef std::map< std::string, OpenDDS::DCPS::TransportImpl*>
            StringToTransportImplMap;

    StringToParticipantMap   participantByString_;
    StringToTypeNameMap      typeNameByString_;
    ParticipantToTopicMap    topicByParticipant_;
    StringToPublisherMap     publisherByString_;
    StringToSubscriberMap    subscriberByString_;
    StringToDataWriterMap    writerByString_;
    StringToDataReaderMap    readerByString_;
    StringToTransportImplMap transportByString_;
};

} } // End of namespace OpenDDS::Model

#if defined (__ACE_INLINE__)
# include "Entities.inl"
#endif  /* __ACE_INLINE__ */

#endif // ENTITIES_H

