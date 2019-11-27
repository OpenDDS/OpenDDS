/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "ParticipantLocationListenerImpl.h"
#include <dds/DdsDcpsCoreTypeSupportC.h>
#include <ace/streams.h>
#include <string>

// Implementation skeleton constructor
ParticipantLocationListenerImpl::ParticipantLocationListenerImpl(unsigned long& locations) :
  location_mask(locations)
{
}

// Implementation skeleton destructor
ParticipantLocationListenerImpl::~ParticipantLocationListenerImpl()
{
}

void ParticipantLocationListenerImpl::on_data_available(DDS::DataReader_ptr reader)
{
  // 1.  Narrow the DataReader to an ParticipantLocationBuiltinTopicDataDataReader
  // 2.  Read the samples from the data reader
  // 3.  Print out the contents of the samples
  OpenDDS::DCPS::ParticipantLocationBuiltinTopicDataDataReader_var builtin_dr =
    OpenDDS::DCPS::ParticipantLocationBuiltinTopicDataDataReader::_narrow(reader);
  if (0 == builtin_dr)
    {
      std::cerr << "ParticipantLocationListenerImpl::"
                << "on_data_available: _narrow failed." << std::endl;
      ACE_OS::exit(1);
    }

  OpenDDS::DCPS::ParticipantLocationBuiltinTopicData participant;
  DDS::SampleInfo si;

  for (DDS::ReturnCode_t status = builtin_dr->read_next_sample(participant, si);
       status == DDS::RETCODE_OK;
       status = builtin_dr->read_next_sample(participant, si)) {

    // copy octet[] to guid
    OpenDDS::DCPS::RepoId guid;
    std::memcpy(&guid, &participant.guid, sizeof(guid));

    std::cout << "== Participant Location ==" << std::endl;
    std::cout
    << " valid: " << si.valid_data << std::endl
    << "  guid: " << guid << std::endl
    << "   loc: "
    << ((participant.location & OpenDDS::DCPS::LOCATION_LOCAL) ? "LOCAL " : "")
    << ((participant.location & OpenDDS::DCPS::LOCATION_ICE) ? "ICE " : "")
    << ((participant.location & OpenDDS::DCPS::LOCATION_RELAY) ? "RELAY " : "")
    << std::endl
    << "  mask: "
    << ((participant.change_mask & OpenDDS::DCPS::LOCATION_LOCAL) ? "LOCAL " : "")
    << ((participant.change_mask & OpenDDS::DCPS::LOCATION_ICE) ? "ICE " : "")
    << ((participant.change_mask & OpenDDS::DCPS::LOCATION_RELAY) ? "RELAY " : "")
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

void ParticipantLocationListenerImpl::on_requested_deadline_missed(
  DDS::DataReader_ptr,
  const DDS::RequestedDeadlineMissedStatus &)
{
  std::cerr << "ParticipantLocationListenerImpl::"
    << "on_requested_deadline_missed" << std::endl;
}

void ParticipantLocationListenerImpl::on_requested_incompatible_qos(
  DDS::DataReader_ptr,
  const DDS::RequestedIncompatibleQosStatus &)
{
  std::cerr << "ParticipantLocationListenerImpl::"
    << "on_requested_incompatible_qos" << std::endl;
}

void ParticipantLocationListenerImpl::on_liveliness_changed(
  DDS::DataReader_ptr,
  const DDS::LivelinessChangedStatus&)
{
  std::cerr << "ParticipantLocationListenerImpl::"
    << "on_liveliness_changed" << std::endl;
}

void ParticipantLocationListenerImpl::on_subscription_matched(
  DDS::DataReader_ptr,
  const DDS::SubscriptionMatchedStatus &)
{
  std::cerr << "ParticipantLocationListenerImpl::"
    << "on_subscription_matched" << std::endl;
}

void ParticipantLocationListenerImpl::on_sample_rejected(
  DDS::DataReader_ptr,
  const DDS::SampleRejectedStatus&)
{
  std::cerr << "ParticipantLocationListenerImpl::"
    << "on_sample_rejected" << std::endl;
}

void ParticipantLocationListenerImpl::on_sample_lost(
  DDS::DataReader_ptr,
  const DDS::SampleLostStatus&)
{
  std::cerr << "ParticipantLocationListenerImpl::"
    << "on_sample_lost" << std::endl;
}
