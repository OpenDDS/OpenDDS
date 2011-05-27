// -*- C++ -*-
//
// $Id$
#ifndef SIMPLEPUBLISHER_H
#define SIMPLEPUBLISHER_H

#include  "SimpleDataWriter.h"
#include  "dds/DCPS/transport/framework/TransportInterface.h"
#include  "dds/DCPS/transport/framework/TheTransportFactory.h"
#include  "dds/DCPS/Definitions.h"


class SimplePublisher : public TAO::DCPS::TransportInterface
{
  public:

    SimplePublisher();
    virtual ~SimplePublisher();

    void init(TAO::DCPS::TransportIdType               transport_id,
              TAO::DCPS::RepoId                   pub_id,
              ssize_t                             num_subscriptions,
              const TAO::DCPS::AssociationData*   subscriptions);

    int run(unsigned num_messages, unsigned size);

    void send_samples(const TAO::DCPS::DataSampleList& samples);

    /// Returns 0 if the data_delivered() has not been called by the
    /// transport framework.  Returns 1 if the data_delivered() has been
    /// called.
    int delivered_test_message();


  protected:

    /// Only called if the TransportImpl is shutdown() when this
    /// TransportInterface object is still attached to the TransportImpl.
    virtual void transport_detached_i();


  private:

    SimpleDataWriter writer_;
};

#endif  /* SIMPLEPUBLISHER_H */
