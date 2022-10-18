/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef UPDATELISTENER_T_H
#define UPDATELISTENER_T_H

#include "dds/DCPS/SubscriberImpl.h"
#include "FederationId.h"
#include "UpdateReceiver_T.h"

#if !defined (ACE_LACKS_PRAGMA_ONCE)
#pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace Federator {

/// @class UpdateListener<DataType, ReaderType>
template<class DataType, class ReaderType>
class UpdateListener
  : public virtual OpenDDS::DCPS::LocalObject<DDS::DataReaderListener> {
public:
  UpdateListener(UpdateProcessor<DataType>& processor);

  virtual ~UpdateListener();

  virtual void on_requested_deadline_missed(
    DDS::DataReader_ptr reader,
    const DDS::RequestedDeadlineMissedStatus & status);

  virtual void on_requested_incompatible_qos(
    DDS::DataReader_ptr reader,
    const DDS::RequestedIncompatibleQosStatus & status);

  virtual void on_liveliness_changed(
    DDS::DataReader_ptr reader,
    const DDS::LivelinessChangedStatus & status);

  virtual void on_subscription_matched(
    DDS::DataReader_ptr reader,
    const DDS::SubscriptionMatchedStatus & status);

  virtual void on_sample_rejected(
    DDS::DataReader_ptr reader,
    const DDS::SampleRejectedStatus& status);

  virtual void on_data_available(
    DDS::DataReader_ptr reader);

  virtual void on_sample_lost(
    DDS::DataReader_ptr reader,
    const DDS::SampleLostStatus& status);

  /// Access our Federation Id value.
  void federationId(const TAO_DDS_DCPSFederationId& id);
  const TAO_DDS_DCPSFederationId& federationId() const;

  void stop();
  void join();

private:
  /// Our Federation Id value.
  TAO_DDS_DCPSFederationId federationId_;

  /// Manager object to delegate sample processing to.
  UpdateReceiver<DataType> receiver_;

};

} // namespace Federator
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#if defined (ACE_TEMPLATES_REQUIRE_SOURCE)
#include "UpdateListener_T.cpp"
#endif /* ACE_TEMPLATES_REQUIRE_SOURCE */

#if defined (ACE_TEMPLATES_REQUIRE_PRAGMA)
#pragma message ("UpdateListener_T.cpp template inst")
#pragma implementation ("UpdateListener_T.cpp")
#endif /* ACE_TEMPLATES_REQUIRE_PRAGMA */

#endif /* UPDATELISTENER_T_H  */
