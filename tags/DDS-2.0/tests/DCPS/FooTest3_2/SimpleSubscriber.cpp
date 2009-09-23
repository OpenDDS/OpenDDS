// -*- C++ -*-
//
// $Id$
#include  "SimpleSubscriber.h"
#include  "TestException.h"
#include  "dds/DCPS/transport/framework/TransportImpl.h"
#include  "dds/DCPS/transport/framework/TransportInterface.h"
#include  "dds/DCPS/transport/framework/ReceivedDataSample.h"
#include <string>

SimpleSubscriber::SimpleSubscriber()
{
}


SimpleSubscriber::~SimpleSubscriber()
{
}


void
SimpleSubscriber::init(OpenDDS::DCPS::TransportIdType          transport_id,
                       OpenDDS::DCPS::RepoId                   sub_id,
                       ssize_t                             num_publications,
                       const OpenDDS::DCPS::AssociationData*   publications,
                       int                                 receive_delay_msec)
{
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
  OpenDDS::DCPS::AttachStatus status = this->attach_transport(transport.in ());

  if (status != OpenDDS::DCPS::ATTACH_OK)
    {
      // We failed to attach to the transport for some reason.
      std::string status_str;

      switch (status)
        {
          case OpenDDS::DCPS::ATTACH_BAD_TRANSPORT:
            status_str = "ATTACH_BAD_TRANSPORT";
            break;
          case OpenDDS::DCPS::ATTACH_ERROR:
            status_str = "ATTACH_ERROR";
            break;
          case OpenDDS::DCPS::ATTACH_INCOMPATIBLE_QOS:
            status_str = "ATTACH_INCOMPATIBLE_QOS";
            break;
          default:
            status_str = "Unknown Status";
            break;
        }

      ACE_ERROR((LM_ERROR,
                ACE_TEXT("(%P|%t) Failed to attach to the transport. ")
                ACE_TEXT("AttachStatus == %s\n"),
                ACE_TEXT_CHAR_TO_TCHAR(status_str.c_str())));
      throw TestException();
    }

  // Good!  We are now attached to the transport.

  // Initialize our DataReader.
  this->reader_.init(sub_id, receive_delay_msec);

  // Add the association between the local sub_id and the remote pub_id
  // to the transport via the TransportInterface.
  int result = this->add_publications(sub_id,
                                      &this->reader_,
                                      0,   /* priority */
                                      num_publications,
                                      publications);

  if (result != 0)
    {
      ACE_ERROR((LM_ERROR,
                 "(%P|%t) Failed to add publications to the "
                 "TransportInterface.\n"));
      throw TestException();
    }

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
