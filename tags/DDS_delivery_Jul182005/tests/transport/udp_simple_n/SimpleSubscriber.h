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


class SimpleSubscriber : public TAO::DCPS::TransportInterface
{
  public:

    SimpleSubscriber();
    virtual ~SimpleSubscriber();

    void init(TAO::DCPS::TransportFactory::IdType transport_id,
              TAO::DCPS::RepoId                   sub_id,
              ssize_t                             num_publications,
              const TAO::DCPS::AssociationData*   publications,
              unsigned                            num_msgs);

    /// Returns 0 if the data_received() has not been called/completed.
    /// Returns 1 if the data_received() has been called, and all of
    /// the TransportReceiveListeners have been told of the data_received().
    int received_test_message() const;


  protected:

    /// Only called if the TransportImpl is shutdown() when this
    /// TransportInterface object is still attached to the TransportImpl.
    virtual void transport_detached_i();


  private:

    SimpleDataReader reader_;
};

#endif  /* SIMPLESUBSCRIBER_H */
