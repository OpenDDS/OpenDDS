/*
 * $Id$
 */

namespace OpenDDS
{
namespace DCPS
{
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
RepoIdBuilder::from_BuiltinTopicKey(DDS::BuiltinTopicKey_t key)
{
  federationId(key[0]);
  participantId(key[1]);
  entityId(key[2]);
}

} // namespace
} // namespace
