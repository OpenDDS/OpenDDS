#include "WriterListener.h"

WriterListener::WriterListener(AssociationTable& association_table) :
  association_table_(association_table)
{}

void WriterListener::on_requested_deadline_missed(::DDS::DataReader_ptr /*reader*/,
                                                  const ::DDS::RequestedDeadlineMissedStatus & /*status*/)
{}

void WriterListener::on_requested_incompatible_qos(::DDS::DataReader_ptr /*reader*/,
                                                   const ::DDS::RequestedIncompatibleQosStatus & /*status*/)
{}

void WriterListener::on_sample_rejected(::DDS::DataReader_ptr /*reader*/,
                                        const ::DDS::SampleRejectedStatus & /*status*/)
{}

void WriterListener::on_liveliness_changed(::DDS::DataReader_ptr /*reader*/,
                                           const ::DDS::LivelinessChangedStatus & /*status*/)
{}

void WriterListener::on_data_available(::DDS::DataReader_ptr reader)
{
  RtpsRelay::WriterEntryDataReader_var dr = RtpsRelay::WriterEntryDataReader::_narrow(reader);
  if (!dr) {
    ACE_ERROR((LM_ERROR, "(%P|%t) %N:%l ERROR: WriterListener::on_data_available failed to narrow RtpsRelay::WriterEntryDataReader\n"));
    return;
  }

  RtpsRelay::WriterEntrySeq data;
  DDS::SampleInfoSeq infos;
  DDS::ReturnCode_t ret = dr->read(data,
                                   infos,
                                   DDS::LENGTH_UNLIMITED,
                                   DDS::NOT_READ_SAMPLE_STATE,
                                   DDS::ANY_VIEW_STATE,
                                   DDS::ANY_INSTANCE_STATE);
  if (ret != DDS::RETCODE_OK) {
    ACE_ERROR((LM_ERROR, "(%P|%t) %N:%l ERROR: WriterListener::on_data_available failed to read\n"));
    return;
  }

  for (size_t idx = 0; idx != infos.length(); ++idx) {
    switch (infos[idx].instance_state) {
    case DDS::ALIVE_INSTANCE_STATE:
      association_table_.insert(data[idx]);
      break;
    case DDS::NOT_ALIVE_DISPOSED_INSTANCE_STATE:
    case DDS::NOT_ALIVE_NO_WRITERS_INSTANCE_STATE:
      association_table_.remove(data[idx]);
      break;
    }
  }
}

void WriterListener::on_subscription_matched(::DDS::DataReader_ptr /*reader*/,
                                             const ::DDS::SubscriptionMatchedStatus & /*status*/)
{}

void WriterListener::on_sample_lost(::DDS::DataReader_ptr /*reader*/,
                                    const ::DDS::SampleLostStatus & /*status*/)
{}
