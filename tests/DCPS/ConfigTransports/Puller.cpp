#include "Puller.h"
#include "Factory.h"
#include "../common/TestSupport.h"

#include "ace/Log_Msg.h"

#include "dds/DCPS/WaitSet.h"

Puller::Puller(const Factory& f,
               const DDS::DomainParticipantFactory_var& factory,
               const DDS::DomainParticipant_var& participant,
               const DDS::DataReaderListener_var& listener) :
        dpf(factory),
        dp(participant),
        topic(f.topic(dp)),
        sub(f.subscriber(dp)),
        reader_(f.reader(sub, topic, listener)) { }

Puller::Puller(const Factory& f,
               const DDS::DomainParticipantFactory_var& factory,
               const DDS::DomainParticipant_var& participant,
               const DDS::Subscriber_var& subscriber,
               const DDS::DataReaderListener_var& listener) :
        dpf(factory),
        dp(participant),
        topic(f.topic(dp)),
        sub(subscriber),
        reader_(f.reader(sub, topic, listener)) { }

Puller::~Puller()
{
  // don't do any cleanup, leave that to be done when everything else
  // is out of scope
}

int
Puller::pull(const ACE_Time_Value& /*duration*/)
{
  // Block until Publisher completes
  DDS::StatusCondition_var condition = reader_->get_statuscondition();
  condition->set_enabled_statuses(
                                  DDS::SUBSCRIPTION_MATCHED_STATUS
                                  | DDS::PUBLICATION_MATCHED_STATUS
                                  );
  //| DDS::DATA_AVAILABLE_STATUS);

  DDS::WaitSet_var ws = new DDS::WaitSet;
  ws->attach_condition(condition);

  //  DDS::Duration_t timeout = {duration.sec(), 0};
  DDS::Duration_t timeout = {DDS::DURATION_INFINITE_SEC, DDS::DURATION_INFINITE_NSEC};

  DDS::ConditionSeq conditions;
  DDS::SubscriptionMatchedStatus matches = {0, 0, 0, 0, 0};

  do
    {
      if (ws->wait(conditions, timeout) != DDS::RETCODE_OK)
        {
          ACE_ERROR_RETURN((LM_ERROR,
                            ACE_TEXT("%N:%l pull()")
                            ACE_TEXT(" ERROR: wait() failed: %p\n")), -1);
        }

      if (reader_->get_subscription_matched_status(matches) != DDS::RETCODE_OK)
        {
          ACE_ERROR_RETURN((LM_ERROR,
                            ACE_TEXT("%N:%l pull()")
                            ACE_TEXT(" ERROR: get_subscription_matched_status() failed: %p\n")), -1);
        }
    }
  while (matches.current_count > 0);

  ws->detach_condition(condition);

  return 0;
}

