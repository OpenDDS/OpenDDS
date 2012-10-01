/*
 * $Id$
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_RAWDATASAMPLE_H
#define OPENDDS_DCPS_RAWDATASAMPLE_H

#include "dds/DCPS/DataSampleHeader.h"

class ACE_Message_Block;

namespace OpenDDS {
namespace DCPS {
   
class OpenDDS_Dcps_Export RawDataSample 
{
public:
  RawDataSample();
  RawDataSample(MessageId,
                 int32_t sec,
                 uint32_t nano_sec,
                 PublicationId pid,
                 bool byte_order,
                 ACE_Message_Block* blk);
  
  RawDataSample(const RawDataSample&);
  ~RawDataSample();
  RawDataSample& operator=(const RawDataSample&);
  
  /// The enum inidicating the message type
  MessageId message_id_;
  /// The timestamp the sender put on the sample
  DDS::Time_t              source_timestamp_;
  /// Id of the datawriter that sent the sample
  PublicationId            publication_id_;
  /// 0 -  Message encoded using big-endian byte order. (see ace/CDR_Base.h)
  /// 1 -  Message encoded using little-endian byte order.
  bool                     sample_byte_order_;
  /// The data in unspecified format
  ACE_Message_Block*       sample_;
};

} // namespace DCPS
} // namespace OpenDDS

#endif
