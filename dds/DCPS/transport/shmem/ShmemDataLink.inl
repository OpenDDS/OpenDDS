/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

ACE_INLINE void
ShmemDataLink::configure(ShmemInst* config)
{
  this->config_ = config;
}

ACE_INLINE ShmemInst*
ShmemDataLink::config()
{
  return this->config_;
}

ACE_INLINE std::string
ShmemDataLink::peer_address()
{
  return this->peer_address_;
}

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL
