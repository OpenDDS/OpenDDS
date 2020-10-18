// -*- C++ -*-
// ============================================================================
/**
 *  @file   subscriber.cpp
 *
 *
 *
 */
// ============================================================================

#include "Domain.h"
#include "DataReaderListenerImpl.h"

#include "../common/TestException.h"
#include "dds/DCPS/Service_Participant.h"
#include "dds/DCPS/Marked_Default_Qos.h"
#include "dds/DCPS/Qos_Helper.h"
#include "dds/DCPS/TopicDescriptionImpl.h"
#include "dds/DCPS/SubscriberImpl.h"
#include "dds/DdsDcpsSubscriptionC.h"
#include "tests/DCPS/FooType4/FooDefTypeSupportImpl.h"

#include "dds/DCPS/StaticIncludes.h"
#ifdef ACE_AS_STATIC_LIBS
#include <dds/DCPS/RTPS/RtpsDiscovery.h>
#include <dds/DCPS/transport/rtps_udp/RtpsUdp.h>
#endif

#include <cmath>

class Subscriber {
public:
  Subscriber(int argc, ACE_TCHAR* argv[]);
  int run();
private:
  Domain domain_;
  DataReaderListenerImpl* listenerImpl_;
  DDS::DataReaderListener_var listener_;
};

Subscriber::Subscriber(int argc, ACE_TCHAR* argv[]) : domain_(argc, argv, "Subscriber")
{
  DDS::Subscriber_var sub = domain_.participant_->create_subscriber(SUBSCRIBER_QOS_DEFAULT,
    DDS::SubscriberListener::_nil(), OpenDDS::DCPS::DEFAULT_STATUS_MASK);
  if (!sub) {
    throw ACE_TEXT("(%P|%t) create_subscriber failed.");
  }

  DDS::TopicDescription_var td = domain_.participant_->lookup_topicdescription(Domain::TOPIC);
  if (!td) {
    throw ACE_TEXT("(%P|%t) lookup_topicdescription failed.\n");
  }

  DDS::DataReaderQos qos;
  sub->get_default_datareader_qos(qos);
  qos.liveliness.lease_duration.sec = domain_.lease_duration_sec_;
  qos.liveliness.lease_duration.nanosec = 0;

  listenerImpl_ = new DataReaderListenerImpl;
  listener_ = listenerImpl_;
  if (!listenerImpl_) {
    throw ACE_TEXT("(%P|%t) failed to create DataReaderListenerImpl.");
  }

  DDS::DataReader_var dr = sub->create_datareader(
    td.in(), qos, listener_.in(), OpenDDS::DCPS::DEFAULT_STATUS_MASK);
}

int Subscriber::run()
{
  ACE_OS::sleep(domain_.test_duration_sec_);
  if( listenerImpl_->deadline_missed() < domain_.threshold_liveliness_lost_) {
    ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) subscriber: ")
      ACE_TEXT("liviness deadline not violated enough for test. ")
      ACE_TEXT("Got %d, expected at least %d.\n"),
      listenerImpl_->deadline_missed(), domain_.threshold_liveliness_lost_));
    return 1;
  } else {
    ACE_DEBUG((LM_INFO, ACE_TEXT("(%P|%t) subscriber: ")
      ACE_TEXT("liviness deadline violated enough for test. ")
      ACE_TEXT("Got %d, expected at least %d.\n"),
      listenerImpl_->deadline_missed(), domain_.threshold_liveliness_lost_));
  }
  return 0;
}

int ACE_TMAIN(int argc, ACE_TCHAR* argv[])
{
  try {
    Subscriber sub(argc, argv);
    return sub.run();
  } catch (const CORBA::Exception& ex) {
    ex._tao_print_exception("Exception caught in main.cpp:");
    return 1;
  } catch (...) {
    ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) Exception caught in main.cpp")));
    return 1;
  }
}
