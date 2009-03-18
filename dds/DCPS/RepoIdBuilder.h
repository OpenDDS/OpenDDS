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
class RepoIdBuilder : public GuidBuilder
{
public:
  explicit RepoIdBuilder(RepoId& repoId);

  ~RepoIdBuilder();

  void federationId(long federationId);

  void participantId(long participantId);

  void from_BuiltinTopicKey(DDS::BuiltinTopicKey_t key);
};

} // namespace
} // namespace

#endif /* DCPS_REPOIDBUILDER_H */
