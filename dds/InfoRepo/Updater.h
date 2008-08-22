// -*- C++ -*-

/**
 * @file      Updater.h
 *
 * $Id$
 *
 * @author Ciju John <johnc@ociweb.com>
 */

#ifndef _UPDATER_
#define _UPDATER_

#include "UpdateDataTypes.h"
#include "dds/DCPS/GuidUtils.h"
#include "ace/Synch.h"

namespace Update {

class Updater
{
  public:
    // Request an image refresh to be sent to
    //  the specified callback (asynchronously).
    virtual void requestImage (void) = 0;

    // Propagate that an entity has been created.
    virtual void create( const UTopic&              topic) = 0;
    virtual void create( const UParticipant&  participant) = 0;
    virtual void create( const URActor&             actor) = 0;
    virtual void create( const UWActor&             actor) = 0;
    virtual void create( const OwnershipData&        data) = 0;

    // Propagate updated Qos parameters for an entity.
    virtual void update( const IdType& id, const ::DDS::DomainParticipantQos& qos) = 0;
    virtual void update( const IdType& id, const ::DDS::TopicQos&             qos) = 0;
    virtual void update( const IdType& id, const ::DDS::DataWriterQos&        qos) = 0;
    virtual void update( const IdType& id, const ::DDS::PublisherQos&         qos) = 0;
    virtual void update( const IdType& id, const ::DDS::DataReaderQos&        qos) = 0;
    virtual void update( const IdType& id, const ::DDS::SubscriberQos&        qos) = 0;

    // Propagate that an entity has been destroyed.
    virtual void destroy(
                   ItemType      type,
                   const IdType& id,
                   ActorType     actor = DataWriter,
                   long          domain = 0,
                   const IdType& participant = ::OpenDDS::DCPS::GUID_UNKNOWN
                 ) = 0;
};

} // End of namespace Update

#endif // _UPDATER_
