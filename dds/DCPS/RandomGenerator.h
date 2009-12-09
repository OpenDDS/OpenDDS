/*
 * $Id$
 *
 * Copyright 2009 Object Computing, Inc.
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef DCPS_RANDOMGENERATOR_H
#define DCPS_RANDOMGENERATOR_H

#include "dcps_export.h"

#include <cstdlib>

namespace OpenDDS {
namespace DCPS {

struct OpenDDS_Dcps_Export RandomGenerator {
  double value_;

  RandomGenerator&  operator++();
  RandomGenerator   operator++(int);

  operator double() const;
};

} // namespace DCPS
} // namespace OpenDDS

#ifdef __ACE_INLINE__
# include "RandomGenerator.inl"
#endif  /* __ACE_INLINE__ */

#endif  /* DCPS_RANDOMGENERATOR_H */
