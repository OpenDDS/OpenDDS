#include "dds/DCPS/transport/rtps_udp/RtpsUdpInst.h"
#include "dds/DCPS/transport/rtps_udp/RtpsUdpDataLink.h"
#ifdef ACE_AS_STATIC_LIBS
#include "dds/DCPS/transport/rtps_udp/RtpsUdp.h"
#endif

#include "dds/DCPS/transport/framework/TransportRegistry.h"
#include "dds/DCPS/transport/framework/TransportSendListener.h"
#include "dds/DCPS/transport/framework/TransportClient.h"
#include "dds/DCPS/transport/framework/TransportExceptions.h"

#include "dds/DCPS/RTPS/RtpsCoreTypeSupportImpl.h"
#include "dds/DCPS/RTPS/BaseMessageTypes.h"
#include "dds/DCPS/RTPS/BaseMessageUtils.h"

#include "dds/DCPS/RepoIdBuilder.h"
#include "dds/DCPS/Serializer.h"
#include "dds/DCPS/AssociationData.h"
#include "dds/DCPS/Service_Participant.h"
#include "dds/DCPS/SendStateDataSampleList.h"
#include "dds/DCPS/DataSampleElement.h"
#include "dds/DCPS/Qos_Helper.h"
#include "dds/DCPS/Marked_Default_Qos.h"
#include "dds/DCPS/Message_Block_Ptr.h"

#include <tao/CORBA_String.h>

#include <ace/OS_main.h>
#include <ace/Basic_Types.h>
#include <ace/CDR_Base.h>
#include <ace/String_Base.h>
#include <ace/Get_Opt.h>
#include <ace/SOCK_Dgram.h>
#include <ace/Message_Block.h>
#include <ace/OS_NS_sys_time.h>
#include <ace/OS_NS_time.h>
#include "ace/OS_NS_unistd.h"

#include <iostream>
#include <sstream>
#include <cstring>
#include <ctime>

#include "TestMsg.h"

class DDS_TEST {  // friended by RtpsUdpDataLink and DataSampleElement
public:
  static void force_inline_qos(bool val) {
    OpenDDS::DCPS::RtpsUdpDataLink::force_inline_qos_ = val;
  }

  static void set_next_send_sample(DataSampleElement& element, DataSampleElement *next_send_sample) {
    element.set_next_send_sample(next_send_sample);
  }

  static int test(ACE_TString host, u_short port);
};


void log_time(const ACE_Time_Value& t)
{
  ACE_TCHAR buffer[32];
  const std::time_t seconds = t.sec();
  std::string timestr(ACE_TEXT_ALWAYS_CHAR(ACE_OS::ctime_r(&seconds, buffer, 32)));
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
    , sub_id_(GUID_UNKNOWN)
    , callbacks_expected_(0)
    , inline_qos_mode_(DEFAULT_QOS)
  {
  }

  virtual ~SimpleDataWriter() {}

  bool init(const AssociationData& publication)
  {
    sub_id_ = publication.remote_id_;
    return associate(publication, true /* active */);
  }

  // Implementing TransportSendListener
  void data_delivered(const DataSampleElement*)
  {
    ACE_DEBUG((LM_INFO, "(%P|%t) SimpleDataWriter::data_delivered()\n"));
    --callbacks_expected_;
  }

  void data_dropped(const DataSampleElement*, bool by_transport)
  {
    ACE_DEBUG((LM_INFO, "(%P|%t) SimpleDataWriter::data_dropped(element, %d)\n",
      int(by_transport)));
    --callbacks_expected_;
  }

  void control_delivered(const Message_Block_Ptr& /*sample*/)
  {
    ACE_DEBUG((LM_INFO, "(%P|%t) SimpleDataWriter::control_delivered()\n"));
  }

  void control_dropped(const Message_Block_Ptr& /*sample*/,
                       bool /*dropped_by_transport*/)
  {
    ACE_DEBUG((LM_INFO, "(%P|%t) SimpleDataWriter::control_dropped()\n"));
  }

  // Enum to define qos returned by this object when populating inline qos
  // This will determine which qos policies are placed in the submessage.
  enum InlineQosMode {
    DEFAULT_QOS,       // Use the default values for all pub and dw qos
    PARTIAL_MOD_QOS,   // Modify some (but not all) qos values
    FULL_MOD_QOS       // Modify all qos values.
  };

  void notify_publication_disconnected(const ReaderIdSeq&) {}
  void notify_publication_reconnected(const ReaderIdSeq&) {}
  void notify_publication_lost(const ReaderIdSeq&) {}
  void remove_associations(const ReaderIdSeq&, bool) {}
  void _add_ref() {}
  void _remove_ref() {}

  virtual void retrieve_inline_qos_data(InlineQosData& qos_data) const
  {
    qos_data.dw_qos     = DATAWRITER_QOS_DEFAULT;
    qos_data.pub_qos    = PUBLISHER_QOS_DEFAULT;
    qos_data.topic_name = "My Topic ";  // Topic name is always included in inline qos
    switch (this->inline_qos_mode_) {
    case FULL_MOD_QOS:
      qos_data.pub_qos.presentation.access_scope = DDS::GROUP_PRESENTATION_QOS;
      qos_data.dw_qos.durability.kind = DDS::PERSISTENT_DURABILITY_QOS;
      qos_data.dw_qos.deadline.period.sec = 10;
      qos_data.dw_qos.latency_budget.duration.sec = 11;
#ifndef OPENDDS_NO_OWNERSHIP_KIND_EXCLUSIVE
      qos_data.dw_qos.ownership.kind = DDS::EXCLUSIVE_OWNERSHIP_QOS;
#endif
      /* Falls through. */
    case PARTIAL_MOD_QOS:
      qos_data.pub_qos.partition.name.length(1);
      qos_data.pub_qos.partition.name[0] = "Hello";
#ifndef OPENDDS_NO_OWNERSHIP_KIND_EXCLUSIVE
      qos_data.dw_qos.ownership_strength.value = 12;
#endif
      qos_data.dw_qos.liveliness.kind = DDS::MANUAL_BY_TOPIC_LIVELINESS_QOS;
      qos_data.dw_qos.reliability.kind = DDS::BEST_EFFORT_RELIABILITY_QOS;
      qos_data.dw_qos.transport_priority.value = 13;
      qos_data.dw_qos.lifespan.duration.sec = 14;
      qos_data.dw_qos.destination_order.kind = DDS::BY_SOURCE_TIMESTAMP_DESTINATIONORDER_QOS;
    case DEFAULT_QOS:
      break;
    }
  }

  // Implementing TransportClient
  bool check_transport_qos(const TransportInst&)
    { return true; }
  const RepoId& get_repo_id() const
    { return pub_id_; }
  DDS::DomainId_t domain_id() const
    { return 0; }
  CORBA::Long get_priority_value(const AssociationData&) const
    { return 0; }

  using TransportClient::enable_transport;
  using TransportClient::disassociate;
  using TransportClient::send;
  using TransportClient::send_control;

  const RepoId pub_id_;
  RepoId sub_id_;
  ssize_t callbacks_expected_;
  InlineQosMode inline_qos_mode_;
};

using namespace OpenDDS::DCPS;
using namespace OpenDDS::RTPS;

int DDS_TEST::test(ACE_TString host, u_short port)
{
  if (host.empty() || port == 0) {
    std::cerr << "ERROR: -h <host> and -p <port> options are required\n";
    return 1;
  }

  // 0. initialization

#ifdef OPENDDS_SAFETY_PROFILE
  if (host == "localhost") {
    host = "127.0.0.1";
  }
#endif
  ACE_INET_Addr remote_addr;
  if (host == ACE_TEXT("localhost")) {
    remote_addr = ACE_INET_Addr(port, host.c_str());
  } else {
    remote_addr = ACE_INET_Addr(port, host.c_str(), AF_INET);
  }

  TransportInst_rch inst = TheTransportRegistry->create_inst("my_rtps",
                                                             "rtps_udp");

  RtpsUdpInst* rtps_inst = dynamic_cast<RtpsUdpInst*>(inst.in());
  if (!rtps_inst) {
    std::cerr << "ERROR: Failed to cast to RtpsUdpInst\n";
    return 1;
  }
  rtps_inst->datalink_release_delay_ = 0;
  rtps_inst->heartbeat_period_ = TimeDuration::from_msec(100);

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

  LocatorSeq locators;
  locators.length(1);
  locators[0].kind = address_to_kind(remote_addr);
  locators[0].port = remote_addr.get_port_number();
  address_to_bytes(locators[0].address, remote_addr);

  size_t size_locator = 0, padding_locator = 0;
  gen_find_size(locators, size_locator, padding_locator);
  ACE_Message_Block mb_locator(size_locator + padding_locator + 1);
  Serializer ser_loc(&mb_locator, ACE_CDR_BYTE_ORDER, Serializer::ALIGN_CDR);
  ser_loc << locators;
  ser_loc << ACE_OutputCDR::from_boolean(false); // requires inline QoS

  SimpleDataWriter sdw(local_guid);
  sdw.enable_transport(true /*reliable*/, false /*durable*/);
  AssociationData subscription;
  subscription.remote_id_ = remote;
  subscription.remote_reliable_ = false;
  subscription.remote_data_.length(1);
  subscription.remote_data_[0].transport_type = "rtps_udp";
  message_block_to_sequence (mb_locator, subscription.remote_data_[0].data);

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
    {static_cast<ACE_CDR::ULong>(now.sec()),
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
  ACE_SOCK_Dgram sock;
  if (!open_appropriate_socket_type(sock, local_addr)) {
    ACE_ERROR_RETURN((LM_ERROR,
      ACE_TEXT("(%P|%t) ERROR: ")
      ACE_TEXT("publisher: open_appropriate_socket_type:")
      ACE_TEXT("%m\n")),
      false);
  }

  ACE_INET_Addr dest;
  locator_to_address(dest, locators[0], local_addr.get_type() != AF_INET);
  ssize_t res = sock.send(msg.rd_ptr(), msg.length(), dest);
  if (res < 0) {
    std::cerr << "ERROR: error in send()\n";
    return 1;
  } else {
    std::ostringstream oss;
    oss << "Sent " << res << " bytes.\n";
    ACE_DEBUG((LM_INFO, oss.str().c_str()));
  }

  // 2a. send control messages through the OpenDDS transport

  // Send an instance registration
  {
    TestMsg control_sample;
    control_sample.key = 0x04030201;
    DataSampleHeader dsh;
    dsh.message_id_ = INSTANCE_REGISTRATION;
    dsh.sequence_ = 2;
    dsh.publication_id_ = local_guid;
    dsh.key_fields_only_ = true;

    // Calculate the data buffer length
    size = 0;
    padding = 0;
    OpenDDS::DCPS::KeyOnly<const TestMsg> ko_instance_data(control_sample);
    find_size_ulong(size, padding);   // encap
    gen_find_size(ko_instance_data, size, padding);
    dsh.message_length_ = static_cast<ACE_UINT32>(size + padding);

    {
      OpenDDS::DCPS::Message_Block_Ptr ir_mb (new ACE_Message_Block(DataSampleHeader::max_marshaled_size(),
                                                       ACE_Message_Block::MB_DATA,
                                                       new ACE_Message_Block(dsh.message_length_)));
      *ir_mb << dsh;

      OpenDDS::DCPS::Serializer serializer(ir_mb->cont(),
                                           host_is_bigendian,
                                           Serializer::ALIGN_CDR);
      ok = (serializer << encap) && (serializer << ko_instance_data);
      if (!ok) {
        std::cerr << "ERROR: failed to serialize data for instance registration\n";
        return 1;
      }
      ::DDS_TEST::force_inline_qos(false);  // No inline QoS
      sdw.send_control(dsh, OpenDDS::DCPS::move(ir_mb));
    }

    // Send a dispose instance
    {
      dsh.message_id_ = DISPOSE_INSTANCE;
      dsh.sequence_ = 3;
      OpenDDS::DCPS::Message_Block_Ptr di_mb (new ACE_Message_Block(DataSampleHeader::max_marshaled_size(),
                                                       ACE_Message_Block::MB_DATA,
                                                       new ACE_Message_Block(dsh.message_length_)));
      *di_mb << dsh;
      OpenDDS::DCPS::Serializer serializer(di_mb->cont(),
                                           host_is_bigendian,
                                           Serializer::ALIGN_CDR);
      ok = (serializer << encap) && (serializer << ko_instance_data);
      if (!ok) {
        std::cerr << "ERROR: failed to serialize data for dispose instance\n";
        return 1;
      }
      ::DDS_TEST::force_inline_qos(true);  // Inline QoS
      sdw.inline_qos_mode_ = SimpleDataWriter::PARTIAL_MOD_QOS;
      sdw.send_control(dsh, OpenDDS::DCPS::move(di_mb));
    }

    // Send an unregister instance
    {
      dsh.message_id_ = UNREGISTER_INSTANCE;
      dsh.sequence_ = 4;
      OpenDDS::DCPS::Message_Block_Ptr ui_mb  (new ACE_Message_Block(DataSampleHeader::max_marshaled_size(),
                                                       ACE_Message_Block::MB_DATA,
                                                       new ACE_Message_Block(dsh.message_length_)));
      *ui_mb << dsh;
      OpenDDS::DCPS::Serializer serializer(ui_mb->cont(),
                                           host_is_bigendian,
                                           Serializer::ALIGN_CDR);
      ok = (serializer << encap) && (serializer << ko_instance_data);
      if (!ok) {
        std::cerr << "ERROR: failed to serialize data for unregister instance\n";
        return 1;
      }
      ::DDS_TEST::force_inline_qos(true);  // Inline QoS
      sdw.inline_qos_mode_ = SimpleDataWriter::FULL_MOD_QOS;
      sdw.send_control(dsh, OpenDDS::DCPS::move(ui_mb));
    }

    // Send a dispose & unregister instance
    {
      dsh.message_id_ = DISPOSE_UNREGISTER_INSTANCE;
      dsh.sequence_ = 5;
      OpenDDS::DCPS::Message_Block_Ptr ui_mb (new ACE_Message_Block(DataSampleHeader::max_marshaled_size(),
                                                       ACE_Message_Block::MB_DATA,
                                                       new ACE_Message_Block(dsh.message_length_)));
      *ui_mb << dsh;
      OpenDDS::DCPS::Serializer serializer(ui_mb->cont(),
                                           host_is_bigendian,
                                           Serializer::ALIGN_CDR);
      ok = (serializer << encap) && (serializer << ko_instance_data);
      if (!ok) {
        std::cerr << "ERROR: failed to serialize data for dispose unregister instance\n";
        return 1;
      }
      ::DDS_TEST::force_inline_qos(true);  // Inline QoS
      sdw.inline_qos_mode_ = SimpleDataWriter::FULL_MOD_QOS;
      sdw.send_control(dsh, OpenDDS::DCPS::move(ui_mb));
    }
  }

  // 2b. send sample data through the OpenDDS transport

  DataSampleElement elements[] = {
    DataSampleElement(local_guid, &sdw, OpenDDS::DCPS::PublicationInstance_rch()),  // Data Sample
    DataSampleElement(local_guid, &sdw, OpenDDS::DCPS::PublicationInstance_rch()),  // Data Sample (key=99 means end)
  };
  SendStateDataSampleList list;
  list.head_ = elements;
  list.size_ = sizeof(elements) / sizeof(elements[0]);
  list.tail_ = &elements[list.size() - 1];
  for (int i = 0; i < list.size() - 1; ++i) {
    DDS_TEST::set_next_send_sample(elements[i], &elements[i + 1]);
  }

  // Send a regular data sample
  int index = 0;
  DataSampleHeader& dsh = elements[index].header_;
  dsh.message_id_ = SAMPLE_DATA;
  dsh.publication_id_ = local_guid;
  dsh.sequence_ = 7; // test GAP generation
  const ACE_Time_Value tv = ACE_OS::gettimeofday();
  log_time(tv);
  DDS::Time_t st = time_value_to_time(tv);
  dsh.source_timestamp_sec_ = st.sec;
  dsh.source_timestamp_nanosec_ = st.nanosec;

  // Calculate the data buffer length
  size = 0;
  padding = 0;
  find_size_ulong(size, padding);   // encap
  gen_find_size(data, size, padding);
  dsh.message_length_ = static_cast<ACE_UINT32>(size + padding);

  elements[index].sample_.reset(
    new ACE_Message_Block(DataSampleHeader::max_marshaled_size(),
      ACE_Message_Block::MB_DATA, new ACE_Message_Block(dsh.message_length_)));

  *elements[index].sample_ << dsh;

  Serializer ser2(elements[index].sample_->cont(), host_is_bigendian,
                  Serializer::ALIGN_CDR);
  ok = (ser2 << encap) && (ser2 << data);
  if (!ok) {
    std::cerr << "ERROR: failed to serialize data for elements[" << index << "]\n";
    return 1;
  }

  // Send a data sample with a key of 99 to terminate the subscriber
  index++;
  DataSampleHeader& dsh2 = elements[index].header_;
  dsh2.sequence_ = dsh.sequence_+1;
  dsh2.message_id_ = SAMPLE_DATA;
  dsh.publication_id_ = local_guid;
  dsh2.key_fields_only_ = false;
  const ACE_Time_Value tv2 = ACE_OS::gettimeofday();
  log_time(tv2);
  DDS::Time_t st2 = time_value_to_time(tv2);
  dsh2.source_timestamp_sec_ = st2.sec;
  dsh2.source_timestamp_nanosec_ = st2.nanosec;
  data.key = 99;
  data.value = "";

  // Calculate the data buffer length
  size = 0;
  padding = 0;
  find_size_ulong(size, padding);   // encap
  gen_find_size(data, size, padding);
  dsh2.message_length_ = static_cast<ACE_UINT32>(size + padding);

  elements[index].sample_.reset(
    new ACE_Message_Block(DataSampleHeader::max_marshaled_size(),
      ACE_Message_Block::MB_DATA, new ACE_Message_Block(dsh2.message_length_)));

  *elements[index].sample_ << dsh2;

  Serializer ser3(elements[index].sample_->cont(), host_is_bigendian,
                  Serializer::ALIGN_CDR);
  ok = (ser3 << encap) && (ser3 << data.key) && (ser3 << data.value);
  if (!ok) {
    std::cerr << "ERROR: failed to serialize data for elements[" << index << "]\n";
    return 1;
  }

  sdw.callbacks_expected_ = list.size();
  ::DDS_TEST::force_inline_qos(true);  // Inline QoS
  sdw.send(list);

  while (sdw.callbacks_expected_) {
    ACE_OS::sleep(1);
  }

  // Allow enough time for a HEARTBEAT message to be generated
  ACE_OS::sleep((rtps_inst->heartbeat_period_ * 2.0).value());


  // 3. cleanup

  sdw.disassociate(subscription.remote_id_);

  TheServiceParticipant->shutdown();
  ACE_Thread_Manager::instance()->wait();

  return 0;
}


int
ACE_TMAIN(int argc, ACE_TCHAR* argv[])
{
  try {
    ::DDS::DomainParticipantFactory_var dpf =
      TheParticipantFactoryWithArgs(argc, argv);

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

    return DDS_TEST::test(host, port);

  } catch (const CORBA::BAD_PARAM& ) {
    std::cerr << "ERROR: caught CORBA::BAD_PARAM exception\n";
    return 1;
  } catch (const OpenDDS::DCPS::Transport::NotConfigured& ) {
    std::cerr << "ERROR: caught OpenDDS::DCPS::Transport::NotConfigured exception\n";
    return 1;
  }
}
