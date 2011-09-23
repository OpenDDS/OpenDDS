/*
 * $Id$
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "RtpsUdpDataLink.h"
#include "RtpsUdpTransport.h"
#include "RtpsUdpInst.h"

#include "dds/DCPS/transport/framework/NetworkAddress.h"
#include "dds/DCPS/transport/framework/TransportCustomizedElement.h"
#include "dds/DCPS/transport/framework/TransportSendElement.h"

#include "dds/DCPS/RTPS/RtpsMessageTypesTypeSupportImpl.h"

#include "ace/Default_Constants.h"
#include "ace/Log_Msg.h"
#include "ace/Message_Block.h"

#ifndef __ACE_INLINE__
# include "RtpsUdpDataLink.inl"
#endif  /* __ACE_INLINE__ */


namespace OpenDDS {
namespace DCPS {

RtpsUdpDataLink::RtpsUdpDataLink(RtpsUdpTransport* transport,
                                 const RepoId& local_id,
                                 bool active)
  : DataLink(transport,
             0, // priority
             false, // is_loopback,
             active),// is_active
    active_(active),
    config_(0),
    reactor_task_(0),
    local_id_(local_id)
{
}

bool
RtpsUdpDataLink::open()
{
  if (this->socket_.open(this->config_->local_address_) != 0) {
    ACE_ERROR_RETURN((LM_ERROR,
                      ACE_TEXT("(%P|%t) ERROR: ")
                      ACE_TEXT("RtpsUdpDataLink::open: socket open: %m\n")),
                     false);
  }

  if (start(static_rchandle_cast<TransportSendStrategy>(this->send_strategy_),
            static_rchandle_cast<TransportStrategy>(this->recv_strategy_))
      != 0) {
    stop_i();
    ACE_ERROR_RETURN((LM_ERROR,
                      ACE_TEXT("(%P|%t) ERROR: ")
                      ACE_TEXT("UdpDataLink::open: start failed!\n")),
                     false);
  }

  return true;
}

void
RtpsUdpDataLink::control_received(ReceivedDataSample& sample,
                                  const ACE_INET_Addr& remote_address)
{
}

void
RtpsUdpDataLink::stop_i()
{
}

TransportQueueElement*
RtpsUdpDataLink::customize_queue_element(TransportQueueElement* element)
{
  const ACE_Message_Block* msg = element->msg();
  if (!msg) {
    return element;
  }

  TransportSendElement* tse = dynamic_cast<TransportSendElement*>(element);
  TransportCustomizedElement* tce =
    dynamic_cast<TransportCustomizedElement*>(element);

  ACE_Message_Block* data = 0;
  const DataSampleListElement* dsle = 0;

  // Based on the type of 'element', find and duplicate the data payload
  // continuation block.
  if (tse) {
    // {DataSampleHeader} -> {Data Payload}
    data = msg->cont()->duplicate();
    dsle = tse->sample();
  } else if (tce) {
    // {DataSampleHeader} -> {Content Filtering GUIDs} -> {Data Payload}
    data = msg->cont()->cont()->duplicate();
    dsle = tce->original_send_element()->sample();
  } else {
    //TODO: handle other types?
    return element;
  }

  // Create RTPS Submessage(s) in place of the OpenDDS DataSampleHeader

  OpenDDS::RTPS::SubmessageSeq subm;
  RtpsSampleHeader::populate_submessages(subm, *dsle);

  size_t size = 0, padding = 0;
  for (CORBA::ULong i = 0; i < subm.length(); ++i) {
    if ((size + padding) % 4) {
      padding += 4 - ((size + padding) % 4);
    }
    gen_find_size(subm[i], size, padding);
  }

  ACE_Message_Block* hdr =
    new ACE_Message_Block(size + padding, ACE_Message_Block::MB_DATA, data);

  for (CORBA::ULong i = 0; i < subm.length(); ++i) {
    // byte swapping is handled in the operator<<() implementation
    Serializer ser(hdr, false, Serializer::ALIGN_CDR);
    ser << subm[i];
    const size_t len = hdr->length();
    if (len % 4) {
      hdr->wr_ptr(4 - (len % 4));
    }
  }

  TransportCustomizedElement* rtps = TransportCustomizedElement::alloc(element);
  rtps->set_msg(hdr);
  rtps->set_requires_exclusive();
  return rtps;
}

} // namespace DCPS
} // namespace OpenDDS
