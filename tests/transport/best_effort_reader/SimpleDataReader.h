#ifndef test_transport_SimpleDataReader_H
#define test_transport_SimpleDataReader_H

#include "AppConfig.h"
#include "TestMsg.h"

#include <dds/DCPS/transport/framework/TransportReceiveListener.h>
#include <dds/DCPS/transport/framework/TransportClient.h>
#include <dds/DCPS/Serializer.h>

#include <ace/String_Base.h>
#include <map>

class SimpleDataReader :
  public OpenDDS::DCPS::TransportReceiveListener,
  public OpenDDS::DCPS::TransportClient
{
public:
  SimpleDataReader(
    const AppConfig& ac, const int readerIndex,
    OpenDDS::DCPS::AssociationData& ad);
  virtual ~SimpleDataReader();
  bool done() { return done_; }

  // Implementing TransportReceiveListener
  void data_received(const OpenDDS::DCPS::ReceivedDataSample& sample);
  void notify_subscription_disconnected(const OpenDDS::DCPS::WriterIdSeq&) {}
  void notify_subscription_reconnected(const OpenDDS::DCPS::WriterIdSeq&) {}
  void notify_subscription_lost(const OpenDDS::DCPS::WriterIdSeq&) {}
  void remove_associations(const OpenDDS::DCPS::WriterIdSeq&, bool) {}
  void _add_ref() {}
  void _remove_ref() {}

  // Implementing TransportClient
  OpenDDS::DCPS::RepoId get_repo_id() const { return AppConfig::readerId[index]; }
  bool check_transport_qos(const OpenDDS::DCPS::TransportInst&) { return true; }
  DDS::DomainId_t domain_id() const { return 0; }
  CORBA::Long get_priority_value(const OpenDDS::DCPS::AssociationData&) const { return 0; }

private:
  typedef std::map<
    OpenDDS::DCPS::RepoId, ACE_INT64, OpenDDS::DCPS::GUID_tKeyLessThan>
    RepoIdSeqNMap;
  bool deserializeEncapsulationHeader(OpenDDS::DCPS::Serializer& s);
  bool deserializeData(TestMsg& data, OpenDDS::DCPS::Serializer& s);
  bool deserializeKey(TestMsg& data, OpenDDS::DCPS::Serializer& s);

  const AppConfig& config;
  const int index;
  bool done_;
  RepoIdSeqNMap id_seqN_;
};

#endif /* test_transport_SimpleDataReader_H */
