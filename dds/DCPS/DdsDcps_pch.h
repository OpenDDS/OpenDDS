/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef DDSDCPS_PCH_H
#define DDSDCPS_PCH_H

#ifdef USING_PCH

#if defined (_MSC_VER) && (_MSC_VER >= 1900)
// Disable warnings suggesting use of checked iterators
#define _SCL_SECURE_NO_WARNINGS
#endif

#include "ace/config-all.h"

#include "dds/DdsDcpsDomainC.h"

#include "tao/ORB_Core.h"

#endif
#endif
