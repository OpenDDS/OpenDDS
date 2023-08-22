/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <dds/DCPS/RTPS/AssociationRecord.h>
#include <dds/DCPS/RTPS/ICE/Ice.h>

using namespace OpenDDS::DCPS;
using namespace OpenDDS::RTPS;

class MockTransportClient : public OpenDDS::DCPS::TransportClient {
public:
  MockTransportClient()
  {
    GUID_t local_id;
    GuidBuilder builder(local_id);
    builder.guidPrefix0(0);
    builder.guidPrefix1(1);
    builder.guidPrefix2(2);
    builder.entityId(3);
    builder.entityKey(4);
    TransportClient::set_guid(local_id);
  }

  virtual ~MockTransportClient()
  {}

  virtual bool check_transport_qos(const TransportInst&)
  {
    return false;
  }

  virtual DDS::DomainId_t domain_id() const
  {
    return 0;
  }

  virtual Priority get_priority_value(const AssociationData&) const
  {
    return 0;
  }
};

TEST(dds_DCPS_RTPS_AssociationRecord, BuiltinAssociationRecord_ctor)
{
  // Initialize the Service Participant so the TransportClient ctor doesn't block.
  DDS::DomainParticipantFactory_var dpf = TheServiceParticipant->get_domain_participant_factory();
  TransportClient_rch transport_client = make_rch<MockTransportClient>();

  GUID_t remote_id = GUID_UNKNOWN;
  {
    GuidBuilder builder(remote_id);
    builder.guidPrefix0(10);
    builder.guidPrefix1(11);
    builder.guidPrefix2(12);
    builder.entityId(13);
    builder.entityKey(14);
  }

  BuiltinAssociationRecord uut(transport_client, remote_id, 0);

  EXPECT_EQ(uut.local_id(), transport_client->get_guid());
  EXPECT_EQ(uut.remote_id(), remote_id);
  EXPECT_FALSE(uut.remote_reliable());
  EXPECT_FALSE(uut.remote_durable());
  EXPECT_FALSE(uut.generate_remote_matched_crypto_handle());
  EXPECT_FALSE(uut.send_local_token());
  EXPECT_FALSE(uut.local_tokens_sent());
  EXPECT_EQ(uut.transport_client_, transport_client);

  TheServiceParticipant->shutdown();
}

TEST(dds_DCPS_RTPS_AssociationRecord, BuiltinAssociationRecord_ctor_with_flags)
{
  // Initialize the Service Participant so the TransportClient ctor doesn't block.
  DDS::DomainParticipantFactory_var dpf = TheServiceParticipant->get_domain_participant_factory();
  TransportClient_rch transport_client = make_rch<MockTransportClient>();

  GUID_t remote_id = GUID_UNKNOWN;
  {
    GuidBuilder builder(remote_id);
    builder.guidPrefix0(10);
    builder.guidPrefix1(11);
    builder.guidPrefix2(12);
    builder.entityId(13);
    builder.entityKey(14);
  }

  BuiltinAssociationRecord uut(transport_client, remote_id, AC_REMOTE_RELIABLE | AC_REMOTE_DURABLE | AC_GENERATE_REMOTE_MATCHED_CRYPTO_HANDLE | AC_SEND_LOCAL_TOKEN);

  EXPECT_TRUE(uut.remote_reliable());
  EXPECT_TRUE(uut.remote_durable());
  EXPECT_TRUE(uut.generate_remote_matched_crypto_handle());
  EXPECT_TRUE(uut.send_local_token());

  TheServiceParticipant->shutdown();
}

TEST(dds_DCPS_RTPS_AssociationRecord, BuiltinAssociationRecord_local_tokens_sent)
{
  // Initialize the Service Participant so the TransportClient ctor doesn't block.
  DDS::DomainParticipantFactory_var dpf = TheServiceParticipant->get_domain_participant_factory();
  TransportClient_rch transport_client = make_rch<MockTransportClient>();

  GUID_t remote_id = GUID_UNKNOWN;
  {
    GuidBuilder builder(remote_id);
    builder.guidPrefix0(10);
    builder.guidPrefix1(11);
    builder.guidPrefix2(12);
    builder.entityId(13);
    builder.entityKey(14);
  }

  BuiltinAssociationRecord uut(transport_client, remote_id, 0);

  uut.local_tokens_sent(true);
  EXPECT_TRUE(uut.local_tokens_sent());
  uut.local_tokens_sent(false);
  EXPECT_FALSE(uut.local_tokens_sent());

  TheServiceParticipant->shutdown();
}

class MockDataWriterCallbacks : public DataWriterCallbacks {
public:

  virtual void set_publication_id(const GUID_t&)
  {}

  virtual void add_association(const ReaderAssociation&,
                               bool)
  {}

  virtual void remove_associations(const ReaderIdSeq&,
                                   CORBA::Boolean)
  {}

  virtual void update_incompatible_qos(const IncompatibleQosStatus&)
  {}

  virtual void update_subscription_params(const GUID_t&,
                                          const DDS::StringSeq&)
  {}

  virtual WeakRcHandle<OpenDDS::ICE::Endpoint> get_ice_endpoint()
  {
    return WeakRcHandle<OpenDDS::ICE::Endpoint>();
  }
};

TEST(dds_DCPS_RTPS_AssociationRecord, WriterAssociationRecord_ctor)
{
  RcHandle<DataWriterCallbacks> callbacks = make_rch<MockDataWriterCallbacks>();
  GUID_t writer_id = GUID_UNKNOWN;
  {
    GuidBuilder builder(writer_id);
    builder.guidPrefix0(0);
    builder.guidPrefix1(1);
    builder.guidPrefix2(2);
    builder.entityId(3);
    builder.entityKey(4);
  }
  ReaderAssociation reader_association;
  {
    GuidBuilder builder(reader_association.readerId);
    builder.guidPrefix0(10);
    builder.guidPrefix1(11);
    builder.guidPrefix2(12);
    builder.entityId(13);
    builder.entityKey(14);
  }

  WriterAssociationRecord uut(callbacks, writer_id, reader_association);

  EXPECT_EQ(uut.writer_id(), writer_id);
  EXPECT_EQ(uut.reader_id(), reader_association.readerId);
  EXPECT_EQ(uut.callbacks_, callbacks);
  EXPECT_EQ(uut.writer_id_, writer_id);
  // IDL defined type.
  //EXPECT_EQ(uut.reader_association_, reader_association);
}

class MockDataReaderCallbacks : public DataReaderCallbacks {
public:
  virtual void set_subscription_id(const GUID_t&)
  {}

  virtual void add_association(const WriterAssociation&,
                               bool)
  {}

  virtual void remove_associations(const WriterIdSeq&,
                                   CORBA::Boolean)
  {}

  virtual void update_incompatible_qos(const IncompatibleQosStatus&)
  {}

  virtual void signal_liveliness(const GUID_t&)
  {}

  virtual WeakRcHandle<OpenDDS::ICE::Endpoint> get_ice_endpoint()
  {
    return WeakRcHandle<OpenDDS::ICE::Endpoint>();
  }
};

TEST(dds_DCPS_RTPS_AssociationRecord, ReaderAssociationRecord_ctor)
{
  RcHandle<DataReaderCallbacks> callbacks = make_rch<MockDataReaderCallbacks>();
  GUID_t reader_id = GUID_UNKNOWN;
  {
    GuidBuilder builder(reader_id);
    builder.guidPrefix0(0);
    builder.guidPrefix1(1);
    builder.guidPrefix2(2);
    builder.entityId(3);
    builder.entityKey(4);
  }
  WriterAssociation writer_association;
  {
    GuidBuilder builder(writer_association.writerId);
    builder.guidPrefix0(10);
    builder.guidPrefix1(11);
    builder.guidPrefix2(12);
    builder.entityId(13);
    builder.entityKey(14);
  }

  ReaderAssociationRecord uut(callbacks, reader_id, writer_association);

  EXPECT_EQ(uut.reader_id(), reader_id);
  EXPECT_EQ(uut.writer_id(), writer_association.writerId);
  EXPECT_EQ(uut.callbacks_, callbacks);
  EXPECT_EQ(uut.reader_id_, reader_id);
  // IDL defined type.
  //EXPECT_EQ(uut.writer_association_, writer_association);
}
