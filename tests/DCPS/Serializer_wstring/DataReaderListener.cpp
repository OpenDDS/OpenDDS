// -*- C++ -*-
//
#include "DataReaderListener.h"
#include "MessengerTypeSupportC.h"
#include "MessengerTypeSupportImpl.h"
#include <dds/DCPS/Service_Participant.h>
#include <ace/streams.h>
#include "tests/Utils/ExceptionStreams.h"

using namespace Messenger;
using namespace std;

const CORBA::Char* charseq = "I'm char seq";
const CORBA::WChar* wcharseq = (const CORBA::WChar*)(L"I'm wchar seq");
const CORBA::Char* strseq = "I'm string seq";
const CORBA::WChar* wstrseq = (const CORBA::WChar*)(L"I'm wstring seq");
const CORBA::Char* str = "I'm string";
const CORBA::WChar* wstr = (const CORBA::WChar*)(L"I'm wstring");

const int num_expected_messages = 10;

DataReaderListenerImpl::DataReaderListenerImpl()
  : num_reads_(0),
    passed_count_ (0)
{
}

DataReaderListenerImpl::~DataReaderListenerImpl ()
{
}

void DataReaderListenerImpl::on_data_available(DDS::DataReader_ptr reader)
{
  ++num_reads_;

  try {
    MessageDataReader_var message_dr = MessageDataReader::_narrow(reader);
    if (CORBA::is_nil (message_dr.in ())) {
      cerr << "read: _narrow failed." << endl;
      exit(1);
    }

    Messenger::Message message;
    DDS::SampleInfo si ;
    DDS::ReturnCode_t status = message_dr->take_next_sample(message, si) ;


    if (status == DDS::RETCODE_OK) {
      if (si.valid_data)
      {
        cout << "Message: subject_id = " << message.subject_id   << endl
            << "         count      = " << message.count        << endl;
        cout << "SampleInfo.sample_rank = " << si.sample_rank << endl;
        if (verify_message (message))
          ++passed_count_;
      }
      else if (si.instance_state == DDS::NOT_ALIVE_DISPOSED_INSTANCE_STATE)
      {
        ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) instance is disposed\n")));
      }
      else if (si.instance_state == DDS::NOT_ALIVE_NO_WRITERS_INSTANCE_STATE)
      {
        ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) instance is unregistered\n")));
      }
    } else if (status == DDS::RETCODE_NO_DATA) {
      cerr << "ERROR: reader received DDS::RETCODE_NO_DATA!" << endl;
    } else {
      cerr << "ERROR: read Message: Error: " <<  status << endl;
    }
  } catch (CORBA::Exception& e) {
    cerr << "Exception caught in read:" << endl << e << endl;
    exit(1);
  }
}


bool DataReaderListenerImpl::verify_message (Messenger::Message& message)
{
  switch (message.count)
  {
    case 9:
      {
        if (ACE_OS::strcmp (message.bounded_wchar_seq.get_buffer (), wcharseq))
        {
          ACE_ERROR_RETURN ((LM_ERROR,
                      "(%P|%t) ERROR: wchar_seq(%d) - unexpected bounded_wchar_seq %W\n", message.count,
                      message.bounded_wchar_seq.get_buffer ()),
                      false);
        }
      }
    case 8:
      {
        if (ACE_OS::strcmp (message.bounded_char_seq.get_buffer (), charseq))
        {
          ACE_ERROR_RETURN ((LM_ERROR,
                      "(%P|%t) ERROR: wchar_seq(%d) - unexpected bounded_char_seq %s\n", message.count,
                      message.bounded_char_seq.get_buffer ()),
                      false);
        }
      }
    case 7:
      {
        if (message.wstring_seq.length () != 5)
        {
          ACE_ERROR_RETURN ((LM_ERROR,
                              "(%P|%t) ERROR: wstring_seq(%d) - unexpected len %d \n", message.count,
                              message.wstring_seq.length ()),
                              false);
        }
        for (int i = 0; i < 4; ++i)
        {
          if (! message.wstring_seq[i].in ())
            ACE_ERROR_RETURN ((LM_ERROR,
                              "(%P|%t) ERROR: wstring_seq(%d) - unexpected seq[%d] \n", message.count,
                              i),
                              false);
        }

        if (ACE_OS::strcmp (message.wstring_seq[4], wstrseq))
        {
          ACE_ERROR_RETURN ((LM_ERROR,
                      "(%P|%t) ERROR: wstring_seq(%d) - unexpected wstring_seq[4]=%W \n", message.count,
                      message.wstring_seq[4].in ()),
                      false);
        }
      }
    case 6:
      {
        if (message.string_seq.length () != 5)
          ACE_ERROR_RETURN ((LM_ERROR,
                            "(%P|%t) ERROR: string_seq(%d) - unexpected len %d \n", message.count,
                            message.string_seq.length ()),
                            false);
        for (int i = 0; i < 4; ++i)
        {
          if (! message.string_seq[i].in ())
            ACE_ERROR_RETURN ((LM_ERROR,
                              "(%P|%t) ERROR: string_seq(%d) - unexpected seq[%d] \n", message.count,
                              i),
                              false);
        }
        if (ACE_OS::strcmp (message.string_seq[4], strseq))
        {
          ACE_ERROR_RETURN ((LM_ERROR,
                      "(%P|%t) ERROR: string_seq(%d) - unexpected string_seq[4]=%s \n", message.count,
                      message.string_seq[4].in ()),
                      false);
        }
      }
    case 5:
      {
        if (ACE_OS::strcmp (message.wchar_seq.get_buffer (), wcharseq))
        {
          ACE_ERROR_RETURN ((LM_ERROR,
                      "(%P|%t) ERROR: wchar_seq(%d) - unexpected wchar_seq %W\n", message.count,
                      message.wchar_seq.get_buffer ()),
                      false);
        }
      }
    case 4:
      {
        if (ACE_OS::strcmp (message.char_seq.get_buffer (), charseq))
          ACE_ERROR_RETURN ((LM_ERROR,
                      "(%P|%t) ERROR: char_seq(%d) - unexpected char_seq %C\n", message.count,
                      message.char_seq.get_buffer ()),
                      false);
      }
    case 3:
      {
        if (!message.wstr.in()) {
          ACE_ERROR_RETURN ((LM_ERROR,
                      "(%P|%t) ERROR: wstring(%d) - expected non-empty wstring.\n", message.count),
                      false);
        }
        if (ACE_OS::strcmp (message.wstr.in (), wstr))
          ACE_ERROR_RETURN ((LM_ERROR,
                      "(%P|%t) ERROR: wstring(%d) - unexpected wstring %W\n", message.count, message.wstr.in ()),
                      false);
      }
    case 2:
      {
        if (ACE_OS::strcmp (message.str.in (), str))
          ACE_ERROR_RETURN ((LM_ERROR,
                      "(%P|%t) ERROR: string(%d) - unexpected string %C\n", message.count, message.str.in ()),
                      false);
      }
    case 1:
      {
        if (message.wch != L'B')
          ACE_ERROR_RETURN ((LM_ERROR,
                      "(%P|%t) ERROR: wchar(%d) - unexpected wchar \n", message.count),
                      false);
      }
    case 0:
      {
        if (message.ch != 'A')
          ACE_ERROR_RETURN ((LM_ERROR,
                      "(%P|%t) ERROR: char(%d) - unexpected char \n", message.count),
                      false);
      }
      break;
    default:
      ACE_ERROR_RETURN ((LM_ERROR,
                  "(%P|%t) ERROR: unexpected message(%d)\n", message.count),
                  false);
      break;
  }

  return true;
}


void DataReaderListenerImpl::on_requested_deadline_missed (
    DDS::DataReader_ptr,
    const DDS::RequestedDeadlineMissedStatus &)
{
  cerr << "DataReaderListenerImpl::on_requested_deadline_missed" << endl;
}

void DataReaderListenerImpl::on_requested_incompatible_qos (
    DDS::DataReader_ptr,
    const DDS::RequestedIncompatibleQosStatus &)
{
  cerr << "DataReaderListenerImpl::on_requested_incompatible_qos" << endl;
}

void DataReaderListenerImpl::on_liveliness_changed (
    DDS::DataReader_ptr,
    const DDS::LivelinessChangedStatus &)
{
  cerr << "DataReaderListenerImpl::on_liveliness_changed" << endl;
}

void DataReaderListenerImpl::on_subscription_matched (
    DDS::DataReader_ptr,
    const DDS::SubscriptionMatchedStatus &)
{
  cerr << "DataReaderListenerImpl::on_subscription_matched" << endl;
}

void DataReaderListenerImpl::on_sample_rejected(
    DDS::DataReader_ptr,
    const DDS::SampleRejectedStatus&)
{
  cerr << "DataReaderListenerImpl::on_sample_rejected" << endl;
}

void DataReaderListenerImpl::on_sample_lost(
  DDS::DataReader_ptr,
  const DDS::SampleLostStatus&)
{
  cerr << "DataReaderListenerImpl::on_sample_lost" << endl;
}

void DataReaderListenerImpl::on_subscription_disconnected (
  DDS::DataReader_ptr,
  const ::OpenDDS::DCPS::SubscriptionDisconnectedStatus &)
{
  cerr << "DataReaderListenerImpl::on_subscription_disconnected" << endl;
}

void DataReaderListenerImpl::on_subscription_reconnected (
  DDS::DataReader_ptr,
  const ::OpenDDS::DCPS::SubscriptionReconnectedStatus &)
{
  cerr << "DataReaderListenerImpl::on_subscription_reconnected" << endl;
}

void DataReaderListenerImpl::on_subscription_lost (
  DDS::DataReader_ptr,
  const ::OpenDDS::DCPS::SubscriptionLostStatus &)
{
  cerr << "DataReaderListenerImpl::on_subscription_lost" << endl;
}

void DataReaderListenerImpl::on_budget_exceeded (
  DDS::DataReader_ptr,
  const ::OpenDDS::DCPS::BudgetExceededStatus&)
{
  cerr << "DataReaderListenerImpl::on_budget_exceeded" << endl;
}

void DataReaderListenerImpl::on_connection_deleted (
  ::DDS::DataReader_ptr)
{
  cerr << "DataReaderListenerImpl::on_connection_deleted" << endl;
}


bool DataReaderListenerImpl::received_all ()
{
  return this->num_reads_ == num_expected_messages;
}


bool DataReaderListenerImpl::passed ()
{
  return this->num_reads_ == this->passed_count_;
}
