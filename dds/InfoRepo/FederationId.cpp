/*
 * $Id$
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "DcpsInfo_pch.h"

#include "FederationId.h"

TAO_DDS_DCPSFederationId::TAO_DDS_DCPSFederationId(RepoKey initId)
: id_(initId)
, overridden_(false)
{
}

void
TAO_DDS_DCPSFederationId::id(RepoKey fedId)
{
  this->id_ = fedId;
  this->overridden_ = true;
}

TAO_DDS_DCPSFederationId::RepoKey
TAO_DDS_DCPSFederationId::id() const
{
  return this->id_;
}

/*
void
TAO_DDS_DCPSFederationId::overridden(bool overrideId)
{
  this->overridden_ = overrideId;
}*/

bool
TAO_DDS_DCPSFederationId::overridden() const
{
  return this->overridden_;
}

