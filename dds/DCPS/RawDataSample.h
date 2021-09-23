/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_RAWDATASAMPLE_H
#define OPENDDS_DCPS_RAWDATASAMPLE_H

#include "DataSampleHeader.h"

#include "ace/Basic_Types.h"

ACE_BEGIN_VERSIONED_NAMESPACE_DECL
class ACE_Message_Block;
ACE_END_VERSIONED_NAMESPACE_DECL

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

class OpenDDS_Dcps_Export RawDataSample
{
public:
  RawDataSample();
  RawDataSample(const DataSampleHeader& header,
                ACE_Message_Block* blk);

  RawDataSample(const RawDataSample&);
  ~RawDataSample();
  RawDataSample& operator=(const RawDataSample&);

  /// The sample data header
  DataSampleHeader header_;
  /// The data in unspecified format
  Message_Block_Ptr sample_;
};

void swap(RawDataSample& lhs, RawDataSample& rhs);

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif
