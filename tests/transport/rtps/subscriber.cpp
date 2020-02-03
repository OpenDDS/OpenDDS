#include "dds/DCPS/transport/rtps_udp/RtpsUdpInst.h"
#ifdef ACE_AS_STATIC_LIBS
#include "dds/DCPS/transport/rtps_udp/RtpsUdp.h"
#endif

#include "dds/DCPS/transport/framework/TransportRegistry.h"
#include "dds/DCPS/transport/framework/TransportReceiveListener.h"
#include "dds/DCPS/transport/framework/TransportClient.h"
#include "dds/DCPS/transport/framework/TransportExceptions.h"
#include "dds/DCPS/transport/framework/ReceivedDataSample.h"


#include "dds/DCPS/RepoIdBuilder.h"
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

#include "TestMsg.h"

using namespace OpenDDS::DCPS;

class SimpleDataReader : public TransportReceiveListener, public TransportClient
{
public:

  explicit SimpleDataReader(const RepoId& sub_id)
    : done_(false)
    , sub_id_(sub_id)
    , pub_id_(GUID_UNKNOWN)
    , seq_(SequenceNumber::ZERO())
    , control_msg_count_(0)
  {}

  virtual ~SimpleDataReader() {}

  bool init(const AssociationData& publication)
  {
    try {
      pub_id_ = publication.remote_id_;
      return associate(publication, false /* active */);
    } catch (const CORBA::BAD_PARAM& ) {
        ACE_ERROR((LM_ERROR, "ERROR: caught CORBA::BAD_PARAM exception\n"));
        return false;
    }
  }

  // Implementing TransportReceiveListener

  void data_received(const ReceivedDataSample& sample)
  {
    ++seq_;
    if (seq_ == 6) {
      ++seq_; // publisher.cpp deliberately skips #6 to test GAP generation
    }

    switch (sample.header_.message_id_) {
    case SAMPLE_DATA: {
      Serializer ser(sample.sample_.get(),
                     sample.header_.byte_order_ != ACE_CDR_BYTE_ORDER,
                     Serializer::ALIGN_CDR);
      bool ok = true;
      ACE_CDR::ULong encap;
      ok &= (ser >> encap); // read and ignore 32-bit CDR Encapsulation header
      TestMsg data;
      ok &= (ser >> data);

      if (!ok) {
        ACE_ERROR((LM_ERROR, "ERROR: failed to deserialize data\n"));
        return;
      }

      if (data.key == 99) {
        ACE_DEBUG((LM_INFO, "data_received() seq# = %d: terminating sample\n",
                   sample.header_.sequence_.getValue()));
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
      oss << "data_received() seq# = " << sample.header_.sequence_.getValue() << "\n\t"
        "id = " << int(sample.header_.message_id_) << "\n\t"
        "timestamp = " << atv.usec() << " usec " << timestr << "\t"
        "byte order = " << sample.header_.byte_order_ << "\n\t"
        "length = " << sample.header_.message_length_ << "\n\t"
        "publication = " << OPENDDS_STRING(pub) << "\n\t"
        "data.key = " << data.key << "\n\t"
        "data.value = " << data.value << "\n";
      ACE_DEBUG((LM_INFO, "%C", oss.str().c_str()));

      if (sample.header_.message_id_ != SAMPLE_DATA
          || sample.header_.sequence_ != seq_ || !sample.header_.byte_order_
          || sample.header_.message_length_ != 533
          || pub.checksum() != GuidConverter(pub_id_).checksum()) {
        ACE_ERROR((LM_ERROR, "ERROR: DataSampleHeader malformed\n"));
      }

      if (data.key != 0x09230923 || std::strlen(data.value.in()) != 520) {
        ACE_ERROR((LM_ERROR, "ERROR: DataSample contents malformed\n"));
      }
      break;
    }
    case INSTANCE_REGISTRATION:
    case DISPOSE_INSTANCE:
    case UNREGISTER_INSTANCE:
    case DISPOSE_UNREGISTER_INSTANCE: {
      OpenDDS::DCPS::Serializer ser(sample.sample_.get(),
                                    sample.header_.byte_order_ != ACE_CDR_BYTE_ORDER,
                                    OpenDDS::DCPS::Serializer::ALIGN_CDR);
      bool ok = true;
      ACE_CDR::ULong encap;
      ok &= (ser >> encap); // read and ignore 32-bit CDR Encapsulation header
      TestMsg data;
      ok &= (ser >> OpenDDS::DCPS::KeyOnly<TestMsg>(data));

      if (!ok) {
        ACE_ERROR((LM_ERROR, "ERROR: failed to deserialize key data\n"));
        return;
      }
      if (data.key == 0x04030201) {
        // Good control message
        control_msg_count_++;
      } else {
        ACE_ERROR((LM_ERROR, "ERROR: key contents malformed\n"));
      }

      std::ostringstream oss;
      oss << "data_received() seq# = " << sample.header_.sequence_.getValue();
      switch (sample.header_.message_id_) {
      case INSTANCE_REGISTRATION:
        oss << ": Instance Registration\n";
        break;
      case DISPOSE_INSTANCE:
        oss << ": Dispose Instance\n";
        break;
      case UNREGISTER_INSTANCE:
        oss << ": Unregister Instance\n";
        break;
      case DISPOSE_UNREGISTER_INSTANCE:
        oss << ": Dispose & Unregister Instance\n";
        break;
      }
      oss << "\tdata.key = " << data.key << "\n";
      ACE_DEBUG((LM_INFO, "%C", oss.str().c_str()));
      break;
    }
    }
  }

  void notify_subscription_disconnected(const WriterIdSeq&) {}
  void notify_subscription_reconnected(const WriterIdSeq&) {}
  void notify_subscription_lost(const WriterIdSeq&) {}
  void remove_associations(const WriterIdSeq&, bool) {}

  void _add_ref() {}
  void _remove_ref() {}

  // Implementing TransportClient
  bool check_transport_qos(const TransportInst&)
    { return true; }
  const RepoId& get_repo_id() const
    { return sub_id_; }
  DDS::DomainId_t domain_id() const
    { return 0; }
  CORBA::Long get_priority_value(const AssociationData&) const
    { return 0; }

  using TransportClient::enable_transport;
  using TransportClient::disassociate;

  bool done_;
  const RepoId sub_id_;
  RepoId pub_id_;
  SequenceNumber seq_;
  int control_msg_count_;
};


int
ACE_TMAIN(int argc, ACE_TCHAR* argv[])
{
  try {
    ::DDS::DomainParticipantFactory_var dpf =
      TheParticipantFactoryWithArgs(argc, argv);

    std::cerr << "STARTING MAIN IN SUBSCRIBER\n";
    ACE_TString host;
    u_short port = 0;

    ACE_Get_Opt opts(argc, argv, ACE_TEXT("h:p:"));
    int option = 0;

    while ((option = opts()) != EOF) {
      switch (option) {
      case 'h':
        host = opts.opt_arg();
        break;
      case 'p':
        port = static_cast<u_short>(ACE_OS::atoi(opts.opt_arg()));
        break;
      }
    }

    if (host.empty() || port == 0) {
      std::cerr << "ERROR: -h <host> and -p <port> options are required\n";
      return 1;
    }

    TransportInst_rch inst = TheTransportRegistry->create_inst("my_rtps",
                                                               "rtps_udp");

    RtpsUdpInst* rtps_inst = dynamic_cast<RtpsUdpInst*>(inst.in());
#ifdef OPENDDS_SAFETY_PROFILE
    if (host == "localhost") {
      host = "127.0.0.1";
    }
#endif
    rtps_inst->local_address(port, ACE_TEXT_ALWAYS_CHAR(host.c_str()));
    rtps_inst->datalink_release_delay_ = 0;

    TransportConfig_rch cfg = TheTransportRegistry->create_config("cfg");
    cfg->instances_.push_back(inst);

    TheTransportRegistry->global_config(cfg);

    RepoIdBuilder local;
    local.federationId(0x01234567);  // guidPrefix1
    local.participantId(0xefcdab89); // guidPrefix2
    local.entityKey(0x452310);
    local.entityKind(ENTITYKIND_USER_READER_WITH_KEY);

    RepoIdBuilder remote; // these values must match what's in publisher.cpp
    remote.federationId(0x01234567);  // guidPrefix1
    remote.participantId(0x89abcdef); // guidPrefix2
    remote.entityKey(0x012345);
    remote.entityKind(ENTITYKIND_USER_WRITER_WITH_KEY);

    SimpleDataReader sdr(local);
    sdr.enable_transport(false /*reliable*/, false /*durable*/);
    // Write a file so that test script knows we're ready
    FILE* file = std::fopen("subready.txt", "w");
    std::fprintf(file, "Ready\n");
    std::fclose(file);

    std::cerr << "***Ready written to subready.txt\n";

    AssociationData publication;
    publication.remote_id_ = remote;
    publication.remote_reliable_ = true;
    publication.remote_data_.length(1);
    publication.remote_data_[0].transport_type = "rtps_udp";
    publication.remote_data_[0].data.length(5);
    for (CORBA::ULong i = 0; i < 5; ++i) {
      publication.remote_data_[0].data[i] = 0;
    }

    std::cerr << "***Association Data created for Publication for SimpleDataReader to init\n";
    std::cout << "Associating with pub..." << std::endl;
    if (!sdr.init(publication)) {
      std::cerr << "subscriber TransportClient::associate() failed\n";
      return 1;
    }

    std::cerr << "***Simple Data Reader init:: publication completed\n";

    while (!sdr.done_) {
      ACE_OS::sleep(1);
    }

    if (sdr.control_msg_count_ != 4) {
      ACE_ERROR((LM_ERROR, "ERROR: Expected 4 control messages, received %d\n",
                 sdr.control_msg_count_));
    }

    sdr.disassociate(publication.remote_id_);

    TheServiceParticipant->shutdown();
    ACE_Thread_Manager::instance()->wait();

    return 0;
  } catch (const OpenDDS::DCPS::Transport::NotConfigured& ) {
    ACE_ERROR((LM_ERROR,
               "ERROR: caught OpenDDS::DCPS::Transport::NotConfigured exception.\n"));
    return 1;
  } catch (const CORBA::BAD_PARAM& ex) {
    ex._tao_print_exception("Exception caught in subscriber.cpp:");
    return 1;
  }
}
