#ifndef test_transport_SimpleDataReader_H
#define test_transport_SimpleDataReader_H

#include "AppConfig.h"
#include "TestMsg.h"

#include "dds/DCPS/transport/framework/TransportReceiveListener.h"
#include "dds/DCPS/transport/framework/TransportClient.h"
#include "dds/DCPS/Serializer.h"

#include <ace/String_Base.h>
#include <map>

class SimpleDataReader : public TransportReceiveListener, public TransportClient
{
public:
  SimpleDataReader(const AppConfig& ac, const int readerIndex, AssociationData& ad);
  virtual ~SimpleDataReader();
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
  const RepoId& get_repo_id() const { return AppConfig::readerId[index]; }
  bool check_transport_qos(const TransportInst&) { return true; }
  DDS::DomainId_t domain_id() const { return 0; }
  CORBA::Long get_priority_value(const AssociationData&) const { return 0; }

private:
  typedef std::map<RepoId, ACE_INT64, GUID_tKeyLessThan> RepoIdSeqNMap;
  bool deserializeEncapsulationHeader(Serializer& s);
  bool deserializeData(TestMsg& data, Serializer& s);
  bool deserializeKey(TestMsg& data, Serializer& s);

  const AppConfig& config;
  const int index;
  bool done_;
  RepoIdSeqNMap id_seqN_;
};

#endif /* test_transport_SimpleDataReader_H */
