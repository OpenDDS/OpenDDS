/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "ParticipantLocationBuiltinTopicDataDataReaderListenerImpl.h"
#include <dds/DdsDcpsCoreTypeSupportC.h>
#include <ace/streams.h>
#include <string>

// Implementation skeleton constructor
ParticipantLocationBuiltinTopicDataDataReaderListenerImpl::ParticipantLocationBuiltinTopicDataDataReaderListenerImpl(unsigned long& locations) :
  location_mask(locations)
{
}

// Implementation skeleton destructor
ParticipantLocationBuiltinTopicDataDataReaderListenerImpl::~ParticipantLocationBuiltinTopicDataDataReaderListenerImpl()
{
}

void ParticipantLocationBuiltinTopicDataDataReaderListenerImpl::on_data_available(DDS::DataReader_ptr reader)
{
  // 1.  Narrow the DataReader to an ParticipantLocationBuiltinTopicDataDataReader
  // 2.  Read the samples from the data reader
  // 3.  Print out the contents of the samples
  DDS::ParticipantLocationBuiltinTopicDataDataReader_var builtin_dr =
    DDS::ParticipantLocationBuiltinTopicDataDataReader::_narrow(reader);
  if (0 == builtin_dr)
    {
      std::cerr << "ParticipantLocationBuiltinTopicDataDataReaderListenerImpl::"
                << "on_data_available: _narrow failed." << std::endl;
      ACE_OS::exit(1);
    }

  DDS::ParticipantLocationBuiltinTopicData participant;
  DDS::SampleInfo si;

  for (DDS::ReturnCode_t status = builtin_dr->read_next_sample(participant, si);
       status == DDS::RETCODE_OK;
       status = builtin_dr->read_next_sample(participant, si)) {
    std::cout << "== Participant Location ==" << std::endl;
    std::cout
    << " valid: " << si.valid_data << std::endl
    << "  guid: " << participant.guid << std::endl
    << "   loc: "
    << ((participant.location & DDS::LOCATION_LOCAL) ? "LOCAL " : "")
    << ((participant.location & DDS::LOCATION_ICE) ? "ICE " : "")
    << ((participant.location & DDS::LOCATION_RELAY) ? "RELAY " : "")
    << std::endl
    << "  mask: "
    << ((participant.change_mask & DDS::LOCATION_LOCAL) ? "LOCAL " : "")
    << ((participant.change_mask & DDS::LOCATION_ICE) ? "ICE " : "")
    << ((participant.change_mask & DDS::LOCATION_RELAY) ? "RELAY " : "")
    << std::endl
    << " local: " << participant.local_addr << std::endl
    << "      : " << participant.local_timestamp << std::endl
    << "   ice: " << participant.ice_addr << std::endl
    << "      : " << participant.ice_timestamp << std::endl
    << " relay: " << participant.relay_addr << std::endl
    << "      : " << participant.relay_timestamp << std::endl;

    // update locations seen
    location_mask |= participant.change_mask;
  }
}

void ParticipantLocationBuiltinTopicDataDataReaderListenerImpl::on_requested_deadline_missed(
  DDS::DataReader_ptr,
  const DDS::RequestedDeadlineMissedStatus &)
{
  std::cerr << "ParticipantLocationBuiltinTopicDataDataReaderListenerImpl::"
    << "on_requested_deadline_missed" << std::endl;
}

void ParticipantLocationBuiltinTopicDataDataReaderListenerImpl::on_requested_incompatible_qos(
  DDS::DataReader_ptr,
  const DDS::RequestedIncompatibleQosStatus &)
{
  std::cerr << "ParticipantLocationBuiltinTopicDataDataReaderListenerImpl::"
    << "on_requested_incompatible_qos" << std::endl;
}

void ParticipantLocationBuiltinTopicDataDataReaderListenerImpl::on_liveliness_changed(
  DDS::DataReader_ptr,
  const DDS::LivelinessChangedStatus&)
{
  std::cerr << "ParticipantLocationBuiltinTopicDataDataReaderListenerImpl::"
    << "on_liveliness_changed" << std::endl;
}

void ParticipantLocationBuiltinTopicDataDataReaderListenerImpl::on_subscription_matched(
  DDS::DataReader_ptr,
  const DDS::SubscriptionMatchedStatus &)
{
  std::cerr << "ParticipantLocationBuiltinTopicDataDataReaderListenerImpl::"
    << "on_subscription_matched" << std::endl;
}

void ParticipantLocationBuiltinTopicDataDataReaderListenerImpl::on_sample_rejected(
  DDS::DataReader_ptr,
  const DDS::SampleRejectedStatus&)
{
  std::cerr << "ParticipantLocationBuiltinTopicDataDataReaderListenerImpl::"
    << "on_sample_rejected" << std::endl;
}

void ParticipantLocationBuiltinTopicDataDataReaderListenerImpl::on_sample_lost(
  DDS::DataReader_ptr,
  const DDS::SampleLostStatus&)
{
  std::cerr << "ParticipantLocationBuiltinTopicDataDataReaderListenerImpl::"
    << "on_sample_lost" << std::endl;
}
