// -*- C++ -*-
//
// $Id$
#ifndef SIMPLESUBSCRIBER_H
#define SIMPLESUBSCRIBER_H

#include  "SimpleDataReader.h"
#include  "dds/DCPS/Definitions.h"
#include  "dds/DCPS/AssociationData.h"
#include  "dds/DCPS/transport/framework/TransportInterface.h"
#include  "dds/DCPS/transport/framework/TheTransportFactory.h"


class SimpleSubscriber : public OpenDDS::DCPS::TransportInterface
{
  public:

    SimpleSubscriber();
    virtual ~SimpleSubscriber();

    void init(OpenDDS::DCPS::TransportIdType          transport_id,
              OpenDDS::DCPS::RepoId                   sub_id,
              ssize_t                             num_publications,
              const OpenDDS::DCPS::AssociationData*   publications,
              int                                 receive_delay_msec);

    /// Returns 0 if the data_received() has not been called/completed.
    /// Returns 1 if the data_received() has been called, and all of
    /// the TransportReceiveListeners have been told of the data_received().
    int received_test_message() const;


  protected:

    virtual void cleanup();


  private:

    SimpleDataReader reader_;
};

#endif  /* SIMPLESUBSCRIBER_H */
