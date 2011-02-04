/*
 * $Id$
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef DCPS_SIMPLETCP_H
#define DCPS_SIMPLETCP_H

#include /**/ "ace/pre.h"

#include "SimpleTcp_export.h"
#include "ace/Service_Object.h"
#include "ace/Service_Config.h"

#if !defined (ACE_LACKS_PRAGMA_ONCE)
# pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

class SimpleTcp_Export DCPS_SimpleTcp_Initializer {
public:
  // Constructor.
  DCPS_SimpleTcp_Initializer();
};

static DCPS_SimpleTcp_Initializer DCPS_SimpleTcp_initializer;

#include /**/ "ace/post.h"

#endif /* DCPS_SIMPLETCP_H */
