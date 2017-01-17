// -*- C++ -*-
//

#include "AlertDataReaderListenerImpl.h"
#include "SatelliteTypeSupportC.h"
#include <ace/streams.h>
#include <string>

AlertDataReaderListenerImpl::AlertDataReaderListenerImpl() :
  liveliness_changed_count_(0),
  error_occurred_(false)
{
  // expect the alive_count either 0 or 1
  expected_status_.alive_count = 1;
  expected_status_.alive_count_change = 1;
  expected_status_.not_alive_count = 0;
  expected_status_.not_alive_count_change = 0;
  expected_status_.last_publication_handle = DDS::HANDLE_NIL;

  last_status_.alive_count = 0;
  last_status_.alive_count_change = 0;
  last_status_.not_alive_count = 0;
  last_status_.not_alive_count_change = 0;
  last_status_.last_publication_handle = DDS::HANDLE_NIL;
}

AlertDataReaderListenerImpl::~AlertDataReaderListenerImpl ()
{
}

void AlertDataReaderListenerImpl::on_data_available(DDS::DataReader_ptr reader)
{
    Satellite::AlertDataReader_var alert_dr = Satellite::AlertDataReader::_narrow(reader);

    if (CORBA::is_nil(alert_dr.in()))
    {
      ACE_ERROR((LM_ERROR, ACE_TEXT("(%P | %t) AlertDataReaderListenerImpl::on_data_available: _narrow failed.\n")));
      error_occurred_ = true;
      ACE_OS::exit(1);
    }

    // Take an Alert
    Satellite::Alert alert;
    DDS::SampleInfo si ;
    DDS::ReturnCode_t status = alert_dr->take_next_sample(alert, si);

    if (status == DDS::RETCODE_OK)
    {
      if (si.valid_data) {
        ACE_DEBUG((LM_DEBUG, "\n======================\n"
          "(%P|%t) AlertDataReaderListenerImpl::on_data_available:\n"
          "\t%C - Received Alert (%d) %C : %C - %C\n"
          "======================\n\n",
          alert.satellite.in(),
          alert.index,
          alert.item.in(),
          alert.code.in(),
          alert.message.in()));

        if (alert.index == 9999 /*Satellite::SYSTEM_SHUTDOWN */)
        {
          ACE_DEBUG((LM_DEBUG, "(%P|%t) AlertDataReaderListenerImpl::on_data_available:\n"
            "Received SYSTEM_SHUTDOWN message, udpating expected liveliness values\n"));
          if (last_status_.alive_count != 1)
          {
            ACE_ERROR((LM_ERROR,
              "ERROR: AlertDataReaderListenerImpl::on_data_available"
              " Received SYSTEM_SHUTDOWN message and expected/got last alive_count %d/%d \n",
              expected_status_.alive_count, last_status_.alive_count));
            error_occurred_ = true;
          }
          expected_status_.alive_count = 0;
        }
      }
    }
    else if (status == ::DDS::RETCODE_NO_DATA)
    {
      ACE_ERROR((LM_ERROR, "(%P|%t) AlertDataReaderListenerImpl::on_data_available: "
        "take returned ::DDS::RETCODE_NO_DATA\n"));
      error_occurred_ = true;
    }
    else
    {
      ACE_ERROR((LM_ERROR, "(%P|%t) AlertDataReaderListenerImpl::on_data_available: "
        "take - Error: %d\n", status));
      error_occurred_ = true;
    }
}

void AlertDataReaderListenerImpl::on_requested_deadline_missed (
    DDS::DataReader_ptr,
    const DDS::RequestedDeadlineMissedStatus &)
{
  ACE_DEBUG((LM_DEBUG,
    ACE_TEXT("(%P|%t) AlertDataReaderListenerImpl::on_requested_deadline_missed \n")));
}

void AlertDataReaderListenerImpl::on_requested_incompatible_qos (
    DDS::DataReader_ptr,
    const DDS::RequestedIncompatibleQosStatus &)
{
  ACE_DEBUG((LM_DEBUG,
    ACE_TEXT("(%P|%t) AlertDataReaderListenerImpl::on_requested_incompatible_qos \n")));
}

void AlertDataReaderListenerImpl::on_liveliness_changed (
    DDS::DataReader_ptr,
    const DDS::LivelinessChangedStatus& status)
{
  liveliness_changed_count_++;

  ACE_DEBUG((LM_DEBUG,
    "\n==================================================\n"
    "(%P|%t) AlertDataReaderListenerImpl::on_liveliness_changed\n"
    "    Liveliness Changes     = %d\n"
    "    Alive Count            = %d\n"
    "    Alive Count Change     = %d\n"
    "    Not Alive Count        = %d\n"
    "    Not Alive Count Change = %d\n"
    "==================================================\n\n",
    liveliness_changed_count_, status.alive_count, status.alive_count_change,
    status.not_alive_count, status.not_alive_count_change));

  if (status.alive_count != expected_status_.alive_count)
  {
    ACE_ERROR((LM_ERROR,
      "ERROR: AlertDataReaderListenerImpl::on_liveliness_changed"
      " expected/got alive_count %d/%d\n",
      expected_status_.alive_count, status.alive_count));
    error_occurred_ = true;
  }

  last_status_ = status;
}

void AlertDataReaderListenerImpl::on_subscription_matched (
    DDS::DataReader_ptr,
    const DDS::SubscriptionMatchedStatus &)
{
  ACE_DEBUG((LM_DEBUG,
    ACE_TEXT("(%P|%t) AlertDataReaderListenerImpl::on_subscription_matched \n")));
}

void AlertDataReaderListenerImpl::on_sample_rejected(
    DDS::DataReader_ptr,
    const DDS::SampleRejectedStatus&)
{
  ACE_DEBUG((LM_DEBUG,
    ACE_TEXT("(%P|%t) AlertDataReaderListenerImpl::on_sample_rejected \n")));
}

void AlertDataReaderListenerImpl::on_sample_lost(
  DDS::DataReader_ptr,
  const DDS::SampleLostStatus&)
{
  ACE_DEBUG((LM_DEBUG,
    ACE_TEXT("(%P|%t) AlertDataReaderListenerImpl::on_sample_lost \n")));
}
