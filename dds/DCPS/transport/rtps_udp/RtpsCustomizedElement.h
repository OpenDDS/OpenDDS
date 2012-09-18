/*
 * $Id$
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_RTPSCUSTOMIZEDELEMENT_H
#define OPENDDS_RTPSCUSTOMIZEDELEMENT_H

#include "Rtps_Udp_Export.h"

#include "dds/DCPS/transport/framework/TransportCustomizedElement.h"

#include "dds/DCPS/Dynamic_Cached_Allocator_With_Overflow_T.h"

namespace OpenDDS {
namespace DCPS {

class OpenDDS_Rtps_Udp_Export RtpsCustomizedElement
  : public TransportCustomizedElement {

public:
  static RtpsCustomizedElement* alloc(TransportQueueElement* orig,
                                      ACE_Message_Block* msg,
                                      ACE_Allocator* allocator = 0);

private:
  RtpsCustomizedElement(TransportQueueElement* orig,
                        ACE_Message_Block* msg,
                        ACE_Allocator* allocator);
  virtual ~RtpsCustomizedElement();

  ElementPair fragment(size_t size);
  const ACE_Message_Block* msg_payload() const;
};

typedef Dynamic_Cached_Allocator_With_Overflow<ACE_Thread_Mutex>
  RtpsCustomizedElementAllocator;


} // namespace DCPS
} // namespace OpenDDS

#if defined (__ACE_INLINE__)
#include "RtpsCustomizedElement.inl"
#endif /* __ACE_INLINE__ */

#endif  /* OPENDDS_RTPSCUSTOMIZEDELEMENT_H */
