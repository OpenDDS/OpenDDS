/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef UPDATEPROCESSOR_T_CPP
#define UPDATEPROCESSOR_T_CPP

#if !defined (ACE_LACKS_PRAGMA_ONCE)
#pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

#include "DcpsInfo_pch.h"
#include "dds/DCPS/debug.h"
#include "UpdateProcessor_T.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace Federator {

template<class DataType>
UpdateProcessor<DataType>::UpdateProcessor()
{
  if (OpenDDS::DCPS::DCPS_debug_level > 0) {
    ACE_DEBUG((LM_DEBUG,
               ACE_TEXT("(%P|%t) UpdateProcessor::UpdateProcessor()\n")));
  }
}

template<class DataType>
UpdateProcessor<DataType>::~UpdateProcessor()
{
  if (OpenDDS::DCPS::DCPS_debug_level > 0) {
    ACE_DEBUG((LM_DEBUG,
               ACE_TEXT("(%P|%t) UpdateProcessor::~UpdateProcessor()\n")));
  }
}

template<class DataType>
void
UpdateProcessor<DataType>::processSample(
  const DataType*          sample,
  const DDS::SampleInfo* info)
{
  if (OpenDDS::DCPS::DCPS_debug_level > 0) {
    ACE_DEBUG((LM_DEBUG,
               ACE_TEXT("(%P|%t) UpdateProcessor::processSample()\n")));
  }

  if (info->valid_data) {
    switch (sample->action) {
    case CreateEntity:
      this->processCreate(sample, info);
      break;
    case UpdateQosValue1:
      this->processUpdateQos1(sample, info);
      break;
    case UpdateQosValue2:
      this->processUpdateQos2(sample, info);
      break;
    case UpdateFilterExpressionParams:
      this->processUpdateFilterExpressionParams(sample, info);
      break;
    case DestroyEntity:
      this->processDelete(sample, info);
      break;
    default:
      ACE_ERROR((LM_ERROR,
                 ACE_TEXT("(%P|%t) ERROR: UpdateProcessor::processSample() - ")
                 ACE_TEXT("upsupported action type: %d.\n"),
                 sample->action));
      break;
    }

  } else {
    if (OpenDDS::DCPS::DCPS_debug_level > 0) {
      ACE_DEBUG((LM_DEBUG,
                 ACE_TEXT("(%P|%t) UpdateProcessor::processSample() - ")
                 ACE_TEXT("sample not valid, declining to process.\n")));
    }
  }
}

template<class DataType>
void
UpdateProcessor<DataType>::processUpdateQos2(
  const DataType*          /* sample */,
  const DDS::SampleInfo* /* info */)
{
  /* This method intentionally left unimplemented. */
}

template<class DataType>
void
UpdateProcessor<DataType>::processUpdateFilterExpressionParams(
  const DataType*        /* sample */,
  const DDS::SampleInfo* /* info */)
{
  /* This method intentionally left unimplemented. */
}

} // namespace Federator
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif /* UPDATEPROCESSOR_T_CPP */
