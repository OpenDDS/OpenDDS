#include "Subscriber.h"
#include "FooTypeTypeSupportImpl.h"

#include <dds/DdsDcpsInfrastructureC.h>
#include <dds/DCPS/Marked_Default_Qos.h>
#include <dds/DCPS/SubscriberImpl.h>
#include <dds/DCPS/StaticIncludes.h>
#include <dds/DCPS/WaitSet.h>
#ifdef ACE_AS_STATIC_LIBS
# include <dds/DCPS/RTPS/RtpsDiscovery.h>
# include <dds/DCPS/transport/rtps_udp/RtpsUdp.h>
#endif

#include <ace/Log_Msg.h>

Subscriber::Subscriber(const DDS::DomainId_t domainId, const std::size_t n_pub_threads, const std::size_t expected_samples, const bool durable)
  : domainId_(domainId)
  , n_pub_threads_(n_pub_threads)
  , expected_samples_(expected_samples)
  , durable_(durable)
{
  ACE_DEBUG((LM_INFO, ACE_TEXT("(%P|%t) -> Subscriber::Subscriber\n")));
  try {
    dpf_ = TheParticipantFactory;
    if (!dpf_) {
      throw std::runtime_error("TheParticipantFactory failed.");
    }
    dp_ = dpf_->create_participant(domainId_, PARTICIPANT_QOS_DEFAULT, 0, OpenDDS::DCPS::DEFAULT_STATUS_MASK);
    if (!dp_) {
      throw std::runtime_error("create_participant failed.");
    }
    FooTypeSupport_var ts = new FooTypeSupportImpl;
    if (ts->register_type(dp_.in(), "") != DDS::RETCODE_OK) {
      throw std::runtime_error("register_type failed.");
    }
    DDS::Topic_var topic = dp_->create_topic("FooTopic",
      CORBA::String_var(ts->get_type_name()), TOPIC_QOS_DEFAULT, 0, OpenDDS::DCPS::DEFAULT_STATUS_MASK);
    if (!topic) {
      throw std::runtime_error("create_topic failed.");
    }
    // Create Subscriber
    DDS::Subscriber_var sub = dp_->create_subscriber(SUBSCRIBER_QOS_DEFAULT, 0, OpenDDS::DCPS::DEFAULT_STATUS_MASK);
    if (!sub) {
      throw std::runtime_error("create_subscriber failed.");
    }
    // Create DataReader
    listener_i_ = new DataReaderListenerImpl(expected_samples_, "(%P|%t)  sub %d%% (%d samples received)\n");
    listener_ = listener_i_;
    DDS::DataReaderQos qos;
    sub->get_default_datareader_qos(qos);
    qos.reliability.kind = DDS::RELIABLE_RELIABILITY_QOS;
    if (durable_) {
      qos.durability.kind = DDS::TRANSIENT_LOCAL_DURABILITY_QOS;
    }
#ifndef OPENDDS_NO_OWNERSHIP_PROFILE
    qos.history.kind = DDS::KEEP_ALL_HISTORY_QOS;
#endif
    reader_ = sub->create_datareader(topic.in(), qos, listener_.in(), OpenDDS::DCPS::DEFAULT_STATUS_MASK);
    if (!reader_) {
      throw std::runtime_error("create_datareader failed.");
    }
    OpenDDS::DCPS::DataReaderImpl* impl = dynamic_cast<OpenDDS::DCPS::DataReaderImpl*>(reader_.in());
    ACE_DEBUG((LM_INFO, "(%P|%t)    Subscriber reader id: %C\n", OpenDDS::DCPS::LogGuid(impl->get_repo_id()).c_str()));
  } catch (const std::exception& e) {
    ACE_ERROR((LM_ERROR, "(%P|%t) %C\n", e.what()));
    cleanup();
    throw;
  } catch (...) {
    ACE_ERROR((LM_ERROR, "(%P|%t) exception\n"));
    cleanup();
    throw;
  }
}
Subscriber::~Subscriber()
{
  cleanup();
  ACE_DEBUG((LM_INFO, ACE_TEXT("(%P|%t) <- Subscriber::~Subscriber\n")));
}

void Subscriber::cleanup()
{
  if (!dp_) {
    ACE_DEBUG((LM_INFO, ACE_TEXT("(%P|%t)    <- Subscriber delete_contained_entities\n")));
    dp_->delete_contained_entities();
    if (!dpf_) {
      ACE_DEBUG((LM_INFO, ACE_TEXT("(%P|%t)    <- Subscriber delete_participant\n")));
      dpf_->delete_participant(dp_.in());
      dpf_ = 0;
    }
    dp_ = 0;
  }
}

int Subscriber::wait(unsigned int num_writers, const int cmp)
{
  ACE_DEBUG((LM_INFO, ACE_TEXT("(%P|%t) -> Subscriber::wait\n")));
  int ret = 1;
  const CORBA::Long n_writers = static_cast<CORBA::Long>(num_writers);
  DDS::StatusCondition_var condition = reader_->get_statuscondition();
  condition->set_enabled_statuses(DDS::SUBSCRIPTION_MATCHED_STATUS);
  DDS::WaitSet_var ws(new DDS::WaitSet);
  ws->attach_condition(condition);
  DDS::SubscriptionMatchedStatus ms = {0, 0, 0, 0, 0};
  const DDS::Duration_t wake_interval = {3, 0};
  DDS::ConditionSeq conditions;
  while (ret == 1) {
    if (reader_->get_subscription_matched_status(ms) == DDS::RETCODE_OK) {
      if ((cmp ==  0 && ms.current_count == n_writers) ||
          (cmp ==  2 && ms.current_count >= n_writers) ||
          (cmp ==  1 && ms.current_count >  n_writers) ||
          (cmp == -1 && ms.current_count <  n_writers) ||
          (cmp == -2 && ms.current_count <= n_writers)) {
        ret = 0;
      } else { // wait for a change
        ACE_DEBUG((LM_INFO, ACE_TEXT("(%P|%t) -> ws->wait %d\n"), ms.current_count));
        DDS::ReturnCode_t stat = ws->wait(conditions, wake_interval);
        if ((stat != DDS::RETCODE_OK) && (stat != DDS::RETCODE_TIMEOUT)) {
          ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: %N:%l: wait failed!\n")));
          ret = -1;
        }
      }
    } else {
      ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: %N:%l: get_subscription_matched_status failed!\n")));
      ret = -1;
    }
  }
  ws->detach_condition(condition);
  ACE_DEBUG((LM_INFO, ACE_TEXT("(%P|%t) <- Subscriber::wait returns %d\n"), ret));
  return ret;
}

void Subscriber::wait_received()
{
  listener_i_->wait_received();
}

int Subscriber::check_result() const
{
  return listener_i_->check_received(n_pub_threads_);
}
