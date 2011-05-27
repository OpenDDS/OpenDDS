// -*- C++ -*-
//
// $Id$
#include  "SimpleSubscriber.h"
#include  "TestException.h"
#include  "dds/DCPS/transport/framework/TransportImpl.h"
#include  "dds/DCPS/transport/framework/TransportInterface.h"
#include  "dds/DCPS/transport/framework/ReceivedDataSample.h"
#include  "ace/SString.h"

SimpleSubscriber::SimpleSubscriber()
  : sub_id_ ( OpenDDS::DCPS::GUID_UNKNOWN), num_publications_ (0)
  , publications_ (0)
{ }

SimpleSubscriber::~SimpleSubscriber()
{
}


void
SimpleSubscriber::init(OpenDDS::DCPS::TransportIdType          transport_id,
                       OpenDDS::DCPS::RepoId                   sub_id,
                       ssize_t                             num_publications,
                       const OpenDDS::DCPS::AssociationData*   publications)
{
  this->sub_id_ = sub_id;
  this->num_publications_ = num_publications;
  this->publications_ = publications;

  // Obtain the transport.
  OpenDDS::DCPS::TransportImpl_rch transport =
    TheTransportFactory->obtain(transport_id);

  if (transport.is_nil())
    {
      // Failed to obtain the transport.
      ACE_ERROR((LM_ERROR,
                 "(%P|%t) Failed to obtain TransportImpl (id %d) from "
                 "TheTransportFactory.\n", transport_id));
      throw TestException();
    }

  // Attempt to attach ourselves to the transport.
  OpenDDS::DCPS::AttachStatus status = this->attach_transport (transport.in ());

  if (status != OpenDDS::DCPS::ATTACH_OK)
    {
      // We failed to attach to the transport for some reason.
      ACE_TString status_str;

      switch (status)
        {
              case OpenDDS::DCPS::ATTACH_BAD_TRANSPORT:
                status_str = ACE_TEXT("ATTACH_BAD_TRANSPORT");
                break;
              case OpenDDS::DCPS::ATTACH_ERROR:
                status_str = ACE_TEXT("ATTACH_ERROR");
                break;
              case OpenDDS::DCPS::ATTACH_INCOMPATIBLE_QOS:
                status_str = ACE_TEXT("ATTACH_INCOMPATIBLE_QOS");
                break;
              default:
                status_str = ACE_TEXT("Unknown Status");
                break;
        }

      ACE_ERROR((LM_ERROR,
                 "(%P|%t) Failed to attach to the transport. "
                 "AttachStatus == %s\n", status_str.c_str()));
      throw TestException();
    }

  // Good!  We are now attached to the transport.

  // Initialize our DataReader.
  this->reader_.init(sub_id);
}

void
SimpleSubscriber::associate ()
{
  OpenDDS::DCPS::AssociationInfo info;
  info.num_associations_ = this->num_publications_;
  info.association_data_ =
    const_cast<OpenDDS::DCPS::AssociationData*>(this->publications_);

  // Add the association between the local sub_id and the remote pub_id
  // to the transport via the TransportInterface.
  int result = this->add_publications (this->sub_id_,
                                       info,
                                       0,   /* priority */
                                       &this->reader_);

  if (result != 0)
    {
      ACE_ERROR((LM_ERROR,
                 "(%P|%t) Failed to add publications to the "
                 "TransportInterface.\n"));
      throw TestException();
    }
}

void
SimpleSubscriber::remove_associations(ssize_t size,
                                      const OpenDDS::DCPS::RepoId* remote_ids,
                                      const OpenDDS::DCPS::RepoId sub_id)
{
  TransportInterface::remove_associations(size, remote_ids, sub_id, false);
}

void
SimpleSubscriber::cleanup()
{
  ACE_DEBUG((LM_DEBUG,
             "(%P|%t) The transport has been shutdown.  Cleaning up.\n"));
  this->reader_.transport_lost();
  // TBD - Shutdown the reactor.
}


int
SimpleSubscriber::received_test_message() const
{
  return this->reader_.received_test_message();
}
