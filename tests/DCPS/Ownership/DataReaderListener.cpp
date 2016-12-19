/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include <ace/Log_Msg.h>
#include <ace/OS_NS_stdlib.h>

#include <dds/DdsDcpsSubscriptionC.h>
#include <dds/DCPS/Service_Participant.h>

#include "DataReaderListener.h"
#include "MessengerTypeSupportC.h"
#include "MessengerTypeSupportImpl.h"

#include <iostream>

extern int testcase;
const int num_messages_per_writer = 20;

DataReaderListenerImpl::DataReaderListenerImpl(const char* reader_id)
  : num_reads_(0),
    reader_id_ (reader_id),
    verify_result_ (true),
    result_verify_complete_ (false)
{
  this->current_strength_[0] = 0;
  this->current_strength_[1] = 0;
}

DataReaderListenerImpl::~DataReaderListenerImpl()
{
}

void DataReaderListenerImpl::on_data_available(DDS::DataReader_ptr reader)
throw(CORBA::SystemException)
{

  try {
    Messenger::MessageDataReader_var message_dr =
      Messenger::MessageDataReader::_narrow(reader);

    if (CORBA::is_nil(message_dr.in())) {
      ACE_ERROR((LM_ERROR,
                 ACE_TEXT("%N:%l: on_data_available()")
                 ACE_TEXT(" ERROR: _narrow failed!\n")));
      ACE_OS::exit(-1);
    }

    DDS::ReturnCode_t status = DDS::RETCODE_OK;
    while (status == DDS::RETCODE_OK) {
      ++num_reads_;
      Messenger::Message message;
      DDS::SampleInfo si;

      status = message_dr->take_next_sample(message, si) ;

      if (status == DDS::RETCODE_OK) {
        if (si.valid_data) {
          ACE_DEBUG ((LM_DEBUG, ACE_TEXT("(%P|%t) %s->%s subject_id: %d ")
            ACE_TEXT("count: %d strength: %d\n"),
            message.from.in(), this->reader_id_, message.subject_id,
            message.count, message.strength));
          std::cout << message.from.in() << "->" << this->reader_id_
          << " subject_id: " << message.subject_id
          << " count: " << message.count
          << " strength: " << message.strength
          << std::endl;
          bool result = verify (message);
          this->verify_result_ &= result;

        } else if (si.instance_state == DDS::NOT_ALIVE_DISPOSED_INSTANCE_STATE) {
          ACE_DEBUG((LM_DEBUG, ACE_TEXT("%N:%l: INFO: instance is disposed\n")));

        } else if (si.instance_state == DDS::NOT_ALIVE_NO_WRITERS_INSTANCE_STATE) {
          ACE_DEBUG((LM_DEBUG, ACE_TEXT("%N:%l: INFO: instance is unregistered\n")));

        } else {
          ACE_ERROR((LM_ERROR,
                     ACE_TEXT("%N:%l: on_data_available()")
                     ACE_TEXT(" ERROR: unknown instance state: %d\n"),
                     si.instance_state));
        }

      } else if (status != DDS::RETCODE_NO_DATA) {
        ACE_ERROR((LM_ERROR,
                   ACE_TEXT("%N:%l: on_data_available()")
                   ACE_TEXT(" ERROR: unexpected status: %d\n"),
                   status));
      }
    }

  } catch (const CORBA::Exception& e) {
    e._tao_print_exception("Exception caught in on_data_available():");
    ACE_OS::exit(-1);
  }
}

void DataReaderListenerImpl::on_requested_deadline_missed(
  DDS::DataReader_ptr,
  const DDS::RequestedDeadlineMissedStatus & status)
throw(CORBA::SystemException)
{
  ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) %s: on_requested_deadline_missed(): ")
          ACE_TEXT(" handle %d total_count_change %d \n"),
          this->reader_id_, status.last_instance_handle, status.total_count_change));
}

void DataReaderListenerImpl::on_requested_incompatible_qos(
  DDS::DataReader_ptr,
  const DDS::RequestedIncompatibleQosStatus &)
throw(CORBA::SystemException)
{
  ACE_DEBUG((LM_DEBUG, ACE_TEXT("%N:%l: INFO: on_requested_incompatible_qos()\n")));
}

void DataReaderListenerImpl::on_liveliness_changed(
  DDS::DataReader_ptr,
  const DDS::LivelinessChangedStatus & status)
throw(CORBA::SystemException)
{
  ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) %s: on_liveliness_changed(): ")
          ACE_TEXT(" last pub handle %d alive_count %d change %d; not_alive_count %d change %d \n"),
          reader_id_, status.last_publication_handle,
          status.alive_count, status.alive_count_change,
          status.not_alive_count, status.not_alive_count_change));
}

void DataReaderListenerImpl::on_subscription_matched(
  DDS::DataReader_ptr,
  const DDS::SubscriptionMatchedStatus &)
throw(CORBA::SystemException)
{
  ACE_DEBUG((LM_DEBUG, ACE_TEXT("%N:%l: INFO: on_subscription_matched()\n")));
}

void DataReaderListenerImpl::on_sample_rejected(
  DDS::DataReader_ptr,
  const DDS::SampleRejectedStatus&)
throw(CORBA::SystemException)
{
  ACE_DEBUG((LM_DEBUG, ACE_TEXT("%N:%l: INFO: on_sample_rejected()\n")));
}

void DataReaderListenerImpl::on_sample_lost(
  DDS::DataReader_ptr,
  const DDS::SampleLostStatus&)
throw(CORBA::SystemException)
{
  ACE_DEBUG((LM_DEBUG, ACE_TEXT("%N:%l: INFO: on_sample_lost()\n")));
}

bool
DataReaderListenerImpl::verify (const Messenger::Message& msg)
{
  if (msg.subject_id != msg.count % 2) {
    ACE_DEBUG((LM_ERROR,
        "(%P|%t) ERROR: subject id %d not count mod 2",
        msg.subject_id));
    return false;
  }

  int previous_strength = this->current_strength_[msg.subject_id];
  // record the strength of writer that sample is from.
  this->current_strength_[msg.subject_id] = msg.strength;

  switch (testcase) {
  case strength:
  {
    // strength should be not be less then before
    if (msg.strength < previous_strength) {
    ACE_DEBUG((LM_ERROR,
        "(%P|%t) ERROR: strength %d less than prev %d\n",
        msg.strength, previous_strength));
      // record the strength of writer that sample is from.
      return false;
    }
  }
  break;
  case liveliness_change:
  case miss_deadline:
  {
    ACE_Time_Value now = ACE_OS::gettimeofday();
    if (msg.count == 5 && msg.strength != 12) {
      ACE_DEBUG((LM_ERROR,
          "(%P|%t) ERROR: count is %d while strength is %d\n",
          msg.count, msg.strength));
      return false;
    }
    else {
      start_missing_ = now;
    }

    if (msg.count == 6 && msg.strength != 12) {
      ACE_DEBUG((LM_ERROR,
          "(%P|%t) ERROR: count is %d while strength is %d\n",
          msg.count, msg.strength));
      return false;
    }
    else {
      end_missing_ = now;
    }

    if (now < end_missing_ && now > start_missing_ && msg.strength != 10) {
      ACE_DEBUG((LM_ERROR,
          "(%P|%t) ERROR: now < end_missing, > start_missing with stength %d\n",
          msg.strength));
      return false;
    }
  }
  break;
  case update_strength:
  {
    ACE_DEBUG((LM_INFO,
         "(%P|%t) prev strength %d\n",
          previous_strength));

    // strength should not be less then before
    if ((! result_verify_complete_) && (msg.strength < previous_strength)) {
      ACE_DEBUG((LM_ERROR,
          "(%P|%t) ERROR: !result_verify_complete_ and strength %d less than prev %d\n",
          msg.strength, previous_strength));
      return false;
    }

    if (! result_verify_complete_ && num_messages_per_writer == msg.count) {
      // The owner writer is done. so the other writer will become
      // owner, then it will not meet the condition of the strength
      // always increase.
      this->result_verify_complete_ = true;

      if (msg.strength != 15) {
        ACE_DEBUG((LM_ERROR,
          "(%P|%t) ERROR: owner complete while strength is %d\n",
          msg.strength));
        return false;
      }
    }

  }
  break;
  default:
  ACE_OS::exit(1);
  break;
  }

  return true;
}


bool
DataReaderListenerImpl::verify_result ()
{
  switch (testcase) {
  case strength:
  {
    this->verify_result_ &=
        (this->current_strength_[0] == this->current_strength_[1]
        && this->current_strength_[0] == 12);
  }
  break;
  case liveliness_change:
  case miss_deadline:
  {
    // The liveliness is changed for both writers in the middle of sending
    // total messages but finally, the higher strength writer takes ownership.
    this->verify_result_ &=
        (this->current_strength_[0] == this->current_strength_[1]
        && this->current_strength_[0] == 12);
  }
  break;
  case update_strength:
  {
  }
  break;
  default:
  ACE_OS::exit(1);
  break;
  }
  return this->verify_result_;
}
