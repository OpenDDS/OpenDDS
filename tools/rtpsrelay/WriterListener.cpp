#include "WriterListener.h"

namespace RtpsRelay {

WriterListener::WriterListener(AssociationTable& association_table,
                               SpdpHandler& spdp_handler)
  : association_table_(association_table)
  , spdp_handler_(spdp_handler)
{}

void WriterListener::on_data_available(DDS::DataReader_ptr reader)
{
  WriterEntryDataReader_var dr = WriterEntryDataReader::_narrow(reader);
  if (!dr) {
    ACE_ERROR((LM_ERROR, "(%P|%t) %N:%l ERROR: WriterListener::on_data_available failed to narrow RtpsRelay::WriterEntryDataReader\n"));
    return;
  }

  WriterEntrySeq data;
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
      {
        RelayAddressesMap relay_addresses_map;
        association_table_.insert(data[idx], relay_addresses_map);
        spdp_handler_.replay(guid_to_guid(data[idx].guid()), relay_addresses_map);
      }
      break;
    case DDS::NOT_ALIVE_DISPOSED_INSTANCE_STATE:
    case DDS::NOT_ALIVE_NO_WRITERS_INSTANCE_STATE:
      association_table_.remove(data[idx]);
      break;
    }
  }
}

}
