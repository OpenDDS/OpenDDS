// -*- C++ -*-
//
#ifndef ENTITIES_H
#define ENTITIES_H

// Needed here to avoid the pragma below when necessary.
#include /**/ "ace/pre.h"
#include /**/ "ace/config-all.h"

#if !defined (ACE_LACKS_PRAGMA_ONCE)
#pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

#include "dds/DCPS/Service_Participant.h"
#include "dds/DCPS/PoolAllocator.h"
#include "Config.h"
#include "Delegate.h"

#include "model_export.h"

#include <string>

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

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
    Entities(int& argc, ACE_TCHAR** argv);

    virtual ~Entities();

    /// @name DDS API Entity accessors.
    /// @{
    DDS::DomainParticipant_var participant( const OPENDDS_STRING& name,
                                            const OPENDDS_STRING& transportConfig);
    DDS::Topic_var             topic(       const OPENDDS_STRING& name,
                                            const OPENDDS_STRING& participant,
                                            const OPENDDS_STRING& transportConfig);
    DDS::Publisher_var         publisher(   const OPENDDS_STRING& name,
                                            const OPENDDS_STRING& transportConfig);
    DDS::Subscriber_var        subscriber(  const OPENDDS_STRING& name,
                                            const OPENDDS_STRING& transportConfig);
    DDS::DataWriter_var        writer(      const OPENDDS_STRING& name,
                                            const OPENDDS_STRING& transportConfig);
    DDS::DataReader_var        reader(      const OPENDDS_STRING& name,
                                            const OPENDDS_STRING& transportConfig);
    /// @}

    /// @name Add user created Entities to the storage.
    /// @{
    void add( const OPENDDS_STRING& name, DDS::DomainParticipant_ptr participant);
    void add( const OPENDDS_STRING& name,
              const OPENDDS_STRING& participant, DDS::Topic_ptr      topic);
    void add( const OPENDDS_STRING& name,        DDS::Publisher_ptr  publisher);
    void add( const OPENDDS_STRING& name,        DDS::Subscriber_ptr subscriber);
    void add( const OPENDDS_STRING& name,        DDS::DataWriter_ptr writer);
    void add( const OPENDDS_STRING& name,        DDS::DataReader_ptr reader);

    /// Type support is special.
    template< typename TypeSupport>
    void add( const OPENDDS_STRING& name,
              const OPENDDS_STRING& participant);
    /// @}


  protected:
    /// Delegate for performing DDS specific actions.
    Delegate delegate_;

  private:
    //OpenDDS::DCPS::TransportImpl_rch
    //transport( OpenDDS::DCPS::TransportIdType key);

    /// Deferred registration of type support for a participant.
    void registerTypes(const OPENDDS_STRING& participant,
                       const OPENDDS_STRING& transportConfig);

    /// The command line and file configuration information.
    Config config_;

  private:
    /// Map of queues.
    typedef OPENDDS_MAP(OPENDDS_STRING, OPENDDS_QUEUE(DDS::TypeSupport_ptr) )
            TypeSupportByParticipantQueues;

    /// Store type support until it can be registered.
    TypeSupportByParticipantQueues typeSupport_;

    // Maps for access via string key names.

    typedef OPENDDS_MAP(OPENDDS_STRING, DDS::DomainParticipant_var)
            StringToParticipantMap;

    typedef OPENDDS_MAP(OPENDDS_STRING,OPENDDS_STRING)
            StringToTypeNameMap;

    typedef OPENDDS_MAP(OPENDDS_STRING, DDS::Topic_ptr) StringToTopicMap;
    typedef OPENDDS_MAP(OPENDDS_STRING, StringToTopicMap)
            ParticipantToTopicMap;

    typedef OPENDDS_MAP(OPENDDS_STRING, DDS::Publisher_ptr)
            StringToPublisherMap;

    typedef OPENDDS_MAP(OPENDDS_STRING, DDS::Subscriber_ptr)
            StringToSubscriberMap;

    typedef OPENDDS_MAP(OPENDDS_STRING, DDS::DataWriter_ptr)
            StringToDataWriterMap;

    typedef OPENDDS_MAP(OPENDDS_STRING, DDS::DataReader_ptr)
            StringToDataReaderMap;

    typedef OPENDDS_MAP(OPENDDS_STRING, OpenDDS::DCPS::TransportImpl*)
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

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#if defined (__ACE_INLINE__)
# include "Entities.inl"
#endif  /* __ACE_INLINE__ */

#endif // ENTITIES_H

