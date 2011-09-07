/*
 * $Id$
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "RtpsUdpTransport.h"
#include "RtpsUdpInst.h"
#include "RtpsUdpSendStrategy.h"
#include "RtpsUdpReceiveStrategy.h"

#include "ace/CDR_Base.h"
#include "ace/Log_Msg.h"

#include "dds/DCPS/transport/framework/NetworkAddress.h"
#include "dds/DCPS/AssociationData.h"

namespace OpenDDS {
namespace DCPS {

RtpsUdpTransport::RtpsUdpTransport(const TransportInst_rch& inst)
{
  if (!inst.is_nil()) {
    configure(inst.in());
  }
}

RtpsUdpDataLink*
RtpsUdpTransport::make_datalink(const ACE_INET_Addr& remote_address,
                                bool active)
{
  RtpsUdpDataLink_rch link;
  ACE_NEW_RETURN(link, RtpsUdpDataLink(this, active), 0);

  if (link.is_nil()) {
    ACE_ERROR_RETURN((LM_ERROR,
                      ACE_TEXT("(%P|%t) ERROR: ")
                      ACE_TEXT("RtpsUdpTransport::make_datalink: ")
                      ACE_TEXT("failed to create DataLink!\n")),
                     0);
  }

  // Configure link with transport configuration and reactor task:
  link->configure(this->config_i_.in(), reactor_task());

  // Assign send strategy:
  RtpsUdpSendStrategy* send_strategy;
  ACE_NEW_RETURN(send_strategy, RtpsUdpSendStrategy(link.in()), 0);
  link->send_strategy(send_strategy);

  // Assign receive strategy:
  RtpsUdpReceiveStrategy* recv_strategy;
  ACE_NEW_RETURN(recv_strategy, RtpsUdpReceiveStrategy(link.in()), 0);
  link->receive_strategy(recv_strategy);

  // Open logical connection:
  if (!link->open(remote_address)) {
    ACE_ERROR_RETURN((LM_ERROR,
                      ACE_TEXT("(%P|%t) ERROR: ")
                      ACE_TEXT("RtpsUdpTransport::make_datalink: ")
                      ACE_TEXT("failed to open DataLink!\n")),
                     0);
  }

  return link._retn();
}

DataLink*
RtpsUdpTransport::find_datalink_i(const RepoId& /*local_id*/,
                                  const RepoId& /*remote_id*/,
                                  const TransportBLOB& /*remote_data*/,
                                  CORBA::Long /*priority*/,
                                  bool /*active*/)
{
  return 0;
}

DataLink*
RtpsUdpTransport::connect_datalink_i(const RepoId& /*local_id*/,
                                     const RepoId& /*remote_id*/,
                                     const TransportBLOB& remote_data,
                                     CORBA::Long priority)
{
  return 0;
}

DataLink*
RtpsUdpTransport::accept_datalink(ConnectionEvent& ce)
{
  return 0;
}

void
RtpsUdpTransport::stop_accepting(ConnectionEvent& ce)
{
}

bool
RtpsUdpTransport::configure_i(TransportInst* config)
{
  this->config_i_ = dynamic_cast<RtpsUdpInst*>(config);
  if (this->config_i_ == 0) {
    ACE_ERROR_RETURN((LM_ERROR,
                      ACE_TEXT("(%P|%t) ERROR: ")
                      ACE_TEXT("RtpsUdpTransport::configure_i: ")
                      ACE_TEXT("invalid configuration!\n")),
                     false);
  }
  this->config_i_->_add_ref();

  this->create_reactor_task();

  return true;
}

void
RtpsUdpTransport::shutdown_i()
{
  // Shutdown reserved datalinks and release configuration:

  this->config_i_ = 0;
}

bool
RtpsUdpTransport::connection_info_i(TransportLocator& info) const
{
  return true;
}

ACE_INET_Addr
RtpsUdpTransport::get_connection_addr(const TransportBLOB& data) const
{
  ACE_INET_Addr local_address;
  return local_address;
}

void
RtpsUdpTransport::release_datalink_i(DataLink* link, bool /*release_pending*/)
{
}

PriorityKey
RtpsUdpTransport::blob_to_key(const TransportBLOB& remote,
                          CORBA::Long priority,
                          bool active)
{
  return PriorityKey();
}

void
RtpsUdpTransport::passive_connection(const ACE_INET_Addr& remote_address,
                                 ACE_Message_Block* data)
{
}

} // namespace DCPS
} // namespace OpenDDS
