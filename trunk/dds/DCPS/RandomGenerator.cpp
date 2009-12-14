/*
 * $Id$
 *
 * Copyright 2009 Object Computing, Inc.
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "DCPS/DdsDcps_pch.h" //Only the _pch include should start with DCPS/

#include "RandomGenerator.h"

#ifndef __ACE_INLINE__
# include "RandomGenerator.inl"
#endif  /* __ACE_INLINE__ */

#include "ace/OS_NS_time.h"

namespace OpenDDS {
namespace DCPS {

struct RandomSeed {
  RandomSeed() {
    std::srand(static_cast<unsigned>(ACE_OS::time(0)));
  }
} random_seed;

} // namespace OpenDDS
} // namespace DCPS
