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
{
}

RawDataSample::RawDataSample(const DataSampleHeader& header,
                             ACE_Message_Block* blk)
  : header_(header)
  , sample_(blk->duplicate())
{
}

RawDataSample::~RawDataSample()
{
}

RawDataSample::RawDataSample(const RawDataSample& other)
  : header_(other.header_)
  , sample_(other.sample_->duplicate())
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
  swap(lhs.sample_, rhs.sample_);
}

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL
