/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_TOPIC_IMPL_H
#define OPENDDS_DCPS_TOPIC_IMPL_H

#include "dds/DdsDcpsTopicC.h"
#include "dds/DdsDcpsInfoUtilsC.h"
#include "EntityImpl.h"
#include "TopicDescriptionImpl.h"
#include "TopicCallbacks.h"

#if !defined (ACE_LACKS_PRAGMA_ONCE)
#pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

class TopicDescriptionImpl;
class Monitor;

/**
* @class TopicImpl
*
* @brief Implements the DDS::Topic interface.
*
* See the DDS specification, OMG formal/2015-04-10, for a description of
* the interface this class is implementing.
*/
class OpenDDS_Dcps_Export TopicImpl
  : public virtual LocalObject<DDS::Topic>,
    public virtual EntityImpl,
    public virtual TopicCallbacks,
    public virtual TopicDescriptionImpl {
public:
  TopicImpl(const char*                    topic_name,
            const char*                    type_name,
            OpenDDS::DCPS::TypeSupport_ptr type_support,
            const DDS::TopicQos &          qos,
            DDS::TopicListener_ptr         a_listener,
            const DDS::StatusMask &        mask,
            DomainParticipantImpl*         participant);

  virtual ~TopicImpl();

  virtual DDS::InstanceHandle_t get_instance_handle();

  virtual DDS::ReturnCode_t set_qos(const DDS::TopicQos& qos);

  virtual DDS::ReturnCode_t get_qos(DDS::TopicQos& qos);

  virtual DDS::ReturnCode_t set_listener(DDS::TopicListener_ptr a_listener,
                                         DDS::StatusMask mask);

  virtual DDS::TopicListener_ptr get_listener();

  virtual DDS::ReturnCode_t get_inconsistent_topic_status(
    DDS::InconsistentTopicStatus& a_status);

  virtual DDS::ReturnCode_t enable();

  /** This method is not defined in the IDL and is defined for
  *  internal use.
  *  Return the id given by discovery.
  */
  RepoId get_id() const;

  // OpenDDS extension which doesn't duplicate the string to prevent
  // the runtime costs of making a copy
  const char* type_name() const;

  // OpenDDS extension which doesn't duplicate the string to prevent
  // the runtime costs of making a copy
  const char* topic_name() const;

  virtual void transport_config(const TransportConfig_rch& cfg);

  void inconsistent_topic(int count);

  /**
   * Compare the QoS of the IDL Type and the Topic/DataWriter/DataReader to see
   * whether the type matches all values in the QoS.
   */
  bool check_data_representation(const DDS::DataRepresentationIdSeq& qos_ids, bool is_data_writer);

private:
  /// The topic qos
  DDS::TopicQos                qos_;

  /// Mutex to protect listener info
  ACE_Thread_Mutex             listener_mutex_;
  /// The mask for which kind of events the listener
  ///  will be notified about.
  DDS::StatusMask              listener_mask_;
  /// The topic listener
  DDS::TopicListener_var       listener_;

  /// The id given by discovery.
  RepoId                       id_;

  /// Mutex to protect status info
  ACE_Thread_Mutex             status_mutex_;
  /// Count of discovered (readers/writers using) topics with the same
  /// topic name but different characteristics (typename)
  DDS::InconsistentTopicStatus inconsistent_topic_status_;

  /// Pointer to the monitor object for this entity
  unique_ptr<Monitor> monitor_;
};



} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif /* OPENDDS_DCPS_TOPIC_IMPL_H  */
