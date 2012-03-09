#include "FileInfoListener.h"
#include "AbstractionLayer.h"

#include "FileInfoC.h"
#include "FileInfoTypeSupportC.h"



FileInfoListener::FileInfoListener(AbstractionLayer* monitor)
: change_monitor_(monitor)
{
}


FileInfoListener::~FileInfoListener()
{
}


void FileInfoListener::on_data_available (::DDS::DataReader_ptr reader)
{

  try {
    DistributedContent::FileDiffDataReader_var fileinfo_dr =
      DistributedContent::FileDiffDataReader::_narrow(reader);
    if (CORBA::is_nil (fileinfo_dr.in ())) {
      ACE_ERROR((LM_ERROR, "ERROR: FileInfoListener::on_data_available() _narrow failed.\n"));
      return;
    }

    DistributedContent::FileDiff diff;
    DDS::SampleInfo si ;
    DDS::ReturnCode_t status = fileinfo_dr->take_next_sample(diff, si) ;

    if (status == DDS::RETCODE_OK) {

      if (0 != change_monitor_)
      {
        change_monitor_->receive_diff(diff);
      }
      else
      {
        ACE_DEBUG((LM_DEBUG,
          "FileInfoListener::on_data_available() no change_monitor_ defined\n"));
      }

    } else if (status == DDS::RETCODE_NO_DATA) {
      ACE_ERROR((LM_ERROR,
        "ERROR: FileInfoListener::on_data_available() received DDS::RETCODE_NO_DATA!"));
    } else {
      ACE_ERROR((LM_ERROR,
        "ERROR: FileInfoListener::on_data_available() read Message: Error: %d\n",
        status));
    }
  } catch (CORBA::Exception&) {
    ACE_ERROR((LM_ERROR,
      "ERROR: FileInfoListener::on_data_available() Exception caught in read\n"));
  }


}


void FileInfoListener::on_requested_deadline_missed (
                                   ::DDS::DataReader_ptr,
                                   const ::DDS::RequestedDeadlineMissedStatus &)
{
}


void FileInfoListener::on_requested_incompatible_qos (
                                    ::DDS::DataReader_ptr,
                                    const ::DDS::RequestedIncompatibleQosStatus &)
{
}


void FileInfoListener::on_sample_rejected (
                         ::DDS::DataReader_ptr,
                         const ::DDS::SampleRejectedStatus &
                         )
{
}



void FileInfoListener::on_liveliness_changed (
                            ::DDS::DataReader_ptr,
                            const ::DDS::LivelinessChangedStatus &)
{
}


void FileInfoListener::on_subscription_matched (
                            ::DDS::DataReader_ptr,
                            const ::DDS::SubscriptionMatchedStatus &)
{
}


void FileInfoListener::on_sample_lost (
                     ::DDS::DataReader_ptr,
                     const ::DDS::SampleLostStatus &)
{
}
