/*
 * $Id$
 *
 * Copyright 2010 Object Computing, Inc.
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "DCPS/DdsDcps_pch.h" //Only the _pch include should start with DCPS/
#include "InfoRepoUtils.h"
#include <string>

namespace OpenDDS {
namespace DCPS {

  namespace InfoRepoUtils {

    // Get and narrow the InfoRepo from an ior.
    // Also accepts "host:port" as a valid InfoRepo ior.
    // Returns the InfoRepo if successful, nil if not
    DCPSInfo_ptr get_repo(const char* ior, CORBA::ORB_ptr orb)
    {
      CORBA::Object_var o;
      try {
        o = orb->string_to_object(ior);
      } catch (CORBA::INV_OBJREF&) {
        // host:port format causes an exception; try again
        // with corbaloc format
        std::string second_try("corbaloc:iiop:");
        second_try += ior;
        second_try += "/DCPSInfoRepo";

        o = orb->string_to_object(second_try.c_str());
      }
      
      return DCPSInfo::_narrow(o.in());  
    }
  } // InfoRepoUtils

} // DCPS
} // OpenDDS
