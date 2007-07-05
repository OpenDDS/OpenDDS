// -*- C++ -*-
//
// $Id$
#include "Sub.h"
#include "dds/DCPS/transport/framework/TransportImpl.h"
#include "dds/DCPS/transport/framework/TheTransportFactory.h"
#include "TestException.h"


Sub::Sub()
  : sub_id_(0)
{
}


Sub::~Sub()
{
}


void
Sub::set_num_to_receive(unsigned num_to_receive)
{
  this->reader_.set_num_to_receive(num_to_receive);
}


void
Sub::set_data_size(char data_size)
{
  this->reader_.set_data_size(data_size);
}


void
Sub::set_local_subscriber(OpenDDS::DCPS::RepoId sub_id)
{
  this->sub_id_ = sub_id;
  this->reader_.set_id(sub_id);
}


void
Sub::add_remote_publisher(OpenDDS::DCPS::RepoId    pub_id,
                          const ACE_INET_Addr& pub_addr)
{
  this->pubs_.push_back(PubInfo(pub_id,pub_addr));
}


void
Sub::init(unsigned impl_id)
{
  this->init_attach_transport(impl_id);
  this->init_add_publications();
}


void
Sub::wait()
{
  while (1)
    {
      if (this->reader_.is_done())
        {
          return;
        }

      ACE_OS::sleep(1);
    }
}


void
Sub::transport_detached_i()
{
  this->reader_.transport_lost();
}


void
Sub::init_attach_transport(unsigned impl_id)
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
Sub::init_add_publications()
{
  unsigned num_pubs = this->pubs_.size();

  OpenDDS::DCPS::AssociationData* pubs = new OpenDDS::DCPS::AssociationData[num_pubs];

  for (unsigned i = 0; i < num_pubs; i++)
    {
      // Ask the PubInfo object to populate the AssociationData object for us.
      this->pubs_[i].as_association(pubs[i]);
    }

  int result = this->add_publications(this->sub_id_,
                                      &(this->reader_),
                                      0,
                                      num_pubs,
                                      pubs);

  delete [] pubs;

  if (result != 0)
    {
      ACE_ERROR((LM_ERROR,
                 "(%P|%t) Failed to add publications to the "
                 "TransportInterface.\n"));
      throw TestException();
    }
}
