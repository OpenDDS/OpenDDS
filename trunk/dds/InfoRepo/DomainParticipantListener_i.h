/*
 * $Id$
 *
 * Copyright 2010 Object Computing, Inc.
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef DOMAINPARTICIPANTLISTENER_I_H
#define DOMAINPARTICIPANTLISTENER_I_H

#include "inforepo_export.h"
#include "dds/DdsDcpsDomainS.h"
#include "dds/DCPS/Definitions.h"

#if !defined (ACE_LACKS_PRAGMA_ONCE)
#pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

//Class OPENDDS_DCPS_DomainParticipantListener_i
class OpenDDS_InfoRepoLib_Export OPENDDS_DCPS_DomainParticipantListener_i
  : public virtual DDS::DomainParticipantListener {
public:
  //Constructor
  OPENDDS_DCPS_DomainParticipantListener_i();

  //Destructor
  virtual ~OPENDDS_DCPS_DomainParticipantListener_i();

  virtual void on_inconsistent_topic(
    DDS::Topic_ptr the_topic,
    const DDS::InconsistentTopicStatus & status)
  ACE_THROW_SPEC((CORBA::SystemException));

  virtual void on_data_on_readers(
    DDS::Subscriber_ptr subs)
  ACE_THROW_SPEC((CORBA::SystemException));

  virtual void on_offered_deadline_missed(
    DDS::DataWriter_ptr writer,
    const DDS::OfferedDeadlineMissedStatus & status)
  ACE_THROW_SPEC((CORBA::SystemException));

  virtual void on_offered_incompatible_qos(
    DDS::DataWriter_ptr writer,
    const DDS::OfferedIncompatibleQosStatus & status)
  ACE_THROW_SPEC((CORBA::SystemException));

  virtual void on_liveliness_lost(
    DDS::DataWriter_ptr writer,
    const DDS::LivelinessLostStatus & status)
  ACE_THROW_SPEC((CORBA::SystemException));

  virtual void on_publication_matched(
    DDS::DataWriter_ptr writer,
    const DDS::PublicationMatchedStatus & status)
  ACE_THROW_SPEC((CORBA::SystemException));

  virtual void on_requested_deadline_missed(
    DDS::DataReader_ptr reader,
    const DDS::RequestedDeadlineMissedStatus & status)
  ACE_THROW_SPEC((CORBA::SystemException));

  virtual void on_requested_incompatible_qos(
    DDS::DataReader_ptr reader,
    const DDS::RequestedIncompatibleQosStatus & status)
  ACE_THROW_SPEC((CORBA::SystemException));

  virtual void on_sample_rejected(
    DDS::DataReader_ptr reader,
    const DDS::SampleRejectedStatus & status)
  ACE_THROW_SPEC((CORBA::SystemException));

  virtual void on_liveliness_changed(
    DDS::DataReader_ptr reader,
    const DDS::LivelinessChangedStatus & status)
  ACE_THROW_SPEC((CORBA::SystemException));

  virtual void on_data_available(
    DDS::DataReader_ptr reader)
  ACE_THROW_SPEC((CORBA::SystemException));

  virtual void on_subscription_matched(
    DDS::DataReader_ptr reader,
    const DDS::SubscriptionMatchedStatus & status)
  ACE_THROW_SPEC((CORBA::SystemException));

  virtual void on_sample_lost(
    DDS::DataReader_ptr reader,
    const DDS::SampleLostStatus & status)
  ACE_THROW_SPEC((CORBA::SystemException));
};

#endif /* DOMAINPARTICIPANTLISTENER_I_H  */
