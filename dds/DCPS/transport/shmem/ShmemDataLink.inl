/*
 * $Id$
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

namespace OpenDDS {
namespace DCPS {

ACE_INLINE void
ShmemDataLink::configure(ShmemInst* config)
{
  this->config_ = config;
}

ACE_INLINE void
ShmemDataLink::send_strategy(ShmemSendStrategy* send_strategy)
{
  this->send_strategy_ = send_strategy;
}

ACE_INLINE void
ShmemDataLink::receive_strategy(ShmemReceiveStrategy* recv_strategy)
{
  this->recv_strategy_ = recv_strategy;
}

ACE_INLINE ShmemInst*
ShmemDataLink::config()
{
  return this->config_;
}

ACE_INLINE std::string
ShmemDataLink::remote_address()
{
  return this->remote_address_;
}

} // namespace DCPS
} // namespace OpenDDS
