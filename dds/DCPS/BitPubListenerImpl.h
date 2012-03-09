/*
 * $Id$
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_BITPUBLISTENERIMPL_H
#define OPENDDS_DCPS_BITPUBLISTENERIMPL_H

#ifndef DDS_HAS_MINIMUM_BIT

#include "dds/DdsDcpsSubscriptionS.h"
#include "Definitions.h"

#if !defined (ACE_LACKS_PRAGMA_ONCE)
#pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

namespace OpenDDS {
namespace DCPS {

class DomainParticipantImpl;

//Class BitPubListenerImpl
class BitPubListenerImpl
  : public virtual OpenDDS::DCPS::LocalObject<DDS::DataReaderListener> {
public:
  //Constructor
  BitPubListenerImpl(DomainParticipantImpl* partipant);

  //Destructor
  virtual ~BitPubListenerImpl();

  virtual void on_requested_deadline_missed(
    DDS::DataReader_ptr reader,
    const DDS::RequestedDeadlineMissedStatus& status);

  virtual void on_requested_incompatible_qos(
    DDS::DataReader_ptr reader,
    const DDS::RequestedIncompatibleQosStatus& status);

  virtual void on_liveliness_changed(
    DDS::DataReader_ptr reader,
    const DDS::LivelinessChangedStatus& status);

  virtual void on_subscription_matched(
    DDS::DataReader_ptr reader,
    const DDS::SubscriptionMatchedStatus& status);

  virtual void on_sample_rejected(
    DDS::DataReader_ptr reader,
    const DDS::SampleRejectedStatus& status);

  virtual void on_data_available(
    DDS::DataReader_ptr reader);

  virtual void on_sample_lost(
    DDS::DataReader_ptr reader,
    const DDS::SampleLostStatus& status);

private:
  DomainParticipantImpl* partipant_;

};

} // namespace DCPS
} // namespace OpenDDS

#endif // DDS_HAS_MINIMUM_BIT

#endif // OPENDDS_DCPS_BITPUBLISTENERIMPL_H
