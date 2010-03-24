/*
 * $Id$
 *
 * Copyright 2010 Object Computing, Inc.
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_QUEUEREMOVEVISITOR_H
#define OPENDDS_DCPS_QUEUEREMOVEVISITOR_H

#include "dds/DCPS/dcps_export.h"
#include "dds/DCPS/GuidUtils.h"
#include "BasicQueueVisitor_T.h"
#include "TransportDefs.h"
#include "ace/Message_Block.h"

namespace OpenDDS {
namespace DCPS {

class TransportQueueElement;

class OpenDDS_Dcps_Export QueueRemoveVisitor : public BasicQueueVisitor<TransportQueueElement> {
public:

  /// Construct with a queue element representing the sample to be removed.
  QueueRemoveVisitor(TransportQueueElement& sample);

  virtual ~QueueRemoveVisitor();

  /// The BasicQueue<T>::accept_remove_visitor() method will call
  /// this visit_element_remove() method for each element in the queue.
  virtual int visit_element_remove(TransportQueueElement* element,
                                   int&                   remove);

  /// Accessor for the status.  Called after this visitor object has
  /// been passed to BasicQueue<T>::accept_remove_visitor().
  int status() const;

  int removed_bytes() const;

private:

  /// The sample that needs to be removed.
  TransportQueueElement& sample_;

  /// Holds the status of our visit.
  int status_;

  int removed_bytes_;
};

} // namespace DCPS
} // namespace OpenDDS

#if defined (__ACE_INLINE__)
#include "QueueRemoveVisitor.inl"
#endif /* __ACE_INLINE__ */

#endif  /* OPENDDS_DCPS_QUEUEREMOVEVISITOR_H */
