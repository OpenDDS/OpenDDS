/*
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
#include "dds/DdsDcpsCoreTypeSupportImpl.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

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
{
  try {
    ::DDS::PublicationBuiltinTopicDataDataReader_var bit_dr =
      ::DDS::PublicationBuiltinTopicDataDataReader::_narrow(reader);

    if (CORBA::is_nil(bit_dr.in())) {
      ACE_ERROR((LM_ERROR,
                 ACE_TEXT("(%P|%t) ERROR: BitPubListenerImpl::on_data_available ")
                 ACE_TEXT("_narrow failed!\n")));
      return;
    }

    ::DDS::PublicationBuiltinTopicData data;
    DDS::SampleInfo si;
    DDS::ReturnCode_t status;

    do {
      status = bit_dr->take_next_sample(data, si);

      if (status == DDS::RETCODE_OK) {
        if (si.valid_data) {
#ifndef OPENDDS_NO_OWNERSHIP_KIND_EXCLUSIVE
          Discovery_rch disc =
            TheServiceParticipant->get_discovery(partipant_->get_domain_id());
          PublicationId pub_id =
            disc->bit_key_to_repo_id(partipant_, BUILT_IN_PUBLICATION_TOPIC, data.key);
          CORBA::Long const ownership_strength = data.ownership_strength.value;
          this->partipant_->update_ownership_strength(pub_id, ownership_strength);
          if (DCPS_debug_level > 4) {
            GuidConverter writer_converter(pub_id);
            ACE_DEBUG((LM_DEBUG,
              ACE_TEXT("(%P|%t) BitPubListenerImpl::on_data_available: %X ")
              ACE_TEXT("reset ownership strength %d for writer %C.\n"),
              this, ownership_strength, OPENDDS_STRING(writer_converter).c_str()));
          }
#endif
        }
        else if (si.instance_state != DDS::NOT_ALIVE_DISPOSED_INSTANCE_STATE
                 && si.instance_state != DDS::NOT_ALIVE_NO_WRITERS_INSTANCE_STATE) {
          ACE_ERROR((LM_ERROR,
                ACE_TEXT("(%P|%t) ERROR: BitPubListenerImpl::on_data_available:")
                ACE_TEXT(" unknown instance state: %d\n"),
                si.instance_state));
        }
      } else if (status != DDS::RETCODE_NO_DATA) {
        ACE_ERROR((LM_ERROR,
                   ACE_TEXT("(%P|%t) ERROR: BitPubListenerImpl::on_data_available:")
                   ACE_TEXT(" unexpected status: %d\n"),
                   status));
      }
    } while (status == DDS::RETCODE_OK);

  } catch (const CORBA::Exception& e) {
    e._tao_print_exception("Exception caught in BitPubListenerImpl::on_data_available():");
  }
}

void BitPubListenerImpl::on_requested_deadline_missed(
  DDS::DataReader_ptr,
  const DDS::RequestedDeadlineMissedStatus &)
{
}

void BitPubListenerImpl::on_requested_incompatible_qos(
  DDS::DataReader_ptr,
  const DDS::RequestedIncompatibleQosStatus &)
{
}

void BitPubListenerImpl::on_liveliness_changed(
  DDS::DataReader_ptr,
  const DDS::LivelinessChangedStatus &)
{
}

void BitPubListenerImpl::on_subscription_matched(
  DDS::DataReader_ptr,
  const DDS::SubscriptionMatchedStatus &)
{
}

void BitPubListenerImpl::on_sample_rejected(
  DDS::DataReader_ptr,
  const DDS::SampleRejectedStatus&)
{
}

void BitPubListenerImpl::on_sample_lost(
  DDS::DataReader_ptr,
  const DDS::SampleLostStatus&)
{
}

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif // DDS_HAS_MINIMUM_BIT
