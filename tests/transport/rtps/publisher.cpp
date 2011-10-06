#include "dds/DCPS/transport/rtps_udp/RtpsUdpInst.h"

#include "dds/DCPS/transport/framework/TransportRegistry.h"
#include "dds/DCPS/transport/framework/TransportSendListener.h"
#include "dds/DCPS/transport/framework/TransportClient.h"

#include "dds/DCPS/RTPS/RtpsMessageTypesTypeSupportImpl.h"
#include "dds/DCPS/RTPS/BaseMessageTypes.h"
#include "dds/DCPS/RTPS/BaseMessageUtils.h"

#include "dds/DCPS/RepoIdBuilder.h"
#include "dds/DCPS/Serializer.h"
#include "dds/DCPS/AssociationData.h"
#include "dds/DCPS/Service_Participant.h"
#include "dds/DCPS/DataSampleList.h"
#include "dds/DCPS/Qos_Helper.h"

#include <tao/CORBA_String.h>

#include <ace/OS_main.h>
#include <ace/Basic_Types.h>
#include <ace/CDR_Base.h>
#include <ace/String_Base.h>
#include <ace/Get_Opt.h>
#include <ace/SOCK_Dgram.h>
#include <ace/Message_Block.h>
#include <ace/OS_NS_sys_time.h>

#include <iostream>
#include <sstream>
#include <cstring>
#include <ctime>

#include "TestMsg.h"

using namespace OpenDDS::DCPS;
using namespace OpenDDS::RTPS;

void log_time(const ACE_Time_Value& t)
{
  const std::time_t seconds = t.sec();
  std::string timestr(std::ctime(&seconds));
  timestr.erase(timestr.size() - 1); // remove \n from ctime()
  ACE_DEBUG((LM_INFO, "Sending with timestamp %C %q usec\n",
             timestr.c_str(), ACE_INT64(t.usec())));
}

// sample text (pasted directly from the RTPS spec) to use in the message data
const char text[] = "Implementation of the protocol that are processing a "
  "received submessage should always use the octetsToInlineQos to skip "
  "any submessage headers it does not expect or understand and continue to "
  "process the inlineQos SubmessageElement (or the first submessage element "
  "that follows inlineQos if the inlineQos is not present). This fule is "
  "necessary so that the received will be able to interoperate with senders "
  "that use future versions of the protocol which may include additional "
  "submessage headers before the inlineQos.\n";

const bool host_is_bigendian = !ACE_CDR_BYTE_ORDER;

class SimpleDataWriter : public TransportSendListener, public TransportClient
{
public:

  explicit SimpleDataWriter(const RepoId& pub_id)
    : pub_id_(pub_id)
  {}

  virtual ~SimpleDataWriter() {}

  bool init(const AssociationData& publication)
  {
    sub_id_ = publication.remote_id_;
    return associate(publication, true /* active */);
  }

  // Implementing TransportSendListener
  void data_delivered(const DataSampleListElement*)
  {
    ACE_DEBUG((LM_INFO, "(%P|%t) SimpleDataWriter::data_delivered()\n"));
    --callbacks_expected_;
  }

  void data_dropped(const DataSampleListElement*, bool by_transport)
  {
    ACE_DEBUG((LM_INFO, "(%P|%t) SimpleDataWriter::data_dropped(element, %d)\n",
      int(by_transport)));
    --callbacks_expected_;
  }

  void notify_publication_disconnected(const ReaderIdSeq&) {}
  void notify_publication_reconnected(const ReaderIdSeq&) {}
  void notify_publication_lost(const ReaderIdSeq&) {}
  void notify_connection_deleted() {}
  void remove_associations(const ReaderIdSeq&, bool) {}

  // Implementing TransportClient
  bool check_transport_qos(const TransportInst&)
    { return true; }
  const RepoId& get_repo_id() const
    { return pub_id_; }
  CORBA::Long get_priority_value(const AssociationData&) const
    { return 0; }

  using TransportClient::enable_transport;
  using TransportClient::disassociate;
  using TransportClient::send;

  const RepoId& pub_id_;
  RepoId sub_id_;
  ssize_t callbacks_expected_;
};

int
ACE_TMAIN(int argc, ACE_TCHAR* argv[])
{
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

  // 0. initialization

  // transports can depend on ORB's reactor for timer scheduling
  CORBA::ORB_var orb = CORBA::ORB_init(argc, argv);
  TheServiceParticipant->set_ORB(orb);

  TransportInst_rch inst = TheTransportRegistry->create_inst("my_rtps",
                                                             "rtps_udp");

  RtpsUdpInst* rtps_inst = dynamic_cast<RtpsUdpInst*>(inst.in());
  rtps_inst->remote_address_.set(port, host.c_str());
  rtps_inst->datalink_release_delay_ = 0;

  TransportConfig_rch cfg = TheTransportRegistry->create_config("cfg");
  cfg->instances_.push_back(inst);

  TheTransportRegistry->global_config(cfg);

  RepoIdBuilder local;
  local.federationId(0x01234567);  // guidPrefix1
  local.participantId(0x89abcdef); // guidPrefix2
  local.entityKey(0x012345);
  local.entityKind(ENTITYKIND_USER_WRITER_WITH_KEY);
  OpenDDS::RTPS::GUID_t local_guid(local);
  const OpenDDS::RTPS::GuidPrefix_t& local_prefix = local_guid.guidPrefix;

  RepoIdBuilder remote; // these values must match what's in subscriber.cpp
  remote.federationId(0x01234567);  // guidPrefix1
  remote.participantId(0xefcdab89); // guidPrefix2
  remote.entityKey(0x452310);
  remote.entityKind(ENTITYKIND_USER_READER_WITH_KEY);

  SimpleDataWriter sdw(local_guid);
  sdw.enable_transport();
  AssociationData subscription;
  subscription.remote_id_ = remote;
  subscription.remote_data_.length(1);
  subscription.remote_data_[0].transport_type = "rtps_udp";

  if (!sdw.init(subscription)) {
    std::cerr << "publisher TransportClient::associate() failed\n";
    return 1;
  }

  // 1. send by directly writing an RTPS Message to the socket

  const Header hdr = { {'R', 'T', 'P', 'S'}, PROTOCOLVERSION, VENDORID_OPENDDS,
    {local_prefix[0], local_prefix[1], local_prefix[2], local_prefix[3],
     local_prefix[4], local_prefix[5], local_prefix[6], local_prefix[7],
     local_prefix[8], local_prefix[9], local_prefix[10], local_prefix[11]} };

  const ACE_Time_Value now = ACE_OS::gettimeofday();
  log_time(now);
  const double conv = 4294.967296; // NTP fractional (2^-32) sec per microsec
  const InfoTimestampSubmessage it = { {INFO_TS, 1, 8},
    {static_cast<ACE_CDR::Long>(now.sec()),
     static_cast<ACE_CDR::ULong>(now.usec() * conv)} };

  DataSubmessage ds = { {DATA, 7, 20 + 24 + 12 + sizeof(text)}, 0, 16,
    ENTITYID_UNKNOWN, local_guid.entityId, {0, 1}, ParameterList() };

  TestMsg data;
  data.key = 0x09230923;
  data.value = text;

  ds.inlineQos.length(1);
  OpenDDS::RTPS::KeyHash_t hash;
  marshal_key_hash(data, hash);
  ds.inlineQos[0].key_hash(hash);

  const ACE_CDR::ULong encap = 0x00000100; // {CDR_LE, options} in BE format
  size_t size = 0, padding = 0;
  gen_find_size(hdr, size, padding);
  gen_find_size(it, size, padding);
  gen_find_size(ds, size, padding);
  find_size_ulong(size, padding);
  gen_find_size(data, size, padding);

  ACE_Message_Block msg(size + padding);
  Serializer ser(&msg, host_is_bigendian, Serializer::ALIGN_CDR);
  bool ok = (ser << hdr) && (ser << it) && (ser << ds)
    && (ser << encap) && (ser << data);
  if (!ok) {
    std::cerr << "ERROR: failed to serialize RTPS message\n";
    return 1;
  }

  ACE_INET_Addr local_addr;
  ACE_SOCK_Dgram sock(local_addr);
  ssize_t res = sock.send(msg.rd_ptr(), msg.length(),
                          rtps_inst->remote_address_);
  if (res < 0) {
    std::cerr << "ERROR: error in send()\n";
    return 1;
  } else {
    std::ostringstream oss;
    oss << "Sent " << res << " bytes.\n";
    ACE_DEBUG((LM_INFO, oss.str().c_str()));
  }

  // 2. send through the OpenDDS transport

  TransportSendElementAllocator alloc(5, sizeof(TransportSendElementAllocator));
  DataSampleListElement elements[] = {
    DataSampleListElement(local_guid, &sdw, 0, &alloc, 0),  // Data Sample
    DataSampleListElement(local_guid, &sdw, 0, &alloc, 0),  // Instance Registration
    DataSampleListElement(local_guid, &sdw, 0, &alloc, 0),  // Dispose Instance
    DataSampleListElement(local_guid, &sdw, 0, &alloc, 0),  // Unregister Instance
    DataSampleListElement(local_guid, &sdw, 0, &alloc, 0),  // Data Sample (key=99 means end)
  };
  DataSampleList list;
  list.head_ = elements;
  list.size_ = sizeof(elements) / sizeof(elements[0]);
  list.tail_ = &elements[list.size_ - 1];
  for (int i = 0; i < list.size_ - 1; ++i) {
    elements[i].next_send_sample_ = &elements[i + 1];
  }

  // Send a regular data sample
  int index = 0;
  DataSampleHeader dsh;
  dsh.message_id_ = SAMPLE_DATA;
  dsh.publication_id_ = local_guid;
  dsh.sequence_ = 2;
  elements[index].sequence_ = dsh.sequence_;
  const ACE_Time_Value t = ACE_OS::gettimeofday();
  log_time(t);
  elements[index].source_timestamp_ = time_value_to_time(t);

  // Calculate the data buffer length
  dsh.message_length_ = 0;
  padding = 0;
  find_size_ulong(dsh.message_length_, padding);   // encap
  gen_find_size(data, dsh.message_length_, padding);
  dsh.message_length_ += padding;

  elements[index].sample_ =
    new ACE_Message_Block(DataSampleHeader::max_marshaled_size(),
      ACE_Message_Block::MB_DATA, new ACE_Message_Block(dsh.message_length_));

  *elements[index].sample_ << dsh;

  Serializer ser2(elements[index].sample_->cont(), host_is_bigendian,
                  Serializer::ALIGN_CDR);
  ok = (ser2 << encap) && (ser2 << data);
  if (!ok) {
    std::cerr << "ERROR: failed to serialize data for elements[" << index << "]\n";
    return 1;
  }

  // Send an instance registration
  index++;
  data.key = 0x04030201;
  dsh.message_id_ = INSTANCE_REGISTRATION;
  dsh.sequence_ = SequenceNumber::SEQUENCENUMBER_UNKNOWN();
  dsh.publication_id_ = local_guid;
  dsh.key_fields_only_ = true;

  // Calculate the data buffer length
  dsh.message_length_ = 0;
  padding = 0;
  OpenDDS::DCPS::KeyOnly<const TestMsg> ko_instance_data(data);
  find_size_ulong(dsh.message_length_, padding);   // encap
  gen_find_size(ko_instance_data, dsh.message_length_, padding);
  dsh.message_length_ += padding;
  
  elements[index].sample_ = new ACE_Message_Block(DataSampleHeader::max_marshaled_size(),
					      ACE_Message_Block::MB_DATA,
					      new ACE_Message_Block(dsh.message_length_));
  *elements[index].sample_ << dsh;

  OpenDDS::DCPS::Serializer serializer(elements[index].sample_->cont(),
				       host_is_bigendian,
				       Serializer::ALIGN_CDR);
  ok = (serializer << encap) && (serializer << ko_instance_data);
  if (!ok) {
    std::cerr << "ERROR: failed to serialize data for elements[" << index << "]\n";
    return 1;
  }

  // Send a dispose instance
  index++;
  {
    dsh.message_id_ = DISPOSE_INSTANCE;
    elements[index].sample_ = new ACE_Message_Block(DataSampleHeader::max_marshaled_size(),
						    ACE_Message_Block::MB_DATA,
						    new ACE_Message_Block(dsh.message_length_));
    *elements[index].sample_ << dsh;
    OpenDDS::DCPS::Serializer serializer(elements[index].sample_->cont(),
					 host_is_bigendian,
					 Serializer::ALIGN_CDR);
    ok = (serializer << encap) && (serializer << ko_instance_data);
    if (!ok) {
      std::cerr << "ERROR: failed to serialize data for elements[" << index << "]\n";
      return 1;
    }
  }

  // Send an unregister instance
  index++;
  {
    dsh.message_id_ = UNREGISTER_INSTANCE;
    elements[index].sample_ = new ACE_Message_Block(DataSampleHeader::max_marshaled_size(),
						    ACE_Message_Block::MB_DATA,
						    new ACE_Message_Block(dsh.message_length_));
    *elements[index].sample_ << dsh;
    OpenDDS::DCPS::Serializer serializer(elements[index].sample_->cont(),
					 host_is_bigendian,
					 Serializer::ALIGN_CDR);
    ok = (serializer << encap) && (serializer << ko_instance_data);
    if (!ok) {
      std::cerr << "ERROR: failed to serialize data for elements[" << index << "]\n";
      return 1;
    }
  }

  // Send a data sample with a key of 99 to terminate the subscriber
  index++;
  dsh.sequence_ = 3;
  dsh.message_id_ = SAMPLE_DATA;
  dsh.key_fields_only_ = false;
  elements[index].sequence_ = dsh.sequence_;
  elements[index].source_timestamp_ = time_value_to_time(ACE_OS::gettimeofday());
  data.key = 99;
  data.value = "";

  // Calculate the data buffer length
  dsh.message_length_ = 0;
  padding = 0;
  find_size_ulong(dsh.message_length_, padding);   // encap
  gen_find_size(data, dsh.message_length_, padding);
  dsh.message_length_ += padding;

  elements[index].sample_ =
    new ACE_Message_Block(DataSampleHeader::max_marshaled_size(),
      ACE_Message_Block::MB_DATA, new ACE_Message_Block(dsh.message_length_));

  *elements[index].sample_ << dsh;

  Serializer ser3(elements[index].sample_->cont(), host_is_bigendian,
                  Serializer::ALIGN_CDR);
  ok = (ser3 << encap) && (ser3 << data.key) && (ser3 << data.value);
  if (!ok) {
    std::cerr << "ERROR: failed to serialize data for elements[" << index << "]\n";
    return 1;
  }

  sdw.callbacks_expected_ = list.size_;
  sdw.send(list);

  while (sdw.callbacks_expected_) {
    ACE_OS::sleep(1);
  }

  // 3. cleanup

  sdw.disassociate(subscription.remote_id_);

  TheServiceParticipant->shutdown();
  orb->shutdown();
  orb->destroy();
  ACE_Thread_Manager::instance()->wait();

  return 0;
}
