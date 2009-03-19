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

} // namespace
} // namespace:
