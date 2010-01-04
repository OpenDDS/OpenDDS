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
RepoIdBuilder::federationId(long federationId)
{
  guidPrefix1(federationId);
}

ACE_INLINE void
RepoIdBuilder::participantId(long participantId)
{
  guidPrefix2(participantId);
}

ACE_INLINE void
RepoIdBuilder::from_BuiltinTopicKey(const DDS::BuiltinTopicKey_t& key)
{
  federationId(key.value[0]);
  participantId(key.value[1]);
  entityId(key.value[2]);
}

} // namespace DCPS
} // namespace OpenDDS
