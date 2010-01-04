/*
 * $Id$
 *
 * Copyright 2010 Object Computing, Inc.
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef DCPS_MULTICASTCONFIGURATION_RCH_H
#define DCPS_MULTICASTCONFIGURATION_RCH_H

#include "dds/DCPS/RcHandle_T.h"

namespace OpenDDS {
namespace DCPS {

class MulticastConfiguration;

typedef RcHandle<MulticastConfiguration> MulticastConfiguration_rch;

} // namespace DCPS
} // namespace OpenDDS

#endif  /* DCPS_MULTICASTCONFIGURATION_RCH_H */
