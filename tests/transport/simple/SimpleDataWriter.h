// -*- C++ -*-
//
// $Id$
#ifndef SIMPLEDATAWRITER_H
#define SIMPLEDATAWRITER_H

#include "dds/DCPS/transport/framework/TransportSendListener.h"
#include "dds/DCPS/transport/framework/TransportClient.h"
#include "dds/DCPS/Definitions.h"

class SimplePublisher;


class SimpleDataWriter
  : public OpenDDS::DCPS::TransportSendListener
  , public OpenDDS::DCPS::TransportClient
{
  public:

    explicit SimpleDataWriter(const OpenDDS::DCPS::RepoId& pub_id);
    virtual ~SimpleDataWriter();

    void init(const OpenDDS::DCPS::AssociationData& subscription);
    int run();

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
    void data_delivered(const OpenDDS::DCPS::DataSampleListElement* sample);
    void data_dropped(const OpenDDS::DCPS::DataSampleListElement* sample,
                      bool dropped_by_transport = false);
    void notify_publication_disconnected(const OpenDDS::DCPS::ReaderIdSeq&) {}
    void notify_publication_reconnected(const OpenDDS::DCPS::ReaderIdSeq&) {}
    void notify_publication_lost(const OpenDDS::DCPS::ReaderIdSeq&) {}
    void notify_connection_deleted() {}
    void remove_associations(const OpenDDS::DCPS::ReaderIdSeq&, bool) {}

    // Implementing TransportClient
    bool check_transport_qos(const OpenDDS::DCPS::TransportInst&)
      { return true; }
    const OpenDDS::DCPS::RepoId& get_repo_id() const
      { return pub_id_; }
    CORBA::Long get_priority_value(const OpenDDS::DCPS::AssociationData&) const
      { return 0; }

    int delivered_test_message();

    using OpenDDS::DCPS::TransportClient::enable_transport;
    using OpenDDS::DCPS::TransportClient::disassociate;

  private:

    const OpenDDS::DCPS::RepoId& pub_id_;
    int delivered_test_message_;
};

#endif  /* SIMPLEDATAWRITER_H */
