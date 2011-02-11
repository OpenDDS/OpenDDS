/*
 * $Id$
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "DCPS/DdsDcps_pch.h" //Only the _pch include should start with DCPS/

#include "RepoIdConverter.h"

namespace OpenDDS {
namespace DCPS {

RepoIdConverter::RepoIdConverter(const RepoId& repoId)
  : GuidConverter(repoId)
{}

RepoIdConverter::~RepoIdConverter()
{}

long
RepoIdConverter::federationId() const
{
  return guid_.guidPrefix[4] << 24 |
         guid_.guidPrefix[5] << 16 |
         guid_.guidPrefix[6] <<  8 |
         guid_.guidPrefix[7];
}

long
RepoIdConverter::participantId() const
{
  return guid_.guidPrefix[ 8] << 24 |
         guid_.guidPrefix[ 9] << 16 |
         guid_.guidPrefix[10] <<  8 |
         guid_.guidPrefix[11];
}

void
RepoIdConverter::get_BuiltinTopicKey(DDS::BuiltinTopicKey_t& key) const
{
  key.value[0] = federationId();
  key.value[1] = participantId();
  key.value[2] = entityId();
}

RepoIdConverter::operator DDS::InstanceHandle_t() const
{
  return entityId();
}

} // namespace DCPS
} // namespace OpenDDS
