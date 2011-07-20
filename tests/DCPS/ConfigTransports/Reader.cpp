#include "Reader.h"
#include "../common/TestSupport.h"

Reader::Reader(DDS::DomainParticipantFactory_ptr factory, DDS::DataReaderListener_ptr listener) :
dpf(DDS::DomainParticipantFactory::_duplicate(factory)),
dp(create_configured_participant(dpf.in())),
topic(create_configured_topic(dp.in())),
sub(create_configured_subscriber(dp.in())),
dr(create_configured_reader(sub.in(), topic.in(), listener)) { }

Reader::Reader(DDS::DomainParticipantFactory_ptr factory, DDS::DomainParticipant_ptr participant, DDS::DataReaderListener_ptr listener) :
dpf(DDS::DomainParticipantFactory::_duplicate(factory)),
dp(DDS::DomainParticipant::_duplicate(participant)),
topic(create_configured_topic(dp.in())),
sub(create_configured_subscriber(dp.in())),
dr(create_configured_reader(sub.in(), topic.in(), listener)) { }

Reader::~Reader()
{
  // Clean up subscriber objects
  sub->delete_contained_entities();
  dp->delete_subscriber(sub.in());
  dp->delete_topic(topic.in());
  dpf->delete_participant(dp.in());
}

bool
Reader::verify_transport()
{
  TEST_ASSERT(!CORBA::is_nil(dr.in()));

  // Wait for things to settle ?!
  ACE_OS::sleep(test_duration);

  // All required protocols must have been found
  return assert_supports_all(dr.in(), protocol_str);
}
