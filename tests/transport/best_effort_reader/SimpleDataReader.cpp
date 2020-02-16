#include "SimpleDataReader.h"

#include "dds/DCPS/transport/framework/TransportExceptions.h"
#include "dds/DCPS/transport/framework/ReceivedDataSample.h"
#include "dds/DCPS/AssociationData.h"
#include "dds/DCPS/GuidConverter.h"
#include "dds/DCPS/Qos_Helper.h"

#include <ace/String_Base.h>
#include <ace/Get_Opt.h>
#include <ace/OS_NS_time.h>
#include "ace/OS_NS_unistd.h"

#include <cstdio>
#include <cstring>
#include <ctime>
#include <iostream>
#include <sstream>

SimpleDataReader::SimpleDataReader(const AppConfig& ac, const int readerIndex, AssociationData& ad)
  : config(ac), index(readerIndex), done_(false)
{
  enable_transport((index == 2), false); //(reliable, durable)

  for (int i = index; i < 3; ++i) {
    ad.remote_id_ = AppConfig::writerId[i];
    if (!associate(ad, false)) {
      throw std::string("reader associate() failed");
    }
    id_seqN_[AppConfig::writerId[i]] = 0;
    config.to_cerr(ad.remote_id_, get_repo_id(), "associated");
  }
  std::cerr << "Reader" << (index+1) << " associated with " << id_seqN_.size() << " writer(s)\n" << std::endl;
}

SimpleDataReader::~SimpleDataReader() {
  for (int i = index; i < 3; ++i) {
    disassociate(AppConfig::writerId[i]);
    config.to_cerr(AppConfig::writerId[i], get_repo_id(), "disassociated");
  }
}

// ========== ========== ========== ========== ========== ========== ==========
// Implementing TransportReceiveListener
void SimpleDataReader::data_received(const ReceivedDataSample& sample)
{
  if (sample.header_.message_id_ != SAMPLE_DATA) {
    return;
  }

  Serializer ser(sample.sample_.get(),
                 sample.header_.byte_order_ != ACE_CDR_BYTE_ORDER,
                 Serializer::ALIGN_CDR);
  TestMsg data;
  if (!deserializeData(data, ser)) {
    return;
  }

  if (data.key == 99) {
    ACE_DEBUG((LM_INFO, ACE_TEXT("%C received terminating sample\n"),
      OPENDDS_STRING(GuidConverter(get_repo_id())).c_str()));
    done_ = true;
    return;
  }

  DDS::Time_t ts = {sample.header_.source_timestamp_sec_, sample.header_.source_timestamp_nanosec_};
  ACE_Time_Value atv = time_to_time_value(ts);

  GuidConverter pub(sample.header_.publication_id_);
  GuidConverter id(get_repo_id());
  ACE_INT64 seqN = sample.header_.sequence_.getValue();

  std::ostringstream oss;
  oss << '(' << atv.usec() << " usec) data_received by " << OPENDDS_STRING(id)
    << "\n    Writer " << OPENDDS_STRING(pub) << " seq#" << seqN
    << " key:" << data.key << " value:"  << data.value << "\n\n";

  ACE_DEBUG((LM_INFO, ACE_TEXT("%C"), oss.str().c_str()));

  RepoIdSeqNMap::iterator i = id_seqN_.find(sample.header_.publication_id_);
  if (i != id_seqN_.end()) {
    if (i->second < seqN) {
      i->second = seqN;
    } else {
      ACE_ERROR((LM_ERROR, ACE_TEXT("ERROR: seq#%q <= seq#%q(last)\n\n"), seqN, i->second));
    }
  } else {
    ACE_ERROR((LM_ERROR, ACE_TEXT("ERROR: received message from unassociated writer.\n")));
  }
}

// ========== ========== ========== ========== ========== ========== ==========
// private:
bool SimpleDataReader::deserializeEncapsulationHeader(Serializer& s) {
  ACE_CDR::ULong encap;
  return (s >> encap); // read and ignore 32-bit CDR Encapsulation header
}

bool SimpleDataReader::deserializeData(TestMsg& data, Serializer& s) {
  bool ok = deserializeEncapsulationHeader(s);
  ok &= (s >> data);
  if (!ok) {
    ACE_ERROR((LM_ERROR, ACE_TEXT("ERROR: failed to deserialize data\n")));
  }
  return ok;
}

bool SimpleDataReader::deserializeKey(TestMsg& data, Serializer& s) {
  bool ok = deserializeEncapsulationHeader(s);
  ok &= (s >> OpenDDS::DCPS::KeyOnly<TestMsg>(data));
  if (!ok) {
    ACE_ERROR((LM_ERROR, ACE_TEXT("ERROR: failed to deserialize key data\n")));
  }
  return ok;
}
