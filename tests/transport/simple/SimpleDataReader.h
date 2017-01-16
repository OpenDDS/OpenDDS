// -*- C++ -*-
//
#ifndef SIMPLEDATAREADER_H
#define SIMPLEDATAREADER_H

#include "dds/DCPS/transport/framework/TransportReceiveListener.h"
#include "dds/DCPS/transport/framework/TransportClient.h"
#include "dds/DCPS/Definitions.h"


class SimpleDataReader
  : public OpenDDS::DCPS::TransportReceiveListener
  , public OpenDDS::DCPS::TransportClient
{
  public:

    explicit SimpleDataReader(const OpenDDS::DCPS::RepoId& sub_id);
    virtual ~SimpleDataReader();

    void init(const OpenDDS::DCPS::AssociationData& publication, int num_msgs);

    // Implementing TransportReceiveListener
    void data_received(const OpenDDS::DCPS::ReceivedDataSample& sample);
    void notify_subscription_disconnected(const OpenDDS::DCPS::WriterIdSeq&) {}
    void notify_subscription_reconnected(const OpenDDS::DCPS::WriterIdSeq&) {}
    void notify_subscription_lost(const OpenDDS::DCPS::WriterIdSeq&) {}
    void notify_connection_deleted(const OpenDDS::DCPS::RepoId&) {}
    void remove_associations(const OpenDDS::DCPS::WriterIdSeq&, bool) {}
    void _add_ref() {}
    void _remove_ref() {}

    // Implementing TransportClient
    bool check_transport_qos(const OpenDDS::DCPS::TransportInst&)
      { return true; }
    const OpenDDS::DCPS::RepoId& get_repo_id() const
      { return sub_id_; }
    DDS::DomainId_t domain_id() const
      { return 0; }
    CORBA::Long get_priority_value(const OpenDDS::DCPS::AssociationData&) const
      { return 0; }

    void transport_lost();

    /// Returns 0 if the data_received() has not been called/completed.
    /// Returns 1 if the data_received() has been called, and all of
    /// the TransportReceiveListeners have been told of the data_received().
    int received_test_message() const;

    void print_time();

    using OpenDDS::DCPS::TransportClient::enable_transport;
    using OpenDDS::DCPS::TransportClient::disassociate;

  private:

    const OpenDDS::DCPS::RepoId& sub_id_;
    int num_messages_expected_;
    int num_messages_received_;
    ACE_Time_Value begin_recvd_;
    ACE_Time_Value finished_recvd_;
};

#endif  /* SIMPLEDATAREADER_H */
