/*
 * $Id$
 *
 * Copyright 2010 Object Computing, Inc.
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_TRANSPORTSENDCONTROLELEMENT_H
#define OPENDDS_DCPS_TRANSPORTSENDCONTROLELEMENT_H

#include "dds/DCPS/dcps_export.h"
#include "dds/DCPS/GuidUtils.h"
#include "TransportDefs.h"
#include "TransportQueueElement.h"

class ACE_Message_Block ;

namespace OpenDDS {
namespace DCPS {

class TransportSendListener;

class TransportSendControlElement;

typedef Cached_Allocator_With_Overflow<TransportSendControlElement, ACE_SYNCH_NULL_MUTEX>
TransportSendControlElementAllocator;

class OpenDDS_Dcps_Export TransportSendControlElement : public TransportQueueElement {
public:

  TransportSendControlElement(int                    initial_count,
                              RepoId                 publisher_id,
                              TransportSendListener* listener,
                              ACE_Message_Block*     msg_block,
                              TransportSendControlElementAllocator* allocator = 0);

  virtual ~TransportSendControlElement();

  /// Overriden to always return true for Send Control elements.
  virtual bool requires_exclusive_packet() const;

  /// Accessor for the publisher id.
  virtual RepoId publication_id() const;

  /// Accessor for the ACE_Message_Block
  virtual const ACE_Message_Block* msg() const;

  /// Is the element a "control" sample from the specified pub_id?
  virtual bool is_control(RepoId pub_id) const;

protected:

  virtual void release_element(bool dropped_by_transport);

private:

  /// The publisher of the control message
  RepoId publisher_id_;

  /// The TransportSendListener object to call back upon.
  TransportSendListener* listener_;

  /// The control message.
  ACE_Message_Block* msg_;

  /// Reference to the TransportSendControlElement
  /// allocator.
  TransportSendControlElementAllocator* allocator_;
};

} // namespace DCPS
} // namespace OpenDDS

#if defined (__ACE_INLINE__)
#include "TransportSendControlElement.inl"
#endif /* __ACE_INLINE__ */

#endif  /* OPENDDS_DCPS_TRANSPORTSENDCONTROLELEMENT_H */
