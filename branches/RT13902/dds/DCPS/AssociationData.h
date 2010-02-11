/*
 * $Id$
 *
 * Copyright 2010 Object Computing, Inc.
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_ASSOCIATIONDATA_H
#define OPENDDS_DCPS_ASSOCIATIONDATA_H

#include "dds/DdsDcpsInfoUtilsC.h"

#include <vector>

namespace OpenDDS {
namespace DCPS {

struct AssociationData {
  RepoId                  remote_id_;
  TransportInterfaceInfo  remote_data_;
};

struct AssociationInfo {
  ssize_t           num_associations_;
  AssociationData*  association_data_;
};

typedef std::vector<AssociationInfo> AssociationInfoList;

} // namespace DCPS
} // namespace OpenDDS

#endif  /* OPENDDS_DCPS_ASSOCIATIONDATA_H */
