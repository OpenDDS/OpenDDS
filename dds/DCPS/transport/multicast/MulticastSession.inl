/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

ACE_INLINE MulticastDataLink*
MulticastSession::link()
{
  return this->link_;
}

ACE_INLINE MulticastPeer
MulticastSession::remote_peer() const
{
  return this->remote_peer_;
}

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL
