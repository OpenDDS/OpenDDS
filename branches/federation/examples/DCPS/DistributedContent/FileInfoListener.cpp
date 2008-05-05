#include "FileInfoListener.h"
#include "AbstractionLayer.h"

#include "FileInfoC.h"
#include "FileDiffTypeSupportC.h"



FileInfoListener::FileInfoListener(AbstractionLayer* monitor)
: change_monitor_(monitor)
{
}


FileInfoListener::~FileInfoListener()
{
}


void FileInfoListener::on_data_available (::DDS::DataReader_ptr reader)
    ACE_THROW_SPEC ((::CORBA::SystemException))
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
    // Alternate code to read directlty via the servant
    //FileInfoDataReaderImpl* dr_servant =
    //  reference_to_servant< FileInfoDataReaderImpl> (fileinfo_dr.in ());
    //DDS::ReturnCode_t status = dr_servant->take_next_sample(diff, si) ;

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
    ACE_THROW_SPEC ((::CORBA::SystemException))
{
}


void FileInfoListener::on_requested_incompatible_qos (
                                    ::DDS::DataReader_ptr,
                                    const ::DDS::RequestedIncompatibleQosStatus &)
    ACE_THROW_SPEC ((::CORBA::SystemException))
{
}


void FileInfoListener::on_sample_rejected (
                         ::DDS::DataReader_ptr,
                         const ::DDS::SampleRejectedStatus &
                         )
    ACE_THROW_SPEC ((::CORBA::SystemException))
{
}



void FileInfoListener::on_liveliness_changed (
                            ::DDS::DataReader_ptr,
                            const ::DDS::LivelinessChangedStatus &)
    ACE_THROW_SPEC ((::CORBA::SystemException))
{
}


void FileInfoListener::on_subscription_match (
                            ::DDS::DataReader_ptr,
                            const ::DDS::SubscriptionMatchStatus &)
    ACE_THROW_SPEC ((::CORBA::SystemException))
{
}


void FileInfoListener::on_sample_lost (
                     ::DDS::DataReader_ptr,
                     const ::DDS::SampleLostStatus &)
    ACE_THROW_SPEC ((::CORBA::SystemException))
{
}
