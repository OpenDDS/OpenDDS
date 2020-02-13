#include "TestMsg.h"
#include "AppConfig.h"

#include "dds/DCPS/transport/framework/TransportReceiveListener.h"
#include "dds/DCPS/transport/framework/TransportClient.h"
#include "dds/DCPS/transport/framework/TransportExceptions.h"
#include "dds/DCPS/transport/framework/ReceivedDataSample.h"

#include "dds/DCPS/GuidConverter.h"
#include "dds/DCPS/AssociationData.h"
#include "dds/DCPS/Service_Participant.h"
#include "dds/DCPS/Qos_Helper.h"

#include <ace/OS_main.h>
#include <ace/String_Base.h>
#include <ace/Get_Opt.h>
#include <ace/OS_NS_time.h>
#include "ace/OS_NS_unistd.h"

#include <cstdio>
#include <cstring>
#include <ctime>
#include <iostream>
#include <sstream>

class SimpleDataReader : public TransportReceiveListener, public TransportClient
{
public:
  SimpleDataReader(const AppConfig& ac, const int readerIndex, AssociationData& publication)
    : config(ac), index(readerIndex), done_(false), seq_()
  {
    enable_transport(config.readersReliable(), false); //(reliable, durable)

    if (index == 1) {
      writeSubReady();
    }

    publication.remote_id_ = config.getPubWtrId();
    if (!associate(publication, false)) {
      throw std::string("subscriber TransportClient::associate() failed");
    }
    std::cerr << "Reader " << OPENDDS_STRING(GuidConverter(get_repo_id())) << " called associate()\n";
  }

  virtual ~SimpleDataReader() { disassociate(config.getPubWtrId()); }
  bool done() { return done_; }

  // Implementing TransportReceiveListener
  void data_received(const ReceivedDataSample& sample);
  void notify_subscription_disconnected(const WriterIdSeq&) {}
  void notify_subscription_reconnected(const WriterIdSeq&) {}
  void notify_subscription_lost(const WriterIdSeq&) {}
  void remove_associations(const WriterIdSeq&, bool) {}
  void _add_ref() {}
  void _remove_ref() {}

  // Implementing TransportClient
  bool check_transport_qos(const TransportInst&) { return true; }
  const RepoId& get_repo_id() const { return config.getSubRdrId(index); }
  DDS::DomainId_t domain_id() const { return 0; }
  CORBA::Long get_priority_value(const AssociationData&) const { return 0; }

private:
  bool deserializeEncapsulationHeader(Serializer& s) {
    ACE_CDR::ULong encap;
    return (s >> encap); // read and ignore 32-bit CDR Encapsulation header
  }
  bool deserializeData(TestMsg& data, Serializer& s) {
    bool ok = deserializeEncapsulationHeader(s);
    ok &= (s >> data);
    if (!ok) {
      ACE_ERROR((LM_ERROR, ACE_TEXT("ERROR: failed to deserialize data\n")));
    }
    return ok;
  }
  bool deserializeKey(TestMsg& data, Serializer& s) {
    bool ok = deserializeEncapsulationHeader(s);
    ok &= (s >> OpenDDS::DCPS::KeyOnly<TestMsg>(data));
    if (!ok) {
      ACE_ERROR((LM_ERROR, ACE_TEXT("ERROR: failed to deserialize key data\n")));
    }
    return ok;
  }

  void writeSubReady() {
    FILE* file = std::fopen("subready.txt", "w");
    std::fprintf(file, "Ready\n");
    std::fclose(file);
  }

  const AppConfig& config;
  const int index;
  bool done_;
  SequenceNumber seq_;
};

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

  GuidConverter pub(sample.header_.publication_id_);
  DDS::Time_t ts = {sample.header_.source_timestamp_sec_,
                    sample.header_.source_timestamp_nanosec_};
  ACE_Time_Value atv = time_to_time_value(ts);
  std::time_t seconds = atv.sec();
  ACE_TCHAR buffer[32];
  std::string timestr(ACE_TEXT_ALWAYS_CHAR(ACE_OS::ctime_r(&seconds, buffer, 32)));
  std::ostringstream oss;
  oss << "data_received() by " << OPENDDS_STRING(GuidConverter(get_repo_id())).c_str() << "\n\t"
    "id = "          << int(sample.header_.message_id_) << "\n\t"
    "timestamp = "   << atv.usec() << " usec " << timestr << "\t"
    "seq# = "        << sample.header_.sequence_.getValue() << "\n\t"
    "byte order = "  << sample.header_.byte_order_ << "\n\t"
    "length = "      << sample.header_.message_length_ << "\n\t"
    "publication = " << OPENDDS_STRING(pub) << "\n\t"
    "data.key = "    << data.key << "\n\t"
    "data.value = "  << data.value << "\n";
  ACE_DEBUG((LM_INFO, ACE_TEXT("%C"), oss.str().c_str()));

  if (!sample.header_.byte_order_ ||
      pub.checksum() != GuidConverter(config.getPubWtrId()).checksum()) {
    ACE_ERROR((LM_ERROR, ACE_TEXT("ERROR: DataSampleHeader malformed\n")));
  }

  if (sample.header_.sequence_ == 2 && index != 1) {
    ACE_ERROR((LM_ERROR, ACE_TEXT("ERROR: received unmatched Directed Write message\n")));
  }
}

class DDS_TEST {
public:
  DDS_TEST(int argc, ACE_TCHAR* argv[]) : config(argc, argv, true) {
    publication.remote_reliable_ = true;
    publication.remote_data_.length(1);
    publication.remote_data_[0].transport_type = "rtps_udp";
    publication.remote_data_[0].data.length(5);
    for (CORBA::ULong i = 0; i < 5; ++i) {
      publication.remote_data_[0].data[i] = 0;
    }
  }

  int run() {
    SimpleDataReader reader0(config, 0, publication);
    SimpleDataReader reader1(config, 1, publication);
    while (!(reader0.done() && reader1.done())) {
      ACE_OS::sleep(1);
    }
    return 0;
  }

private:
  AppConfig config;
  AssociationData publication;
};

int ACE_TMAIN(int argc, ACE_TCHAR* argv[])
{
  try {
    DDS_TEST test(argc, argv);
    return test.run();
  } catch (...) {
    return 1;
  }
}
