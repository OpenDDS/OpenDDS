/*
 * $Id$
 */

#ifndef DCPS_REPOIDBUILDER_H
#define DCPS_REPOIDBUILDER_H

#include "dds/DdsDcpsInfrastructureC.h"
#include "dds/DdsDcpsInfoUtilsC.h"

#include "GuidBuilder.h"

#include "dcps_export.h"

namespace OpenDDS
{
namespace DCPS
{
class OpenDDS_Dcps_Export RepoIdBuilder : public GuidBuilder
{
public:
  RepoIdBuilder();
  explicit RepoIdBuilder(RepoId& repoId);

  ~RepoIdBuilder();

  void federationId(long federationId)
  {
    guidPrefix1(federationId);
  }

  void participantId(long participantId)
  {
    guidPrefix2(participantId); 
  }

  void from_BuiltinTopicKey(DDS::BuiltinTopicKey_t key)
  {
    federationId(key[0]);
    participantId(key[1]);
    entityId(key[2]);
  }
};

} // namespace
} // namespace

#endif /* DCPS_REPOIDBUILDER_H */
