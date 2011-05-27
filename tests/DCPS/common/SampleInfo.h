#ifndef _SAMPLEINFO_H_
#define _SAMPLEINFO_H_
// ============================================================================
// -*- C++ -*-
// ============================================================================
/**
 *  @file   SampleInfo.h
 *
 *  $Id$
 *
 *
 */

#include "dds/DdsDcpsInfrastructureC.h"
#include "common_export.h"

void Common_Export PrintSampleInfo(const ::DDS::SampleInfo& si) ;

/*
 * NOTE: comparison operators don't compare instance_handle or
 * source_timestamp
 */
bool Common_Export operator==(const ::DDS::SampleInfo& si1, const ::DDS::SampleInfo& si2) ;

inline bool operator!=(const ::DDS::SampleInfo& si1,
                       const ::DDS::SampleInfo& si2)
{
    return !operator==(si1, si2) ;
}

#endif
