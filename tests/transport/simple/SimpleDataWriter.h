// -*- C++ -*-
//
#ifndef SIMPLEDATAWRITER_H
#define SIMPLEDATAWRITER_H

#include "dds/DCPS/transport/framework/TransportSendListener.h"
#include "dds/DCPS/transport/framework/TransportClient.h"
#include "dds/DCPS/DataSampleElement.h"
#include "dds/DCPS/Definitions.h"

class SimplePublisher;
class DataSampleElement;

class SimpleDataWriter
  : public OpenDDS::DCPS::TransportSendListener
  , public OpenDDS::DCPS::TransportClient
{
  public:

    explicit SimpleDataWriter(const OpenDDS::DCPS::RepoId& pub_id);
    virtual ~SimpleDataWriter();

    void init(const OpenDDS::DCPS::AssociationData& subscription);

    // Implement in derived DDS_TEST class since internals of SendStateDataSampleList
    // and DataSampleElement need to be accessed and DDS_TEST is a friend of
    // SendStateDataSampleList and DataSampleElement.
    virtual int run(int num_msgs, int msg_size) = 0;

    // This means that the TransportImpl has been shutdown, making the
    // transport_interface sent to the run() method no longer valid.
    // This method should effectively block until the run() is notified,
    // and promises to stop using the transport_interface object
    // anymore.  This would really only mean something if our run() is
    // is running in a thread other than the thread that told the
    // TransportImpl to shutdown.  For now, this will just report a log
    // message.
    void transport_lost();

    // Implementing TransportSendListener
    void data_delivered(const OpenDDS::DCPS::DataSampleElement* sample);
    void data_dropped(const OpenDDS::DCPS::DataSampleElement* sample,
                      bool dropped_by_transport = false);
    void notify_publication_disconnected(const OpenDDS::DCPS::ReaderIdSeq&) {}
    void notify_publication_reconnected(const OpenDDS::DCPS::ReaderIdSeq&) {}
    void notify_publication_lost(const OpenDDS::DCPS::ReaderIdSeq&) {}
    void notify_connection_deleted(const OpenDDS::DCPS::RepoId&) {}
    void remove_associations(const OpenDDS::DCPS::ReaderIdSeq&, bool) {}
    void _add_ref() {}
    void _remove_ref() {}

    // Implementing TransportClient
    bool check_transport_qos(const OpenDDS::DCPS::TransportInst&)
      { return true; }
    const OpenDDS::DCPS::RepoId& get_repo_id() const
      { return pub_id_; }
    DDS::DomainId_t domain_id() const
      { return 0; }
    CORBA::Long get_priority_value(const OpenDDS::DCPS::AssociationData&) const
      { return 0; }

    int delivered_test_message();

    using OpenDDS::DCPS::TransportClient::enable_transport;
    using OpenDDS::DCPS::TransportClient::disassociate;

  protected:

    const OpenDDS::DCPS::RepoId& pub_id_;
    int num_messages_sent_;
    int num_messages_delivered_;
};

class DDS_TEST : public SimpleDataWriter
{
public:
    explicit DDS_TEST(const OpenDDS::DCPS::RepoId& pub_id);
    virtual int run(int num_msgs, int msg_size);

    static void cleanup(OpenDDS::DCPS::DataSampleElementAllocator& alloc,
                        OpenDDS::DCPS::SendStateDataSampleList& list);

};

#endif  /* SIMPLEDATAWRITER_H */
