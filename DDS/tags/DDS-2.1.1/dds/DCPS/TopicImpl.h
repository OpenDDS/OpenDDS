/*
 * $Id$
 *
 * Copyright 2010 Object Computing, Inc.
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef TAO_DDS_DCPS_TOPIC_IMPL_H
#define TAO_DDS_DCPS_TOPIC_IMPL_H

#include "dds/DdsDcpsTopicS.h"
#include "dds/DdsDcpsInfoC.h"
#include "EntityImpl.h"
#include "TopicDescriptionImpl.h"

#if !defined (ACE_LACKS_PRAGMA_ONCE)
#pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

namespace OpenDDS {
namespace DCPS {

class TopicDescriptionImpl;
class Monitor;

/**
* @class TopicImpl
*
* @brief Implements the DDS::Topic interface.
*
* See the DDS specification, OMG formal/04-12-02, for a description of
* the interface this class is implementing.
*/
class OpenDDS_Dcps_Export TopicImpl
  : public virtual OpenDDS::DCPS::LocalObject<DDS::Topic>,
    public virtual EntityImpl,
    public virtual TopicDescriptionImpl {
public:

  //Constructor
  TopicImpl(const RepoId                   topic_id,
            const char*                    topic_name,
            const char*                    type_name,
            OpenDDS::DCPS::TypeSupport_ptr type_support,
            const DDS::TopicQos &          qos,
            DDS::TopicListener_ptr         a_listener,
            const DDS::StatusMask &        mask,
            DomainParticipantImpl*         participant);

  //Destructor
  virtual ~TopicImpl();

  virtual DDS::InstanceHandle_t get_instance_handle()
  ACE_THROW_SPEC((CORBA::SystemException));

  virtual DDS::ReturnCode_t set_qos(
    const DDS::TopicQos & qos)
  ACE_THROW_SPEC((CORBA::SystemException));

  virtual DDS::ReturnCode_t get_qos(
    DDS::TopicQos & qos)
  ACE_THROW_SPEC((CORBA::SystemException));

  virtual DDS::ReturnCode_t set_listener(
    DDS::TopicListener_ptr a_listener,
    DDS::StatusMask mask)
  ACE_THROW_SPEC((CORBA::SystemException));

  virtual DDS::TopicListener_ptr get_listener()
  ACE_THROW_SPEC((CORBA::SystemException));

  virtual DDS::ReturnCode_t get_inconsistent_topic_status(
    DDS::InconsistentTopicStatus & a_status)
  ACE_THROW_SPEC((CORBA::SystemException));

  virtual DDS::ReturnCode_t enable()
  ACE_THROW_SPEC((CORBA::SystemException));

  /** This method is not defined in the IDL and is defined for
  *  internal use.
  *  Return the id given by the DCPSInfo repositoy.
  */
  RepoId get_id() const;

  CORBA::Long entity_refs() const {
    return entity_refs_;
  };

  void add_entity_ref() {
    entity_refs_++;
  };

  void remove_entity_ref() {
    entity_refs_--;
  };

private:
  /// The topic qos
  DDS::TopicQos                qos_;

  /// The mask for which kind of events the listener
  ///  will be notified about.
  DDS::StatusMask              listener_mask_;
  /// The topic listener
  DDS::TopicListener_var       listener_;
  /// The topic listener servant.
  DDS::TopicListener*          fast_listener_;

  /// The id given by DCPSInfo/repository.
  RepoId                       id_;

  /// The number of DataReaders and DataWriters using this Topic.
  CORBA::Long                  entity_refs_;

  /// count of different topics with the same topic name but
  /// different characteristics (typename or ?incompatible Qos?).
  DDS::InconsistentTopicStatus inconsistent_topic_status_;

  /// Pointer to the monitor object for this entity
  Monitor* monitor_;
};

} // namespace DCPS
} // namespace OpenDDS

#endif /* TAO_DDS_DCPS_TOPIC_IMPL_H  */
