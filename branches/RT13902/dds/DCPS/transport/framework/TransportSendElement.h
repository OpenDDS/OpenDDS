/*
 * $Id$
 *
 * Copyright 2010 Object Computing, Inc.
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_TRANSPORTSENDELEMENT_H
#define OPENDDS_DCPS_TRANSPORTSENDELEMENT_H

#include "dds/DCPS/dcps_export.h"
#include "TransportQueueElement.h"
#include "dds/DCPS/DataSampleList.h"

namespace OpenDDS {
namespace DCPS {

class OpenDDS_Dcps_Export TransportSendElement : public TransportQueueElement {
public:

  TransportSendElement(int initial_count,
                       const DataSampleListElement* sample,
                       TransportSendElementAllocator* allocator = 0);
  virtual ~TransportSendElement();

  /// Accessor for the publisher id.
  virtual RepoId publication_id() const;

  /// Accessor for the ACE_Message_Block
  virtual const ACE_Message_Block* msg() const;

protected:

  virtual void release_element(bool dropped_by_transport);

private:

  /// This is the actual element that the transport framework was
  /// asked to send.
  const DataSampleListElement* element_;

  /// Reference to TransportSendElement allocator.
  TransportSendElementAllocator* allocator_;
};

} // namespace DCPS
} // namespace OpenDDS

#if defined (__ACE_INLINE__)
#include "TransportSendElement.inl"
#endif /* __ACE_INLINE__ */

#endif  /* OPENDDS_DCPS_TRANSPORTSENDELEMENT_H */
