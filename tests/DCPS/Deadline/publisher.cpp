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
#include "Writer.h"

#include <tests/Utils/ExceptionStreams.h>

#include <dds/DCPS/Marked_Default_Qos.h>
#include <dds/DCPS/PublisherImpl.h>
#include <dds/DCPS/Qos_Helper.h>
#include <dds/DCPS/StaticIncludes.h>
#ifdef ACE_AS_STATIC_LIBS
#  include <dds/DCPS/RTPS/RtpsDiscovery.h>
#  include <dds/DCPS/transport/rtps_udp/RtpsUdp.h>
#endif

#include <ace/streams.h>
#include <ace/OS_NS_unistd.h>
#include <ace/Get_Opt.h>

#include <iostream>
#include <memory>
#include <stdexcept>

class Publisher
{
public:
  Publisher(int argc, ACE_TCHAR* argv[]);
  ~Publisher() { cleanup(); }
  int run();

private:
  static const int N_WriteThread = 2;
  void cleanup();
  bool set_deadline_qos();
  bool check_status(const CORBA::Long count);

  Domain domain_;
  DDS::Publisher_var pub_;
  DataWriterListenerImpl* listener_i_;
  DDS::DataWriterListener_var listener_;
  DDS::DataWriter_var dw_;
};

Publisher::Publisher(int argc, ACE_TCHAR* argv[]) : domain_(argc, argv, "Publisher")
{
  ACE_DEBUG((LM_INFO, ACE_TEXT("(%P|%t) Publisher::Publisher\n")));
  try {
    pub_ = domain_.dp()->create_publisher(PUBLISHER_QOS_DEFAULT, 0, OpenDDS::DCPS::DEFAULT_STATUS_MASK);
    if (!pub_) {
      throw std::runtime_error("create_publisher failed.");
    }

    listener_i_ = new DataWriterListenerImpl;
    listener_ = listener_i_;
    if (!listener_) {
      throw std::runtime_error("DataWriterListenerImpl is null.");
    }

    dw_ = pub_->create_datawriter(domain_.topic().in(), DATAWRITER_QOS_DEFAULT, listener_.in(), OpenDDS::DCPS::DEFAULT_STATUS_MASK);
    if (!dw_) {
      throw std::runtime_error("create_datawriter is null.");
    }
  } catch (...) {
    ACE_ERROR((LM_ERROR, "ERROR: Publisher::Publisher failed"));
    cleanup();
    throw;
  }
}

int Publisher::run()
{
  if (!set_deadline_qos()) {
    return 1;
  }

  ACE_DEBUG((LM_INFO, ACE_TEXT("(%P|%t) Publisher::run: wait_matched\n")));
  if (!listener_i_->wait_matched(2, OpenDDS::DCPS::TimeDuration(10))) {
    ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: Publisher::run wait_matched failed.\n")));
    return 1;
  }

  Writer writer(dw_);
  if (!writer.start()) {
    return 1;
  }

  ACE_DEBUG((LM_INFO, ACE_TEXT("(%P|%t) Publisher::run sleep for %d milliseconds\n"), Domain::w_sleep.value().msec()));
  ACE_OS::sleep(Domain::w_sleep.value()); // wait for a set of deadline periods to expire
  CORBA::Long expected = Domain::n_expiration * N_WriteThread;
  if (!check_status(expected)) {
    return 1;
  }

  writer.end();

  ACE_DEBUG((LM_INFO, ACE_TEXT("(%P|%t) Publisher::run sleep for %d milliseconds\n"), Domain::w_sleep.value().msec()));
  ACE_OS::sleep(Domain::w_sleep.value()); // wait for another set of deadline periods to expire
  expected = (Domain::n_expiration + 2) * N_WriteThread;
  if (!check_status(expected)) {
    return 1;
  }

  // Wait for datareader finish.
  while (1) {
    DDS::InstanceHandleSeq handles;
    dw_->get_matched_subscriptions(handles);
    if (handles.length() == 0)
      break;
    else
      ACE_OS::sleep(1);
  }
  return 0;
}

void Publisher::cleanup()
{
  if (dw_) dw_ = 0;
  if (listener_) listener_ = 0;
  if (pub_) pub_ = 0;
}

bool Publisher::set_deadline_qos()
{
  ACE_DEBUG((LM_INFO, ACE_TEXT("(%P|%t) Publisher::set_deadline_qos (%d sec)\n"), Domain::w_deadline.sec));
  DDS::DataWriterQos qos;
  pub_->get_default_datawriter_qos(qos);
  qos.deadline.period.sec = Domain::w_deadline.sec;
  qos.deadline.period.nanosec = Domain::w_deadline.nanosec;
  if (dw_->set_qos(qos) != DDS::RETCODE_OK) {
    ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: dw_->set_qos failed.\n")));
    return false;
  }
  return true;
}

bool Publisher::check_status(const CORBA::Long count)
{
  ACE_DEBUG((LM_INFO, ACE_TEXT("(%P|%t) Publisher::check_status count(%d)\n"), count));
  DDS::OfferedDeadlineMissedStatus s;
  if (dw_->get_offered_deadline_missed_status(s) != DDS::RETCODE_OK) {
    ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: get_offered_deadline_missed_status failed.\n")));
    return false;
  }
  if (s.total_count != count) {
    ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: offered_deadline_missed (%d) != (%d)\n"), s.total_count, count));
    return false;
  }
  // Check if the total count changed is correctly giving the change between
  // the last time our listener got invoked and our manual call now
  const CORBA::Long change = s.total_count - listener_i_->offered_deadline_total_count();
  if (s.total_count_change != change) {
    ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: offered_deadline_missed change (%d) != (%d)\n"), s.total_count_change, change));
    return false;
  }
  return true;
}

int ACE_TMAIN(int argc, ACE_TCHAR *argv[])
{
  int ret = 1;
  try {
    Publisher pub(argc, argv);
    ret = pub.run();
  } catch (const CORBA::Exception& e) {
    e._tao_print_exception("Publisher ERROR: ");
  } catch (const std::exception& e) {
    ACE_ERROR((LM_ERROR, ACE_TEXT("Publisher ERROR: %C\n"), e.what()));
  } catch (...) {
    ACE_ERROR((LM_ERROR, ACE_TEXT("Publisher ERROR: ...\n")));
  }
  TheServiceParticipant->shutdown();
  return ret;
}
