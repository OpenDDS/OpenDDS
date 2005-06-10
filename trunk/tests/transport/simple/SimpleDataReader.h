// -*- C++ -*-
//
// $Id$
#ifndef SIMPLEDATAREADER_H
#define SIMPLEDATAREADER_H

#include  "dds/DCPS/transport/framework/TransportReceiveListener.h"
#include  "dds/DCPS/Definitions.h"


class SimpleDataReader : public TAO::DCPS::TransportReceiveListener
{
  public:

    SimpleDataReader();
    virtual ~SimpleDataReader();

    void init(TAO::DCPS::RepoId sub_id);

    virtual void data_received(const TAO::DCPS::ReceivedDataSample& sample);

    void transport_lost();

    /// Returns 0 if the data_received() has not been called/completed.
    /// Returns 1 if the data_received() has been called, and all of
    /// the TransportReceiveListeners have been told of the data_received().
    int received_test_message() const;


  private:

    TAO::DCPS::RepoId sub_id_;
    int received_test_message_;
};

#endif  /* SIMPLEDATAREADER_H */
