// -*- C++ -*-
// ============================================================================
/**
 *  @file   publisher.cpp
 *
 *
 *
 */
// ============================================================================

#include "Domain.h"
#include "DataWriterListenerImpl.h"

#include "../common/TestException.h"
#include "dds/DCPS/Service_Participant.h"
#include "dds/DCPS/Marked_Default_Qos.h"
#include "dds/DCPS/Qos_Helper.h"
#include "dds/DCPS/PublisherImpl.h"
#include "tests/DCPS/FooType4/FooDefTypeSupportImpl.h"

#include "dds/DCPS/StaticIncludes.h"
#ifdef ACE_AS_STATIC_LIBS
#include <dds/DCPS/RTPS/RtpsDiscovery.h>
#include <dds/DCPS/transport/rtps_udp/RtpsUdp.h>
#endif

#include "ace/Reactor.h"

#include <cmath>

class Publisher
{
public:
  Publisher(int argc, ACE_TCHAR* argv[]);
  int run();
private:
  Domain domain_;
  DDS::DataWriterListener_var listener_;
  DDS::DataWriter_var writer_;
};

Publisher::Publisher(int argc, ACE_TCHAR* argv[]) : domain_(argc, argv, "Publisher")
{
  DDS::Publisher_var pub = domain_.participant_->create_publisher(PUBLISHER_QOS_DEFAULT,
    DDS::PublisherListener::_nil(), OpenDDS::DCPS::DEFAULT_STATUS_MASK);
  if (!pub) {
    throw ACE_TEXT("(%P|%t) create_publisher failed.");
  }

  DDS::DataWriterQos qos;
  pub->get_default_datawriter_qos(qos);
  qos.liveliness.lease_duration.sec = domain_.lease_duration_sec_;
  qos.liveliness.lease_duration.nanosec = 0;

  listener_ = new DataWriterListenerImpl;
  writer_ = pub->create_datawriter(domain_.topic_.in(), qos, listener_.in(), OpenDDS::DCPS::DEFAULT_STATUS_MASK);
  if (!writer_) {
    throw ACE_TEXT("(%P|%t) create_datawriter failed.");
  }
}

int Publisher::run()
{
  ACE_OS::sleep(domain_.test_duration_sec_);
  // check to see if the publisher worked
  DataWriterListenerImpl* dwl = dynamic_cast<DataWriterListenerImpl*>(listener_.in());
  if (!dwl) {
    ACE_ERROR((LM_ERROR,
      ACE_TEXT("(%P|%t) publisher didn't obtain DataWriterListenerImpl. TEST_DURATION_SEC=%.1f\n"),
      static_cast<float>(domain_.test_duration_sec_.sec()) + (static_cast<float>(domain_.test_duration_sec_.usec()) / 1e6f)));
    return 1;
  }
  if (!dwl->valid()) {
    ACE_ERROR ((LM_ERROR,
      ACE_TEXT("(%P|%t) publisher didn't connect with subscriber. TEST_DURATION_SEC=%.1f\n"),
      static_cast<float>(domain_.test_duration_sec_.sec()) + (static_cast<float>(domain_.test_duration_sec_.usec()) / 1e6f)));
    return 1;
  }
  return 0;
}

int ACE_TMAIN(int argc, ACE_TCHAR* argv[])
{
  try {
    Publisher pub(argc, argv);
    return pub.run();
  } catch (const CORBA::Exception& ex) {
    ex._tao_print_exception("Exception caught in main.cpp:");
    return 1;
  } catch (...) {
    ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) Exception caught in main.cpp")));
    return 1;
  }
}
