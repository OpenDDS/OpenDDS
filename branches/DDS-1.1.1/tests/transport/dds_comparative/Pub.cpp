// -*- C++ -*-
//
// $Id$
#include "Pub.h"
#include "dds/DCPS/transport/framework/TransportImpl.h"
#include "dds/DCPS/transport/framework/TheTransportFactory.h"
#include "TestException.h"


Pub::Pub()
  : pub_id_(0)
{
}


Pub::~Pub()
{
}


void
Pub::set_num_to_send(unsigned num_to_send)
{
  this->writer_.set_num_to_send(num_to_send);
}


void
Pub::set_data_size(char data_size)
{
  this->writer_.set_data_size(data_size);
}


void
Pub::set_local_publisher(OpenDDS::DCPS::RepoId pub_id)
{
  this->pub_id_ = pub_id;
  this->writer_.set_id(pub_id);
}


void
Pub::add_remote_subscriber(OpenDDS::DCPS::RepoId    sub_id,
                           const ACE_INET_Addr& sub_addr,
                           const std::string&   sub_addr_str)
{
  this->subs_.push_back(SubInfo(sub_id,sub_addr,sub_addr_str));
}


void
Pub::init(unsigned impl_id)
{
  // Attach ourselves to the transport
  this->init_attach_transport(impl_id);

  // Add our subscriptions through the TransportInterface.
  this->init_add_subscriptions();
}


void
Pub::run()
{
  this->writer_.run(this);
}


void
Pub::wait()
{
  while (1)
    {
      if (this->writer_.is_done())
        {
          return;
        }

      ACE_OS::sleep(1);
    }
}


void
Pub::send_samples(const OpenDDS::DCPS::DataSampleList& samples)
{
  this->send(samples);
}


void
Pub::transport_detached_i()
{
  ACE_DEBUG((LM_DEBUG, "(%P|%t) Transport has detached from Pub.\n"));
  this->writer_.transport_lost();
}


void
Pub::init_attach_transport(unsigned impl_id)
{
  // Obtain the transport.
  OpenDDS::DCPS::TransportImpl_rch transport =
                                      TheTransportFactory->obtain(impl_id);

  if (transport.is_nil())
    {
      // Failed to obtain the transport.
      ACE_ERROR((LM_ERROR,
                 "(%P|%t) Failed to obtain TransportImpl from "
                 "TheTransportFactory using impl_id (%d).\n",
                 impl_id));
      throw TestException();
    }

  // Attempt to attach the transport to ourselves.
  OpenDDS::DCPS::AttachStatus status = this->attach_transport(transport.in());

  if (status != OpenDDS::DCPS::ATTACH_OK)
    {
      // We failed to attach to the transport for some reason.
      const char* emsg = "Failed attachment to transport. AttachStatus == ";

      switch (status)
        {
          case OpenDDS::DCPS::ATTACH_BAD_TRANSPORT:
            ACE_ERROR((LM_ERROR,"(%P|%t) %s ATTACH_BAD_TRANSPORT\n",emsg));
            throw TestException();
          case OpenDDS::DCPS::ATTACH_ERROR:
            ACE_ERROR((LM_ERROR,"(%P|%t) %s ATTACH_ERROR\n",emsg));
            throw TestException();
          case OpenDDS::DCPS::ATTACH_INCOMPATIBLE_QOS:
            ACE_ERROR((LM_ERROR,"(%P|%t) %s ATTACH_INCOMPATIBLE_QOS\n",emsg));
            throw TestException();
          default:
            ACE_ERROR((LM_ERROR,"(%P|%t) %s <!UNKNOWN!>\n",emsg));
            throw TestException();
        }
    }
}



void
Pub::init_add_subscriptions()
{
  unsigned num_subs = this->subs_.size();

  OpenDDS::DCPS::AssociationData* subs = new OpenDDS::DCPS::AssociationData[num_subs];

  for (unsigned i = 0; i < num_subs; i++)
    {
      // Ask the SubInfo object to populate the AssociationData object for us.
      this->subs_[i].as_association(subs[i]);
    }

  int result = this->add_subscriptions(this->pub_id_, 0, num_subs, subs);

  delete [] subs;

  if (result != 0)
    {
      ACE_ERROR((LM_ERROR,
                 "(%P|%t) Failed to add subscriptions to the "
                 "TransportInterface.\n"));
      throw TestException();
    }
}
