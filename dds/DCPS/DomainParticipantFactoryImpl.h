/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_DOMAIN_PARTICIPANT_FACTORY_IMPL_H
#define OPENDDS_DCPS_DOMAIN_PARTICIPANT_FACTORY_IMPL_H

#include "Definitions.h"
#include "dds/DdsDcpsDomainC.h"
#include "ace/Null_Mutex.h"
#include "ace/Recursive_Thread_Mutex.h"
#include "dds/DCPS/LocalObject.h"
#include "dds/DCPS/PoolAllocator.h"

#if !defined (ACE_LACKS_PRAGMA_ONCE)
#pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

class DomainParticipantImpl;

/**
* @class DomainParticipantFactoryImpl
*
* @brief Implements the OpenDDS::DCPS::DomainParticipantFactory
*        interfaces.
*
* This class acts as factory of the DomainParticipant.
*
* See the DDS specification, OMG formal/04-12-02, for a description of
* the interface this class is implementing.
*
*/
class OpenDDS_Dcps_Export DomainParticipantFactoryImpl
  : public virtual OpenDDS::DCPS::LocalObject<DDS::DomainParticipantFactory> {
public:

  typedef Objref_Servant_Pair <DomainParticipantImpl, DDS::DomainParticipant,
    DDS::DomainParticipant_ptr, DDS::DomainParticipant_var> Participant_Pair;
  typedef OPENDDS_SET(Participant_Pair) DPSet;
  typedef OPENDDS_MAP(DDS::DomainId_t, DPSet) DPMap;

  DomainParticipantFactoryImpl();

  virtual ~DomainParticipantFactoryImpl();

  virtual DDS::DomainParticipant_ptr create_participant(
    DDS::DomainId_t domainId,
    const DDS::DomainParticipantQos & qos,
    DDS::DomainParticipantListener_ptr a_listener,
    DDS::StatusMask mask);

  virtual DDS::ReturnCode_t delete_participant(
    DDS::DomainParticipant_ptr a_participant);

  virtual DDS::DomainParticipant_ptr lookup_participant(
    DDS::DomainId_t domainId);

  virtual DDS::ReturnCode_t set_default_participant_qos(
    const DDS::DomainParticipantQos & qos);

  virtual DDS::ReturnCode_t get_default_participant_qos(
    DDS::DomainParticipantQos & qos);

  virtual DDS::DomainParticipantFactory_ptr get_instance();

  virtual DDS::ReturnCode_t set_qos(
    const DDS::DomainParticipantFactoryQos & qos);

  virtual DDS::ReturnCode_t get_qos(
    DDS::DomainParticipantFactoryQos & qos);

  /// Expose the participants for reading.
  const DPMap& participants() const;

private:

  DDS::DomainParticipantFactoryQos qos_;

  /// The default qos value of DomainParticipant.
  DDS::DomainParticipantQos   default_participant_qos_;

  /// The collection of domain participants.
  DPMap                       participants_;

  /// Protect the participant collection.
  /// Use recursive mutex to allow nested acquisition and
  /// release of a mutex that occurs in the same thread.
  ACE_Recursive_Thread_Mutex  participants_protector_;
};

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif /* OPENDDS_DCPS_DOMAIN_PARTICIPANT_FACTORY_IMPL_H  */
