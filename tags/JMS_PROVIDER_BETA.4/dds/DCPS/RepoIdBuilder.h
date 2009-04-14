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

  void federationId(long federationId);

  void participantId(long participantId);

  void from_BuiltinTopicKey(DDS::BuiltinTopicKey_t key);
};

} // namespace
} // namespace

#ifdef __ACE_INLINE__
# include "RepoIdBuilder.inl"
#endif /* __ACE_INLINE__ */

#endif /* DCPS_REPOIDBUILDER_H */
