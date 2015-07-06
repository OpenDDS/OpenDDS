/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "DCPS/DdsDcps_pch.h" //Only the _pch include should start with DCPS/
#include "Transient_Kludge.h"

#include "ace/Singleton.h"

#if !defined (__ACE_INLINE__)
#include "Transient_Kludge.inl"
#endif /* __ACE_INLINE__ */

OpenDDS::DCPS::Transient_Kludge*
OpenDDS::DCPS::Transient_Kludge::instance()
{
  // Hide the template instantiation to prevent multiple instances
  // from being created.

  return ACE_Singleton<Transient_Kludge, ACE_SYNCH_MUTEX>::instance();
}
