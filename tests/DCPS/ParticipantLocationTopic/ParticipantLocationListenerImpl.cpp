/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "ParticipantLocationListenerImpl.h"
#include <dds/DdsDcpsCoreTypeSupportImpl.h>
#include <ace/streams.h>
#include <string>

// Implementation skeleton constructor
ParticipantLocationListenerImpl::ParticipantLocationListenerImpl(const std::string& id) :
  id_(id)
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

    std::cout << "== " << id_ << " Participant Location ==" << std::endl;
    std::cout
    << " valid: " << (si.valid_data == 1 ? "true" : "false") << std::endl
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
    << "      : " << participant.local_timestamp.sec << std::endl
    << "   ice: " << participant.ice_addr << std::endl
    << "      : " << participant.ice_timestamp.sec << std::endl
    << " relay: " << participant.relay_addr << std::endl
    << "      : " << participant.relay_timestamp.sec << std::endl;

    // update locations if SampleInfo is valid.
    if (si.valid_data == 1)
    {
      std::pair<LocationMapType::iterator, bool> p = location_map.insert(std::make_pair(guid, 0));
      p.first->second |= participant.location;
    }
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

bool ParticipantLocationListenerImpl::check(bool no_ice)
{
  const unsigned long expected =
    OpenDDS::DCPS::LOCATION_LOCAL |
    (!no_ice ? OpenDDS::DCPS::LOCATION_ICE : 0) |
    OpenDDS::DCPS::LOCATION_RELAY;

  std::cout << id_ << " expecting "
            << " LOCAL"
            << ((expected & OpenDDS::DCPS::LOCATION_ICE) ? " ICE" : "")
            << " RELAY"
            << std::endl;

  bool found = false;
  for (LocationMapType::const_iterator pos = location_map.begin(), limit = location_map.end();
       pos != limit; ++ pos) {
    std::cout << id_ << " " << pos->first
              << ((pos->second & OpenDDS::DCPS::LOCATION_LOCAL) ? " LOCAL" : "")
              << ((pos->second & OpenDDS::DCPS::LOCATION_ICE) ? " ICE" : "")
              << ((pos->second & OpenDDS::DCPS::LOCATION_RELAY) ? " RELAY" : "")
              << std::endl;
    found = found || pos->second == expected;
  }
  return found;
}
