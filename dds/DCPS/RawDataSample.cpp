/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */
#include "DCPS/DdsDcps_pch.h" //Only the _pch include should start with DCPS/
#include "dds/DCPS/RawDataSample.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {


RawDataSample::RawDataSample()
  : message_id_(SAMPLE_DATA)
  , publication_id_(GUID_UNKNOWN)
  , sample_byte_order_(0)
  , sample_(0)
{
  source_timestamp_.sec = 0;
  source_timestamp_.nanosec = 0;
}

RawDataSample::RawDataSample(MessageId          mid,
                             ACE_INT32            sec,
                             ACE_UINT32           nano_sec,
                             PublicationId      pid,
                             bool               byte_order,
                             ACE_Message_Block* blk)
  : message_id_(mid)
  , publication_id_(pid)
  , sample_byte_order_(byte_order)
  , sample_(blk->duplicate())
{
  source_timestamp_.sec = sec;
  source_timestamp_.nanosec = nano_sec;
}

RawDataSample::~RawDataSample()
{
  if (sample_)
    sample_->release();
}

RawDataSample::RawDataSample(const RawDataSample& other)
  : message_id_(other.message_id_)
  , source_timestamp_(other.source_timestamp_)
  , publication_id_(other.publication_id_)
  , sample_byte_order_(other.sample_byte_order_)
  , sample_(other.sample_->duplicate())
{
}


RawDataSample&
RawDataSample::operator=(const RawDataSample& other)
{
  RawDataSample tmp(other);
  std::swap(*this, tmp);
  return *this;
}

void swap(RawDataSample& lhs, RawDataSample& rhs)
{
  std::swap(lhs.message_id_, rhs.message_id_);
  std::swap(lhs.publication_id_, rhs.publication_id_);
  std::swap(lhs.sample_byte_order_, rhs.sample_byte_order_);
  std::swap(lhs.sample_, rhs.sample_);
}

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL
