#include "dds/DCPS/transport/rtps_udp/RtpsUdpInst.h"

#include "dds/DCPS/transport/framework/TransportRegistry.h"
#include "dds/DCPS/transport/framework/TransportReceiveListener.h"
#include "dds/DCPS/transport/framework/TransportClient.h"

#include "dds/DCPS/RepoIdBuilder.h"
#include "dds/DCPS/GuidConverter.h"
#include "dds/DCPS/AssociationData.h"
#include "dds/DCPS/Service_Participant.h"

#include <ace/OS_main.h>
#include <ace/String_Base.h>
#include <ace/Get_Opt.h>

#include <cstdio>
#include <iostream>
#include <sstream>

using namespace OpenDDS::DCPS;

class SimpleDataReader : public TransportReceiveListener, public TransportClient
{
public:

  explicit SimpleDataReader(const RepoId& sub_id)
    : message_received_(false)
    , sub_id_(sub_id)
  {}

  virtual ~SimpleDataReader() {}

  bool init(const AssociationData& publication)
  {
    return associate(publication, false /* active */);
  }

  // Implementing TransportReceiveListener

  void data_received(const ReceivedDataSample& sample)
  {
    message_received_ = true;

    Serializer ser(sample.sample_,
                   sample.header_.byte_order_ != ACE_CDR_BYTE_ORDER,
                   Serializer::ALIGN_CDR);
    ACE_CDR::ULong key;
    bool ok = true;
    ok &= (ser >> key); // read and ignore 32-bit CDR Encapsulation header
    ok &= (ser >> key);
    CORBA::String_var value;
    ok &= (ser >> value.out());

    GuidConverter pub(sample.header_.publication_id_);
    std::ostringstream oss;
     oss << "data_received():\n\t"
      "id = " << int(sample.header_.message_id_) << "\n\t"
      "seq# = " << sample.header_.sequence_.getValue() << "\n\t"
      "byte order = " << sample.header_.byte_order_ << "\n\t"
      "length = " << sample.header_.message_length_ << "\n\t"
      "publication = " << pub << "\n\t"
      "key = " << key << "\n\t"
      "value = " << value << "\n";
    ACE_DEBUG((LM_INFO, "%C", oss.str().c_str()));
  }

  void notify_subscription_disconnected(const WriterIdSeq&) {}
  void notify_subscription_reconnected(const WriterIdSeq&) {}
  void notify_subscription_lost(const WriterIdSeq&) {}
  void notify_connection_deleted() {}
  void remove_associations(const WriterIdSeq&, bool) {}

  // Implementing TransportClient
  bool check_transport_qos(const TransportInst&)
    { return true; }
  const RepoId& get_repo_id() const
    { return sub_id_; }
  CORBA::Long get_priority_value(const AssociationData&) const
    { return 0; }

  using TransportClient::enable_transport;
  using TransportClient::disassociate;

  bool message_received_;
  const RepoId& sub_id_;
};


int
ACE_TMAIN(int argc, ACE_TCHAR* argv[])
{
  ACE_TString host;
  u_short port;

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

  // transports can depend on ORB's reactor for timer scheduling
  CORBA::ORB_var orb = CORBA::ORB_init(argc, argv);
  TheServiceParticipant->set_ORB(orb);

  TransportInst_rch inst = TheTransportRegistry->create_inst("my_rtps",
                                                             "rtps_udp");

  RtpsUdpInst* rtps_inst = dynamic_cast<RtpsUdpInst*>(inst.in());
  rtps_inst->local_address_.set(port, host.c_str());

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
  sdr.enable_transport();
  // Write a file so that test script knows we're ready
  FILE* file = std::fopen("subready.txt", "w");
  std::fprintf(file, "Ready\n");
  std::fclose(file);

  AssociationData publication;
  publication.remote_id_ = remote;
  publication.remote_data_.length(1);
  publication.remote_data_[0].transport_type = "rtps_udp";

  std::cout << "Associating with pub..." << std::endl;
  if (!sdr.init(publication)) {
    std::cerr << "TransportClient::associate() failed\n";
    return 1;
  }

  while (!sdr.message_received_) {
    ACE_OS::sleep(1);
  }

  sdr.disassociate(publication.remote_id_);

  TheTransportRegistry->release();
  ACE_Thread_Manager::instance()->wait();

  return 0;
}
