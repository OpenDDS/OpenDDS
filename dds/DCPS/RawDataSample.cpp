/*
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */
#include "DCPS/DdsDcps_pch.h" //Only the _pch include should start with DCPS/
#include "RawDataSample.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

RawDataSample::RawDataSample()
  : message_id_(SAMPLE_DATA)
  , publication_id_(GUID_UNKNOWN)
  , sample_byte_order_(false)
  , encoding_kind_(Encoding::KIND_UNALIGNED_CDR)
{
  source_timestamp_.sec = 0;
  source_timestamp_.nanosec = 0;
}

RawDataSample::RawDataSample(const DataSampleHeader& header,
                             MessageId mid,
                             ACE_INT32 sec,
                             ACE_UINT32 nano_sec,
                             PublicationId pid,
                             bool byte_order,
                             ACE_Message_Block* blk,
                             Encoding::Kind encoding_kind)
  : header_(header)
  , message_id_(mid)
  , publication_id_(pid)
  , sample_byte_order_(byte_order)
  , sample_(blk->duplicate())
  , encoding_kind_(encoding_kind)
{
  source_timestamp_.sec = sec;
  source_timestamp_.nanosec = nano_sec;
}

RawDataSample::~RawDataSample()
{
}

RawDataSample::RawDataSample(const RawDataSample& other)
  : header_(other.header_)
  , message_id_(other.message_id_)
  , source_timestamp_(other.source_timestamp_)
  , publication_id_(other.publication_id_)
  , sample_byte_order_(other.sample_byte_order_)
  , sample_(other.sample_->duplicate())
  , encoding_kind_(other.encoding_kind_)
{
}

RawDataSample&
RawDataSample::operator=(const RawDataSample& other)
{
  RawDataSample tmp(other);
  swap(*this, tmp);
  return *this;
}

void swap(RawDataSample& lhs, RawDataSample& rhs)
{
  using std::swap;
  swap(lhs.header_, rhs.header_);
  swap(lhs.message_id_, rhs.message_id_);
  swap(lhs.source_timestamp_, rhs.source_timestamp_);
  swap(lhs.publication_id_, rhs.publication_id_);
  swap(lhs.sample_byte_order_, rhs.sample_byte_order_);
  swap(lhs.sample_, rhs.sample_);
  swap(lhs.encoding_kind_, rhs.encoding_kind_);
}

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL
