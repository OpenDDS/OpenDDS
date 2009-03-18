/*
 * $Id$
 */

#include "DCPS/DdsDcps_pch.h" //Only the _pch include should start with DCPS/

#include "RepoIdBuilder.h"

namespace OpenDDS
{
namespace DCPS
{
RepoIdBuilder::RepoIdBuilder()
{
}

RepoIdBuilder::RepoIdBuilder(RepoId& repoId)
  : GuidBuilder(repoId)
{
}

RepoIdBuilder::~RepoIdBuilder()
{
}

inline void
RepoIdBuilder::federationId(long federationId)
{
  guidPrefix1(federationId);
}

inline void
RepoIdBuilder::participantId(long participantId)
{
  guidPrefix2(participantId); 
}

void
RepoIdBuilder::from_BuiltinTopicKey(DDS::BuiltinTopicKey_t key)
{
  federationId(key[0]);
  participantId(key[1]);
  entityId(key[2]);
}

} // namespace
} // namespace:
