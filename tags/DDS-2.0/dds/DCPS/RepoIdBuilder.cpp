/*
 * $Id$
 *
 * Copyright 2009 Object Computing, Inc.
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "DCPS/DdsDcps_pch.h" //Only the _pch include should start with DCPS/

#include "RepoIdBuilder.h"

#ifndef __ACE_INLINE__
# include "RepoIdBuilder.inl"
#endif /* __ACE_INLINE__ */

namespace OpenDDS {
namespace DCPS {

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

} // namespace DCPS
} // namespace OpenDDS
