#include "TestMsg.h"
#include "Domain.h"

#include "dds/DCPS/transport/framework/TransportSendListener.h"
#include "dds/DCPS/transport/framework/TransportClient.h"
#include "dds/DCPS/transport/framework/TransportExceptions.h"
#include "dds/DCPS/transport/rtps_udp/RtpsUdpDataLink.h"

#include "dds/DCPS/RTPS/RtpsCoreTypeSupportImpl.h"
#include "dds/DCPS/RTPS/BaseMessageTypes.h"
#include "dds/DCPS/RTPS/BaseMessageUtils.h"

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

class SimpleDataWriter : public TransportSendListener, public TransportClient
{
public:
  explicit SimpleDataWriter(const Domain& d, const LocatorSeq& locators)
    : domain(d), callbacks_expected_(0)
  {
    enable_transport(true, false); // (reliable, durable)

    AssociationData subscription;
    subscription.remote_reliable_ = false;
    subscription.remote_data_.length(1);
    subscription.remote_data_[0].transport_type = "rtps_udp";
    setLocators(subscription.remote_data_[0].data, locators);

    subscription.remote_id_ = domain.getSubRdrId(0);
    if (!associate(subscription, true)) {
      throw std::string("publisher failed to associate reader 1");
    }
    subscription.remote_id_ = domain.getSubRdrId(1);
    if (!associate(subscription, true)) {
      throw std::string("publisher failed to associate reader 2");
    }
  }

  virtual ~SimpleDataWriter() {
    disassociate(domain.getSubRdrId(0));
    disassociate(domain.getSubRdrId(1));
  }

  void callbacksExpected(ssize_t n) { callbacks_expected_ = n; }
  ssize_t callbacksExpected() const { return callbacks_expected_; }

  // Implementing TransportSendListener
  void data_delivered(const DataSampleElement*) {
    ACE_DEBUG((LM_INFO, "(%P|%t) SimpleDataWriter::data_delivered()\n"));
    --callbacks_expected_;
  }

  void data_dropped(const DataSampleElement*, bool by_transport) {
    ACE_DEBUG((LM_INFO, "(%P|%t) SimpleDataWriter::data_dropped(element, %d)\n",
      int(by_transport)));
    --callbacks_expected_;
  }

  void control_delivered(const Message_Block_Ptr&) { //sample
    ACE_DEBUG((LM_INFO, "(%P|%t) SimpleDataWriter::control_delivered()\n"));
  }

  void control_dropped(const Message_Block_Ptr&, bool) { //(sample, dropped_by_transport)
    ACE_DEBUG((LM_INFO, "(%P|%t) SimpleDataWriter::control_dropped()\n"));
  }

  void notify_publication_disconnected(const ReaderIdSeq&) {}
  void notify_publication_reconnected(const ReaderIdSeq&) {}
  void notify_publication_lost(const ReaderIdSeq&) {}
  void remove_associations(const ReaderIdSeq&, bool) {}
  void _add_ref() {}
  void _remove_ref() {}

  // Implementing TransportClient
  bool check_transport_qos(const TransportInst&) { return true; }
  const RepoId& get_repo_id() const { return domain.getPubWtrId(); }
  DDS::DomainId_t domain_id() const { return 0; }
  CORBA::Long get_priority_value(const AssociationData&) const { return 0; }

private:
  void setLocators(OpenDDS::DCPS::TransportBLOB& data, const LocatorSeq& locators) {
    size_t size = 0, padding = 0;
    gen_find_size(locators, size, padding);
    ACE_Message_Block mb(size + padding + 1);
    Serializer ser(&mb, ACE_CDR_BYTE_ORDER, Serializer::ALIGN_CDR);
    ser << locators;
    ser << ACE_OutputCDR::from_boolean(false);
    OpenDDS::RTPS::message_block_to_sequence(mb, data);
  }

  const Domain& domain;
  ssize_t callbacks_expected_;
};

class DDS_TEST {
public:
  DDS_TEST(int argc, ACE_TCHAR* argv[]) : domain(argc, argv), msgSeqN(0) {
    domain.setHeartbeatPeriod(100);

    ACE_INET_Addr remoteAddr = domain.getHostAddress();
    locators.length(1);
    locators[0].kind = OpenDDS::RTPS::address_to_kind(remoteAddr);
    locators[0].port = remoteAddr.get_port_number();
    OpenDDS::RTPS::address_to_bytes(locators[0].address, remoteAddr);
  }

  int run();

private:
  static const bool hostIsBigEndian = !ACE_CDR_BYTE_ORDER;
  static const ACE_CDR::ULong encap = 0x00000100; // {CDR_LE, options} in BE format
  static const CORBA::Octet DE  = OpenDDS::RTPS::FLAG_D | OpenDDS::RTPS::FLAG_E;
  static const CORBA::Octet DEQ = OpenDDS::RTPS::FLAG_D | OpenDDS::RTPS::FLAG_E | OpenDDS::RTPS::FLAG_Q;

  void force_inline_qos(bool val) {
    OpenDDS::DCPS::RtpsUdpDataLink::force_inline_qos_ = val;
  }
  void log_time(const ACE_Time_Value& t) const {
    ACE_TCHAR buffer[32];
    const std::time_t seconds = t.sec();
    std::string ts(ACE_TEXT_ALWAYS_CHAR(ACE_OS::ctime_r(&seconds, buffer, 32)));
    ts.erase(ts.size() - 1); // remove \n from ctime()
    ACE_DEBUG((LM_INFO, "Sending with timestamp %C %q usec\n", ts.c_str(), ACE_INT64(t.usec())));
  }

  bool writeToSocket(const TestMsg& msg, const CORBA::Octet flags = DE) const;
  void serializeData(DataSampleElement& ds, const TestMsg& msg) const;

  void listData(SendStateDataSampleList& list, DataSampleElement ds[], const ssize_t size) {
    list.head_ = ds;
    list.size_ = size;
    list.tail_ = &ds[size - 1];
    for (int i = 0; i < size - 1; ++i) {
      ds[i].set_next_send_sample(&ds[i + 1]);
    }
  }

  Domain domain;
  LocatorSeq locators;
  mutable CORBA::ULong msgSeqN;
};

using namespace OpenDDS::RTPS;

bool DDS_TEST::writeToSocket(const TestMsg& msg, const CORBA::Octet flags) const
{
  const OpenDDS::RTPS::GuidPrefix_t& local_prefix = domain.getPubWtrId().guidPrefix;
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

  DataSubmessage ds = { {DATA, flags, 20 + (flags==DEQ?24:0) + 12 + ACE_OS::strlen(msg.value)+1},
    0, 16, ENTITYID_UNKNOWN, domain.getPubWtrId().entityId, {0, ++msgSeqN}, ParameterList() };
  if(flags == DEQ) {
    ds.inlineQos.length(1);
    ds.inlineQos[0].guid(domain.getSubRdrId(1));
    ds.inlineQos[0]._d(OpenDDS::RTPS::PID_DIRECTED_WRITE);
  }

  size_t size = 0, padding = 0;
  gen_find_size(hdr, size, padding);
  gen_find_size(it, size, padding);
  gen_find_size(ds, size, padding);
  find_size_ulong(size, padding);
  gen_find_size(msg, size, padding);

  ACE_Message_Block mb(size + padding);
  Serializer ser(&mb, hostIsBigEndian, Serializer::ALIGN_CDR);
  bool ok = (ser << hdr) && (ser << it) && (ser << ds) && (ser << encap) && (ser << msg);
  if (!ok) {
    ACE_DEBUG((LM_INFO, "ERROR: failed to serialize RTPS message\n"));
    return false;
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
  ssize_t res = sock.send(mb.rd_ptr(), mb.length(), dest);
  if (res < 0) {
    ACE_DEBUG((LM_INFO, "ERROR: in sock.send()\n"));
    return false;
  } else {
    std::ostringstream oss;
    oss << "Sent " << res << " bytes.\n";
    ACE_DEBUG((LM_INFO, oss.str().c_str()));
  }

  return true;
}

void DDS_TEST::serializeData(DataSampleElement& ds, const TestMsg& msg) const
{
  DataSampleHeader& dsh = ds.header_;
  dsh.message_id_ = SAMPLE_DATA;
  dsh.publication_id_ = domain.getPubWtrId();
  dsh.sequence_ = ++msgSeqN;
  const ACE_Time_Value tv = ACE_OS::gettimeofday();
  log_time(tv);
  DDS::Time_t st = time_value_to_time(tv);
  dsh.source_timestamp_sec_ = st.sec;
  dsh.source_timestamp_nanosec_ = st.nanosec;
  dsh.key_fields_only_ = false;

  // Calculate the data buffer length
  size_t size = 0, padding = 0;
  find_size_ulong(size, padding);   // encap
  gen_find_size(msg, size, padding);
  dsh.message_length_ = static_cast<ACE_UINT32>(size + padding);

  ds.sample_.reset(new ACE_Message_Block(DataSampleHeader::max_marshaled_size(),
    ACE_Message_Block::MB_DATA, new ACE_Message_Block(dsh.message_length_)));
  *ds.sample_ << dsh;

  Serializer ser(ds.sample_->cont(), hostIsBigEndian, Serializer::ALIGN_CDR);
  if (!((ser << encap) && (ser << msg))) {
    throw std::string("ERROR: failed to serialize element " + std::to_string(msgSeqN));
  }
}

int DDS_TEST::run()
{
  SimpleDataWriter sdw(domain, locators);

  if(!writeToSocket(TestMsg(10, "TestMsg"))) {
    ACE_DEBUG((LM_INFO, "ERROR: failed to write to socket\n"));
  }
  if(!writeToSocket(TestMsg(10, "Directed Write"), DEQ)) {
    ACE_DEBUG((LM_INFO, "ERROR: failed to write to socket\n"));
  }

  // send sample data through the OpenDDS transport
  DataSampleElement elements[] = {
    DataSampleElement(domain.getPubWtrId(), &sdw, OpenDDS::DCPS::PublicationInstance_rch()),
    DataSampleElement(domain.getPubWtrId(), &sdw, OpenDDS::DCPS::PublicationInstance_rch()),
  };

  serializeData(elements[0], TestMsg(10, "Through OpenDDS Transport"));
  serializeData(elements[1], TestMsg(99, "key=99 means end"));

  SendStateDataSampleList list;
  listData(list, elements, sizeof(elements) / sizeof(elements[0]));
  sdw.callbacksExpected(list.size());
  sdw.send(list);

  while (sdw.callbacksExpected()) {
    ACE_OS::sleep(1);
  }

  // Allow enough time for a HEARTBEAT message to be generated
  ACE_OS::sleep((domain.getHeartbeatPeriod() * 2.0).value());

  return 0;
}

int ACE_TMAIN(int argc, ACE_TCHAR* argv[])
{
  try {
    DDS_TEST test(argc, argv);
    return test.run();
  } catch (...) {
    return 1;
  }
}
