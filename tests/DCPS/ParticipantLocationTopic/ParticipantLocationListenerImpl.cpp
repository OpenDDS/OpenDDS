/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "ParticipantLocationListenerImpl.h"
#include <dds/OpenddsDcpsExtTypeSupportImpl.h>
#include <ace/streams.h>
#include <string>

// Implementation skeleton constructor
ParticipantLocationListenerImpl::ParticipantLocationListenerImpl(const std::string& id,
                                                                 bool noice,
                                                                 bool ipv6,
                                                                 callback_t done_callback)
  : id_(id)
  , no_ice_(noice)
  , ipv6_(ipv6)
  , done_callback_(done_callback)
  , done_(false)
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
    << ((participant.location & OpenDDS::DCPS::LOCATION_LOCAL6) ? "LOCAL6 " : "")
    << ((participant.location & OpenDDS::DCPS::LOCATION_ICE6) ? "ICE6 " : "")
    << ((participant.location & OpenDDS::DCPS::LOCATION_RELAY6) ? "RELAY6 " : "")
    << std::endl
    << "  mask: "
    << ((participant.change_mask & OpenDDS::DCPS::LOCATION_LOCAL) ? "LOCAL " : "")
    << ((participant.change_mask & OpenDDS::DCPS::LOCATION_ICE) ? "ICE " : "")
    << ((participant.change_mask & OpenDDS::DCPS::LOCATION_RELAY) ? "RELAY " : "")
    << ((participant.change_mask & OpenDDS::DCPS::LOCATION_LOCAL6) ? "LOCAL6 " : "")
    << ((participant.change_mask & OpenDDS::DCPS::LOCATION_ICE6) ? "ICE6 " : "")
    << ((participant.change_mask & OpenDDS::DCPS::LOCATION_RELAY6) ? "RELAY6 " : "")
    << std::endl
    << " local: " << participant.local_addr << std::endl
    << "      : " << participant.local_timestamp.sec << std::endl
    << "   ice: " << participant.ice_addr << std::endl
    << "      : " << participant.ice_timestamp.sec << std::endl
    << " relay: " << participant.relay_addr << std::endl
    << "      : " << participant.relay_timestamp.sec << std::endl
    << "local6: " << participant.local6_addr << std::endl
    << "      : " << participant.local6_timestamp.sec << std::endl
    << "  ice6: " << participant.ice6_addr << std::endl
    << "      : " << participant.ice6_timestamp.sec << std::endl
    << "relay6: " << participant.relay6_addr << std::endl
    << "      : " << participant.relay6_timestamp.sec << std::endl;

    // update locations if SampleInfo is valid.
    if (si.valid_data == 1)
    {
      std::pair<LocationMapType::iterator, bool> p = location_map.insert(std::make_pair(guid, 0));
      p.first->second |= participant.location;
    }

    if (!done_ && check(true)) {
      done_ = true;
      std::cout << "== " << id_ << " Participant received all expected locations" << std::endl;
      done_callback_();
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

bool ParticipantLocationListenerImpl::check(bool print_results)
{
  const unsigned long expected =
    OpenDDS::DCPS::LOCATION_LOCAL
    | (ipv6_ ? OpenDDS::DCPS::LOCATION_RELAY6 : OpenDDS::DCPS::LOCATION_RELAY)
#ifdef ACE_HAS_IPV6
    | OpenDDS::DCPS::LOCATION_LOCAL6
    | (!no_ice_ ? OpenDDS::DCPS::LOCATION_ICE6 : 0)
#else
    | (!no_ice_ && !ipv6_? OpenDDS::DCPS::LOCATION_ICE : 0)
#endif
    ;

  if (print_results) {
    std::cout << id_ << " expecting "
              << " LOCAL"
              << ((expected & OpenDDS::DCPS::LOCATION_ICE) ? " ICE" : "")
              << ((expected & OpenDDS::DCPS::LOCATION_RELAY) ? " RELAY" : "")
#ifdef ACE_HAS_IPV6
              << ((expected & OpenDDS::DCPS::LOCATION_LOCAL6) ? " LOCAL6" : "")
#endif
              << ((expected & OpenDDS::DCPS::LOCATION_ICE6) ? " ICE6" : "")
              << ((expected & OpenDDS::DCPS::LOCATION_RELAY6) ? " RELAY6" : "")
              << std::endl;
  }

  bool found = false;
  for (LocationMapType::const_iterator pos = location_map.begin(), limit = location_map.end();
       pos != limit; ++ pos) {
    if (print_results) {
      std::cout << id_ << " " << pos->first
                << ((pos->second & OpenDDS::DCPS::LOCATION_LOCAL) ? " LOCAL" : "")
                << ((pos->second & OpenDDS::DCPS::LOCATION_ICE) ? " ICE" : "")
                << ((pos->second & OpenDDS::DCPS::LOCATION_RELAY) ? " RELAY" : "")
                << ((pos->second & OpenDDS::DCPS::LOCATION_LOCAL6) ? " LOCAL6" : "")
                << ((pos->second & OpenDDS::DCPS::LOCATION_ICE6) ? " ICE6" : "")
                << ((pos->second & OpenDDS::DCPS::LOCATION_RELAY6) ? " RELAY6" : "")
                << std::endl;
    }
    found = found || pos->second == expected;
  }
  return found;
}
