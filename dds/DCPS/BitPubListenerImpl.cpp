/*
 * $Id$
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "DCPS/DdsDcps_pch.h" //Only the _pch include should start with DCPS/

#ifndef DDS_HAS_MINIMUM_BIT

#include "BitPubListenerImpl.h"
#include "DomainParticipantImpl.h"
#include "GuidConverter.h"
#include "Discovery.h"
#include "Service_Participant.h"
#include "BuiltInTopicUtils.h"

namespace OpenDDS {
namespace DCPS {

BitPubListenerImpl::BitPubListenerImpl(DomainParticipantImpl* partipant)
: partipant_ (partipant)
{
}

BitPubListenerImpl::~BitPubListenerImpl()
{
}

void BitPubListenerImpl::on_data_available(DDS::DataReader_ptr reader)
ACE_THROW_SPEC((CORBA::SystemException))
{
  try {
    ::DDS::PublicationBuiltinTopicDataDataReader_var bit_dr =
      ::DDS::PublicationBuiltinTopicDataDataReader::_narrow(reader);

    if (CORBA::is_nil(bit_dr.in())) {
      ACE_ERROR((LM_ERROR,
                 ACE_TEXT("(%P|%t) ERROR: BitPubListenerImpl::on_data_available ")
                 ACE_TEXT("_narrow failed!\n")));
    }

    ::DDS::PublicationBuiltinTopicData data;
    DDS::SampleInfo si;

    DDS::ReturnCode_t status = bit_dr->take_next_sample(data, si);

    if (status == DDS::RETCODE_OK) {
      if (si.valid_data) {
        Discovery_rch disc =
          TheServiceParticipant->get_discovery(partipant_->get_domain_id());
        PublicationId pub_id =
          disc->bit_key_to_repo_id(partipant_, BUILT_IN_PUBLICATION_TOPIC, data.key);
        CORBA::Long ownership_strength = data.ownership_strength.value;
        this->partipant_->update_ownership_strength(pub_id, ownership_strength);
        GuidConverter writer_converter(pub_id);
        ACE_DEBUG((LM_DEBUG,
          ACE_TEXT("(%P|%t) BitPubListenerImpl::on_data_available: %X ")
          ACE_TEXT("reset ownership strength %d for writer %C.\n"),
          this, ownership_strength, std::string(writer_converter).c_str()));
      }
      else if (si.instance_state != DDS::NOT_ALIVE_DISPOSED_INSTANCE_STATE
               && si.instance_state != DDS::NOT_ALIVE_NO_WRITERS_INSTANCE_STATE) {
        ACE_ERROR((LM_ERROR,
              ACE_TEXT("(%P|%t) ERROR: BitPubListenerImpl::on_data_available:")
              ACE_TEXT(" unknown instance state: %d\n"),
              si.instance_state));
      }
    }
    else {
      ACE_ERROR((LM_ERROR,
                 ACE_TEXT("(%P|%t) ERROR: BitPubListenerImpl::on_data_available:")
                 ACE_TEXT(" unexpected status: %d\n"),
                 status));
    }
  } catch (const CORBA::Exception& e) {
    e._tao_print_exception("Exception caught in BitPubListenerImpl::on_data_available():");
  }
}

void BitPubListenerImpl::on_requested_deadline_missed(
  DDS::DataReader_ptr,
  const DDS::RequestedDeadlineMissedStatus &)
ACE_THROW_SPEC((CORBA::SystemException))
{
}

void BitPubListenerImpl::on_requested_incompatible_qos(
  DDS::DataReader_ptr,
  const DDS::RequestedIncompatibleQosStatus &)
ACE_THROW_SPEC((CORBA::SystemException))
{
}

void BitPubListenerImpl::on_liveliness_changed(
  DDS::DataReader_ptr,
  const DDS::LivelinessChangedStatus &)
ACE_THROW_SPEC((CORBA::SystemException))
{
}

void BitPubListenerImpl::on_subscription_matched(
  DDS::DataReader_ptr,
  const DDS::SubscriptionMatchedStatus &)
ACE_THROW_SPEC((CORBA::SystemException))
{
}

void BitPubListenerImpl::on_sample_rejected(
  DDS::DataReader_ptr,
  const DDS::SampleRejectedStatus&)
ACE_THROW_SPEC((CORBA::SystemException))
{
}

void BitPubListenerImpl::on_sample_lost(
  DDS::DataReader_ptr,
  const DDS::SampleLostStatus&)
ACE_THROW_SPEC((CORBA::SystemException))
{
}

} // namespace DCPS
} // namespace OpenDDS

#endif // DDS_HAS_MINIMUM_BIT
