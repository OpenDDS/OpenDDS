/*
 * $Id$
 *
 * Copyright 2009 Object Computing, Inc.
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef FAILOVERLISTENER_T_H
#define FAILOVERLISTENER_T_H

#include "dds/DdsDcpsSubscriptionExtC.h"

#if !defined (ACE_LACKS_PRAGMA_ONCE)
#pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

namespace OpenDDS {
namespace DCPS {

/// @class FailoverListener
class FailoverListener
  : public virtual OpenDDS::DCPS::LocalObject<OpenDDS::DCPS::DataReaderListener> {
public:
  /// Only construct with a repository key value.
  FailoverListener(int key);

  /// Virtual destructor
  virtual ~FailoverListener();

  virtual void on_requested_deadline_missed(
    DDS::DataReader_ptr reader,
    const DDS::RequestedDeadlineMissedStatus & status)
  throw(CORBA::SystemException);

  virtual void on_requested_incompatible_qos(
    DDS::DataReader_ptr reader,
    const DDS::RequestedIncompatibleQosStatus & status)
  throw(CORBA::SystemException);

  virtual void on_liveliness_changed(
    DDS::DataReader_ptr reader,
    const DDS::LivelinessChangedStatus & status)
  throw(CORBA::SystemException);

  virtual void on_subscription_matched(
    DDS::DataReader_ptr reader,
    const DDS::SubscriptionMatchedStatus & status)
  throw(CORBA::SystemException);

  virtual void on_sample_rejected(
    DDS::DataReader_ptr reader,
    const DDS::SampleRejectedStatus& status)
  throw(CORBA::SystemException);

  virtual void on_data_available(
    DDS::DataReader_ptr reader)
  throw(CORBA::SystemException);

  virtual void on_sample_lost(
    DDS::DataReader_ptr reader,
    const DDS::SampleLostStatus& status)
  throw(CORBA::SystemException);

  virtual void on_subscription_disconnected(
    DDS::DataReader_ptr reader,
    const OpenDDS::DCPS::SubscriptionDisconnectedStatus& status)
  throw(CORBA::SystemException);

  virtual void on_subscription_reconnected(
    DDS::DataReader_ptr reader,
    const OpenDDS::DCPS::SubscriptionReconnectedStatus& status)
  throw(CORBA::SystemException);

  virtual void on_subscription_lost(
    DDS::DataReader_ptr reader,
    const OpenDDS::DCPS::SubscriptionLostStatus& status)
  throw(CORBA::SystemException);

  virtual void on_connection_deleted(
    DDS::DataReader_ptr reader)
  throw(CORBA::SystemException);

  virtual void on_budget_exceeded(
    DDS::DataReader_ptr reader,
    const OpenDDS::DCPS::BudgetExceededStatus& status)
  throw(CORBA::SystemException);

private:
  /// Our repository key.  If we trigger, this is the key to the
  /// repository that has been lost.
  int key_;
};

} // namespace DCPS
} // namespace OpenDDS

#endif /* FAILOVERLISTENER_T_H  */
