/*
 * $Id$
 *
 * Copyright 2010 Object Computing, Inc.
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

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
