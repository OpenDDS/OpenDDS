// ============================================================================
/**
 *  @file   DCPS_Utils.h
 *
 *  $Id$
 *
 *
 */
// ============================================================================
#ifndef DCPS_UTILS_H
#define DCPS_UTILS_H

#include /**/ "dds/DdsDcpsInfrastructureC.h"
#include /**/ "DCPS_IR_Subscription.h"
#include /**/ "DCPS_IR_Publication.h"

#if !defined (ACE_LACKS_PRAGMA_ONCE)
#pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */


/// Increments the count of occurances of the incompatible policy
///  for the status
void increment_incompatibility_count (OpenDDS::DCPS::IncompatibleQosStatus* status,
                                      ::DDS::QosPolicyId_t incompatible_policy);

/// Compares whether a publication and subscription are compatible
int compatibleQOS(DCPS_IR_Publication * publication,
                  DCPS_IR_Subscription * subscription);


#endif /* DCPS_UTILS_H */
