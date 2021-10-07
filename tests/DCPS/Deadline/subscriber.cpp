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
#include "MessengerTypeSupportImpl.h"
#include <dds/DCPS/Service_Participant.h>
#include <dds/DCPS/Marked_Default_Qos.h>
#include <dds/DCPS/SubscriberImpl.h>
#include <dds/DCPS/Qos_Helper.h>
#include "dds/DCPS/WaitSet.h"
#include "dds/DCPS/TimeTypes.h"
#include "dds/DCPS/DCPS_Utils.h"
#include "dds/DdsDcpsInfrastructureC.h"

#include "dds/DCPS/StaticIncludes.h"
#ifdef ACE_AS_STATIC_LIBS
#include <dds/DCPS/RTPS/RtpsDiscovery.h>
#include <dds/DCPS/transport/rtps_udp/RtpsUdp.h>
#endif

#include <ace/streams.h>
#include "tests/Utils/ExceptionStreams.h"
#include <ace/Time_Value.h>
#include <ace/OS_NS_unistd.h>

#include <stdexcept>

class Subscriber
{
public:
  Subscriber(int argc, ACE_TCHAR* argv[]);
  ~Subscriber() { cleanup(); }
  int run();

private:
  void cleanup();
  void wait_incompatible_qos_status(DDS::StatusCondition_var cond) const;
  void test_incompatible_deadline_qos();
  bool check_status(const DDS::InstanceHandle_t& i1r1, const DDS::InstanceHandle_t& i1r2,
                    const DDS::InstanceHandle_t& i2r1, const DDS::InstanceHandle_t& i2r2,
                    const CORBA::Long expected, CORBA::Long& delta1);
  bool get_status(DDS::RequestedDeadlineMissedStatus& s1, DDS::RequestedDeadlineMissedStatus& s2);
  bool check_instance(const int dr, const DDS::InstanceHandle_t& last, const DDS::InstanceHandle_t& i1, const DDS::InstanceHandle_t& i2) const;
  bool check_change(DDS::RequestedDeadlineMissedStatus& s1, DDS::RequestedDeadlineMissedStatus& s2, CORBA::Long& delta1) const;

  Domain domain_;
  DDS::Subscriber_var sub_;
  DataReaderListenerImpl* listener_i_;
  DDS::DataReaderListener_var listener_;
  DDS::DataReader_var dr1_;
  DDS::DataReader_var dr2_;
};

Subscriber::Subscriber(int argc, ACE_TCHAR* argv[]) : domain_(argc, argv, "Subscriber")
{
  ACE_DEBUG((LM_INFO, ACE_TEXT("(%P|%t) Subscriber::Subscriber\n")));
  try {
    sub_ = domain_.dp()->create_subscriber(SUBSCRIBER_QOS_DEFAULT, 0, OpenDDS::DCPS::DEFAULT_STATUS_MASK);
    if (!sub_) {
      throw std::runtime_error("create_subscriber failed.");
    }

    test_incompatible_deadline_qos();

    listener_i_ = new DataReaderListenerImpl;
    listener_ = listener_i_;
    if (!listener_) {
      throw std::runtime_error("DataReaderListenerImpl is nil.");
    }

    DDS::DataReaderQos qos;
    sub_->get_default_datareader_qos(qos);
    qos.deadline.period.sec = Domain::r_deadline.sec;
    qos.deadline.period.nanosec = Domain::r_deadline.nanosec;
    qos.reliability.kind = DDS::RELIABLE_RELIABILITY_QOS;

    ACE_DEBUG((LM_INFO, ACE_TEXT("(%P|%t) create 2 readers with deadline.period (%d sec)\n"), qos.deadline.period.sec));
    // To test deadline expiration handling without/with listener callback
    dr1_ = sub_->create_datareader(domain_.topic().in(), qos, 0, OpenDDS::DCPS::DEFAULT_STATUS_MASK);
    dr2_ = sub_->create_datareader(domain_.topic().in(), qos, listener_.in(), OpenDDS::DCPS::DEFAULT_STATUS_MASK);
    if (!dr1_ || !dr2_) {
      throw std::runtime_error("create_datareader failed.");
    }
  } catch (...) {
    ACE_ERROR((LM_ERROR, "ERROR: Subscriber::Subscriber failed"));
    cleanup();
    throw;
  }
}

int Subscriber::run()
{
  ACE_DEBUG((LM_INFO, ACE_TEXT("(%P|%t) Subscriber::run wait_matched\n")));
  if (!listener_i_->wait_matched(1, OpenDDS::DCPS::TimeDuration(10))) {
    ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: Subscriber wait_matched failed.\n")));
    return 1;
  }

  ACE_DEBUG((LM_INFO, ACE_TEXT("(%P|%t) Subscriber sleep for %d milliseconds\n"), Domain::r_sleep.value().msec()));
  ACE_OS::sleep(Domain::r_sleep.value()); // Wait for deadline periods to expire

  Messenger::MessageDataReader_var mdr1 = Messenger::MessageDataReader::_narrow(dr1_.in());
  Messenger::MessageDataReader_var mdr2 = Messenger::MessageDataReader::_narrow(dr2_.in());
  Messenger::Message msg;
  msg.subject_id = Domain::s_key1;
  const DDS::InstanceHandle_t ins1r1 = mdr1->lookup_instance(msg);
  const DDS::InstanceHandle_t ins1r2 = mdr2->lookup_instance(msg);
  msg.subject_id = Domain::s_key2;
  const DDS::InstanceHandle_t ins2r1 = mdr1->lookup_instance(msg);
  const DDS::InstanceHandle_t ins2r2 = mdr2->lookup_instance(msg);

  CORBA::Long delta1 = 0;
  if (!check_status(ins1r1, ins1r2, ins2r1, ins2r2, Domain::n_instance, delta1)) {
    return 1;
  }

  const OpenDDS::DCPS::TimeDuration no_miss_period = Domain::r_sleep + Domain::n_msg * Domain::w_interval;
  ACE_DEBUG((LM_INFO, ACE_TEXT("(%P|%t) Subscriber sleep for %d msec\n"), no_miss_period.value().msec()));
  // Wait for another set of deadline periods(5 + 11 secs).
  // During this period, the writers continue to write all samples with .5 second interval.
  ACE_OS::sleep(no_miss_period.value());
  if (!check_status(ins1r1, ins1r2, ins2r1, ins2r2, 3 * Domain::n_instance, delta1)) {
    return 1;
  }

  return listener_i_->wait_all_received();
}

void Subscriber::cleanup()
{
  if (dr2_) dr2_ = 0;
  if (dr1_) dr1_ = 0;
  if (listener_) listener_ = 0;
  if (sub_) sub_ = 0;
}

void Subscriber::wait_incompatible_qos_status(DDS::StatusCondition_var cond) const
{
  ACE_DEBUG((LM_INFO, ACE_TEXT("(%P|%t) Subscriber::wait_incompatible_qos_status\n")));
  cond->set_enabled_statuses(DDS::REQUESTED_INCOMPATIBLE_QOS_STATUS);
  DDS::WaitSet_var ws = new DDS::WaitSet;
  const DDS::Duration_t four_sec = {4, 0};
  DDS::ConditionSeq active;
  std::string e;
  ws->attach_condition(cond);
  DDS::ReturnCode_t rc = ws->wait(active, four_sec);
  if (rc == DDS::RETCODE_OK) {
    // check if incompatible deadline was correctly flagged
    if ((active.length() == 0) || (active[0] != cond)) {
      e = "Failed to get requested incompatible qos status";
    }
  } else {
    e = "REQUESTED_INCOMPATIBLE_QOS_STATUS WaitSet::wait returned: ";
    e += OpenDDS::DCPS::retcode_to_string(rc);
  }
  ws->detach_condition(cond);
  if (!e.empty()) {
    throw std::runtime_error(e);
  }
}

// Compatible deadline QoS: offered deadline period <= requested deadline period.
void Subscriber::test_incompatible_deadline_qos()
{
  ACE_DEBUG((LM_INFO, ACE_TEXT("(%P|%t) Subscriber::test_incompatible_deadline_qos\n")));
  DDS::DataReaderQos incompatible_qos;
  sub_->get_default_datareader_qos(incompatible_qos);
  incompatible_qos.deadline.period.sec = 2; // < offered deadline.period
  incompatible_qos.deadline.period.nanosec = 0;
  DDS::DataReader_var tmp_dr = sub_->create_datareader(
    domain_.topic().in(), incompatible_qos, 0, OpenDDS::DCPS::DEFAULT_STATUS_MASK);
  if (!tmp_dr) {
    throw std::runtime_error("create_datareader with incompatible_qos failed.\n");
  }

  wait_incompatible_qos_status(tmp_dr->get_statuscondition());

  DDS::RequestedIncompatibleQosStatus stat;
  if (tmp_dr->get_requested_incompatible_qos_status(stat) != DDS::RETCODE_OK) {
    throw std::runtime_error("get_requested_incompatible_qos_status failed");
  }
  bool incompatible_deadline = false;
  const DDS::QosPolicyCountSeq& policies = stat.policies;
  const CORBA::ULong len = policies.length();
  for (CORBA::ULong i = 0; i < len; ++i) {
    if (policies[i].policy_id == DDS::DEADLINE_QOS_POLICY_ID) {
      incompatible_deadline = true;
      break;
    }
  }
  if (!incompatible_deadline) {
    throw std::runtime_error("Reader/Writer associated with incompatible deadline QoS.");
  }
}

bool Subscriber::check_status(const DDS::InstanceHandle_t& i1r1, const DDS::InstanceHandle_t& i1r2,
                              const DDS::InstanceHandle_t& i2r1, const DDS::InstanceHandle_t& i2r2,
                              const CORBA::Long expected, CORBA::Long& delta1)
{
  ACE_DEBUG((LM_INFO, ACE_TEXT("(%P|%t) Subscriber::check_status expected=%d\n"), expected));
  DDS::RequestedDeadlineMissedStatus s1;
  DDS::RequestedDeadlineMissedStatus s2;
  if (!get_status(s1, s2)) {
    return false;
  }
  if (!check_instance(1, s1.last_instance_handle, i1r1, i2r1) ||
      !check_instance(2, s2.last_instance_handle, i1r2, i2r2)) {
    return false;
  }
  // Writer writes each instance after 9 seconds. Reader deadline period is 5 seconds.
  // After Domain::r_sleep (11 seconds), the deadline missed should be 1 per instance.
  if (s1.total_count != expected || s2.total_count != expected) {
    ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: missed requested deadline (%d or %d) != (%d)\n"),
               s1.total_count, s2.total_count, expected));
    return false;
  }
  if (!check_change(s1, s2, delta1)) {
    return false;
  }
  return true;
}

bool Subscriber::get_status(DDS::RequestedDeadlineMissedStatus& s1, DDS::RequestedDeadlineMissedStatus& s2)
{
  if (dr1_->get_requested_deadline_missed_status(s1) != DDS::RETCODE_OK) {
    ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: dr1_->get_requested_deadline_missed_status failed\n")));
    return false;
  }
  if (dr2_->get_requested_deadline_missed_status(s2) != DDS::RETCODE_OK) {
    ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: dr2_->get_requested_deadline_missed_status failed\n")));
    return false;
  }
  return true;
}

bool Subscriber::check_instance(const int dr, const DDS::InstanceHandle_t& last, const DDS::InstanceHandle_t& i1, const DDS::InstanceHandle_t& i2) const
{
  ACE_DEBUG((LM_INFO, ACE_TEXT("(%P|%t) Subscriber::check_instance dr%d\n"), dr));
  if (last != i1 && last != i2) {
    ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: last_instance_handle (%d) != dr%d's (%d or %d)\n"), last, dr, i1, i2));
    return false;
  }
  return true;
}

bool Subscriber::check_change(DDS::RequestedDeadlineMissedStatus& s1, DDS::RequestedDeadlineMissedStatus& s2, CORBA::Long& delta1) const
{
  ACE_DEBUG((LM_INFO, ACE_TEXT("(%P|%t) Subscriber::check_change\n")));
  // total_count_change of dr1 (no listener) should match the total_count minus last time change
  // total_count_change of dr2 should match the total_count minus that from listener
  delta1 = s1.total_count - delta1; // also save for the next check
  CORBA::Long delta2 = s2.total_count - listener_i_->requested_deadline_total_count();
  if (s1.total_count_change != delta1 || s2.total_count_change != delta2) {
    ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: missed requested deadline change (%d, %d) != (%d, %d)\n"),
               s1.total_count_change, s2.total_count_change, delta1, delta2));
    return false;
  }
  return true;
}

int ACE_TMAIN(int argc, ACE_TCHAR *argv[])
{
  int ret = 1;
  try {
    Subscriber sub(argc, argv);
    ret = sub.run();
  } catch (const CORBA::Exception& e) {
    e._tao_print_exception("Subscriber ERROR: ");
  } catch (const std::exception& e) {
    ACE_ERROR((LM_ERROR, ACE_TEXT("Subscriber ERROR: %C\n"), e.what()));
  } catch (...) {
    ACE_ERROR((LM_ERROR, ACE_TEXT("Subscriber ERROR: ...\n")));
  }
  TheServiceParticipant->shutdown();
  return ret;
}
