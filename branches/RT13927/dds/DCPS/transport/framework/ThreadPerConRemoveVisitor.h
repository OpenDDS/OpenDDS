/*
 * $Id$
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_THREADPERCONREMOVEVISITOR_H
#define OPENDDS_DCPS_THREADPERCONREMOVEVISITOR_H

#include "dds/DCPS/dcps_export.h"
#include "BasicQueueVisitor_T.h"
#include "TransportDefs.h"
#include "ThreadPerConnectionSendTask.h"
#include "ace/Message_Block.h"

namespace OpenDDS {
namespace DCPS {

class OpenDDS_Dcps_Export ThreadPerConRemoveVisitor : public BasicQueueVisitor<SendRequest> {
public:

  /// In order to construct a QueueRemoveVisitor, it must be
  /// provided with the DataSampleListElement* (used as an
  /// identifier) that should be removed from the BasicQueue<T>
  /// (the one this visitor will visit when it is passed-in
  /// to a BasicQueue<T>::accept_remove_visitor() invocation).
  ThreadPerConRemoveVisitor(const ACE_Message_Block* sample);

  virtual ~ThreadPerConRemoveVisitor();

  /// The BasicQueue<T>::accept_remove_visitor() method will call
  /// this visit_element_remove() method for each element in the queue.
  virtual int visit_element_remove(SendRequest*           element,
                                   int&                   remove);

  /// Accessor for the status.  Called after this visitor object has
  /// been passed to BasicQueue<T>::accept_remove_visitor().
  int status() const;

private:

  /// The sample that needs to be removed.
  const ACE_Message_Block* sample_;

  /// Holds the status of our visit.
  int status_;
};

} // namespace DCPS
} // namespace OpenDDS

#if defined (__ACE_INLINE__)
#include "ThreadPerConRemoveVisitor.inl"
#endif /* __ACE_INLINE__ */

#endif  /* OPENDDS_DCPS_THREADPERCONREMOVEVISITOR_H */
