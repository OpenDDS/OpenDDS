// -*- C++ -*-
//
// $Id$
#include  "SimplePublisher.h"
#include  "dds/DCPS/transport/framework/TransportImpl.h"
#include  "dds/DCPS/transport/framework/TheTransportFactory.h"
#include  "TestException.h"

#include "dds/DCPS/transport/framework/EntryExit.h"


SimplePublisher::SimplePublisher()
{
  DBG_ENTRY("SimplePublisher","SimplePublisher");
}


SimplePublisher::~SimplePublisher()
{
  DBG_ENTRY("SimplePublisher","~SimplePublisher");
}


void
SimplePublisher::init(TAO::DCPS::TransportFactory::IdType transport_id,
                      TAO::DCPS::RepoId                   pub_id,
                      ssize_t                             num_subscriptions,
                      const TAO::DCPS::AssociationData*   subscriptions)
{
  DBG_ENTRY("SimplePublisher","init");

  VDBG((LM_DEBUG, "(%P|%t) DBG:   "
             "Use TheTransportFactory to obtain() the TransportImpl object "
             "that it knows as transport_id (%d).\n", transport_id));

  // Obtain the transport.
  TAO::DCPS::TransportImpl_rch transport =
                                    TheTransportFactory->obtain(transport_id);

  if (transport.is_nil())
    {
      // Failed to obtain the transport.
      ACE_ERROR((LM_ERROR,
                 "(%P|%t) Failed to obtain TransportImpl (id %d) from "
                 "TheTransportFactory.\n", transport_id));
      throw TestException();
    }

  VDBG((LM_DEBUG, "(%P|%t) DBG:   "
             "Attach ourselves (SimplePublisher) to the TransportImpl.\n"));

  // Attempt to attach the transport to ourselves.
  TAO::DCPS::AttachStatus status = this->attach_transport(transport.in());

  if (status != TAO::DCPS::ATTACH_OK)
    {
      // We failed to attach to the transport for some reason.
      std::string status_str;

      switch (status)
        {
          case TAO::DCPS::ATTACH_BAD_TRANSPORT:
            status_str = "ATTACH_BAD_TRANSPORT";
            break;
          case TAO::DCPS::ATTACH_ERROR:
            status_str = "ATTACH_ERROR";
            break;
          case TAO::DCPS::ATTACH_INCOMPATIBLE_QOS:
            status_str = "ATTACH_INCOMPATIBLE_QOS";
            break;
          default:
            status_str = "Unknown Status";
            break;
        }

      ACE_ERROR((LM_ERROR,
                 "(%P|%t) Failed to attach to the transport. "
                 "AttachStatus == %s\n", status_str.c_str()));

      throw TestException();
    }

  // Good!  We are now attached to the transport.
  VDBG((LM_DEBUG, "(%P|%t) DBG:   "
             "SimplePublisher is now attached to the TransportImpl.\n"));

  VDBG((LM_DEBUG, "(%P|%t) DBG:   "
             "Initialize our SimpleDataWriter object.\n"));

  // Initialize our DataWriter.
  this->writer_.init(pub_id);

  VDBG((LM_DEBUG, "(%P|%t) DBG:   "
             "Add the subscriptions.\n"));

  // Add the association between the local pub_id and the remote sub_id
  // to the transport via the TransportInterface (our base class).
  int result = this->add_subscriptions(pub_id,
                                       0,                  /* priority */
                                       num_subscriptions,
                                       subscriptions);

  if (result != 0)
    {
      ACE_ERROR((LM_ERROR,
                 "(%P|%t) Failed to add subscriptions to the "
                 "TransportInterface.\n"));
      throw TestException();
    }

  VDBG((LM_DEBUG, "(%P|%t) DBG:   "
             "The subscriptions have been added successfully.\n"));
}


int
SimplePublisher::run()
{
  DBG_ENTRY("SimplePublisher","run");

  return this->writer_.run(this);
}


void
SimplePublisher::send_samples(const TAO::DCPS::DataSampleList& samples)
{
  DBG_ENTRY("SimplePublisher","send_samples");

  this->send(samples);
}


void
SimplePublisher::transport_detached_i()
{
  DBG_ENTRY("SimplePublisher","transport_detached_i");

  ACE_DEBUG((LM_DEBUG,
             "(%P|%t) Transport has detached from SimplePublisher.\n"));

  this->writer_.transport_lost();
}


int
SimplePublisher::delivered_test_message()
{
  return this->writer_.delivered_test_message();
}
