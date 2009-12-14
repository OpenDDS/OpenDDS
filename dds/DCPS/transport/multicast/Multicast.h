/*
 * $Id$
 *
 * Copyright 2009 Object Computing, Inc.
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef DCPS_MULTICAST_H
#define DCPS_MULTICAST_H

#include "Multicast_Export.h"

namespace OpenDDS {
namespace DCPS {

class OpenDDS_Multicast_Export MulticastInitializer {
public:
  MulticastInitializer();
};

static MulticastInitializer multicast_init;

} // namespace DCPS
} // namespace OpenDDS

#endif  /* DCPS_MULTICAST_H */
