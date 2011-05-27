/*
 * $Id$
 *
 * Copyright 2010 Object Computing, Inc.
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef DCPS_THREADSYNCHSTRATEGY_RCH_H
#define DCPS_THREADSYNCHSTRATEGY_RCH_H

#include "dds/DCPS/RcHandle_T.h"

namespace OpenDDS {
namespace DCPS {

class ThreadSynchStrategy;

typedef RcHandle<ThreadSynchStrategy> ThreadSynchStrategy_rch;

} // namespace DCPS
} // namespace OpenDDS

#endif  /* DCPS_THREADSYNCHSTRATEGY_RCH_H */
