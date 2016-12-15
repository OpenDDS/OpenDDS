/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef UPDATER_H
#define UPDATER_H

#include "UpdateDataTypes.h"
#include "dds/DCPS/GuidUtils.h"
#include "ace/Synch_Traits.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace Update {

class Updater {
public:
  virtual ~Updater();

  // Request an image refresh to be sent to
  //  the specified callback (asynchronously).
  virtual void requestImage() = 0;

  // Propagate that an entity has been created.
  virtual void create(const UTopic&              topic) = 0;
  virtual void create(const UParticipant&  participant) = 0;
  virtual void create(const URActor&             actor) = 0;
  virtual void create(const UWActor&             actor) = 0;
  virtual void create(const OwnershipData&        data) = 0;

  // Propagate updated Qos parameters for an entity.
  virtual void update(const IdPath& id, const DDS::DomainParticipantQos& qos) = 0;
  virtual void update(const IdPath& id, const DDS::TopicQos&             qos) = 0;
  virtual void update(const IdPath& id, const DDS::DataWriterQos&        qos) = 0;
  virtual void update(const IdPath& id, const DDS::PublisherQos&         qos) = 0;
  virtual void update(const IdPath& id, const DDS::DataReaderQos&        qos) = 0;
  virtual void update(const IdPath& id, const DDS::SubscriberQos&        qos) = 0;
  virtual void update(const IdPath& id, const DDS::StringSeq&     exprParams) = 0;

  // Propagate that an entity has been destroyed.
  virtual void destroy(const IdPath& id, ItemType type, ActorType actor) = 0;
};

inline
Updater::~Updater()
{
}

} // namespace Update

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif /* UPDATER_H */
