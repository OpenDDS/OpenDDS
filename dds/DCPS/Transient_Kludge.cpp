/*
 * $Id$
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "DCPS/DdsDcps_pch.h" //Only the _pch include should start with DCPS/
#include "Transient_Kludge.h"

#include "tao/TAO_Singleton.h"

#if !defined (__ACE_INLINE__)
#include "Transient_Kludge.inl"
#endif /* __ACE_INLINE__ */

OpenDDS::DCPS::Transient_Kludge*
OpenDDS::DCPS::Transient_Kludge::instance()
{
  // Hide the template instantiation to prevent multiple instances
  // from being created.

  return
    TAO_Singleton<Transient_Kludge, TAO_SYNCH_MUTEX>::instance();
}
