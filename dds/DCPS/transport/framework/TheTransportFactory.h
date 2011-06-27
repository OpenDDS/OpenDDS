/*
 * $Id$
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_THETRANSPORTFACTORY_H
#define OPENDDS_DCPS_THETRANSPORTFACTORY_H

#include "TransportFactory.h"
#include "TransportRegistry.h"

#define TheTransportFactory OpenDDS::DCPS::TransportFactory::instance()
#define TheTransportRegistry OpenDDS::DCPS::TransportRegistry::instance()

#endif  /* OPENDDS_DCPS_THETRANSPORTFACTORY_H */
