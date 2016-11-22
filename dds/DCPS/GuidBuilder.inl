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
GuidBuilder::entityId(long entityId)
{
  entityKey(entityId >> 8);
  entityKind(static_cast<CORBA::Octet>(0xff & entityId));
}

ACE_INLINE
GuidBuilder::operator GUID_t()
{
  return guid_;
}

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL
