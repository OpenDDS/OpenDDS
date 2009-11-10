/*
 * $Id$
 *
 * Copyright 2009 Object Computing, Inc.
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef TAO_DDS_DCPS_DOMAIN_PARTICIPANT_FACTORY_IMPL_H
#define TAO_DDS_DCPS_DOMAIN_PARTICIPANT_FACTORY_IMPL_H

#include "Definitions.h"
#include "dds/DdsDcpsDomainS.h"
#include "ace/Null_Mutex.h"
#include "ace/Recursive_Thread_Mutex.h"
#include "dds/DCPS/LocalObject.h"

#include <map>
#include <set>

#if !defined (ACE_LACKS_PRAGMA_ONCE)
#pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

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
  typedef std::set<Participant_Pair> DPSet;
  typedef std::map<DDS::DomainId_t, DPSet> DPMap;

  /** Constructor **/
  DomainParticipantFactoryImpl();

  /** Destructor **/
  virtual ~DomainParticipantFactoryImpl();

  virtual DDS::DomainParticipant_ptr create_participant(
    DDS::DomainId_t domainId,
    const DDS::DomainParticipantQos & qos,
    DDS::DomainParticipantListener_ptr a_listener,
    DDS::StatusMask mask)
  ACE_THROW_SPEC((CORBA::SystemException));

  virtual DDS::ReturnCode_t delete_participant(
    DDS::DomainParticipant_ptr a_participant)
  ACE_THROW_SPEC((CORBA::SystemException));

  virtual DDS::DomainParticipant_ptr lookup_participant(
    DDS::DomainId_t domainId)
  ACE_THROW_SPEC((CORBA::SystemException));

  virtual DDS::ReturnCode_t set_default_participant_qos(
    const DDS::DomainParticipantQos & qos)
  ACE_THROW_SPEC((CORBA::SystemException));

  virtual DDS::ReturnCode_t get_default_participant_qos(
    DDS::DomainParticipantQos & qos)
  ACE_THROW_SPEC((CORBA::SystemException));

  virtual DDS::DomainParticipantFactory_ptr get_instance()
  ACE_THROW_SPEC((CORBA::SystemException));

  virtual DDS::ReturnCode_t set_qos(
    const DDS::DomainParticipantFactoryQos & qos)
  ACE_THROW_SPEC((CORBA::SystemException));

  virtual DDS::ReturnCode_t get_qos(
    DDS::DomainParticipantFactoryQos & qos)
  ACE_THROW_SPEC((CORBA::SystemException));

  /** This method is not defined in the IDL and is defined for
  *  internal use.
  *  Delete all participants belong to this factory and hence
  *  delete all entities created by the participants.
  *  Note: The delete will terminate and return error code if
  *        any contained participant is not empty.
  */
  DDS::ReturnCode_t delete_contained_participants();

  // Expose the participants for reading.
  const DPMap& participants() const;

private:

  DDS::DomainParticipantFactoryQos qos_;

  /// The default qos value of DomainParticipant.
  DDS::DomainParticipantQos   default_participant_qos_;

  /// The collection of domain participants.
  DPMap                       participants_;

  // Protect the participant collection.
  // Use recursive mutex to allow nested acquisition and
  // release of a mutex that occurs in the same thread.
  ACE_Recursive_Thread_Mutex  participants_protector_;
};

} // namespace DCPS
} // namespace OpenDDS

#endif /* TAO_DDS_DCPS_DOMAIN_PARTICIPANT_FACTORY_IMPL_H  */
