/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_RAWDATASAMPLE_H
#define OPENDDS_DCPS_RAWDATASAMPLE_H

#include "dds/DCPS/DataSampleHeader.h"

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
  RawDataSample(MessageId,
                ACE_INT32          sec,
                ACE_UINT32         nano_sec,
                PublicationId      pid,
                bool               byte_order,
                ACE_Message_Block* blk);

  RawDataSample(const RawDataSample&);
  ~RawDataSample();
  RawDataSample& operator=(const RawDataSample&);

  /// The enum indicating the message type
  MessageId message_id_;
  /// The timestamp the sender put on the sample
  DDS::Time_t source_timestamp_;
  /// Id of the datawriter that sent the sample
  PublicationId publication_id_;
  /// 0 -  Message encoded using big-endian byte order. (see ace/CDR_Base.h)
  /// 1 -  Message encoded using little-endian byte order.
  bool sample_byte_order_;
  /// The data in unspecified format
  ACE_Message_Block*       sample_;
};

void swap(RawDataSample& lhs, RawDataSample& rhs);

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif
