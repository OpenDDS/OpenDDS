// -*- C++ -*-
//
// $Id$
#ifndef TAO_DCPS_QUEUEREMOVEVISITOR_H
#define TAO_DCPS_QUEUEREMOVEVISITOR_H

#include  "BasicQueueVisitor_T.h"
#include  "TransportDefs.h"
#include  "ace/Message_Block.h"


namespace TAO
{

  namespace DCPS
  {

    class TransportQueueElement;

    class QueueRemoveVisitor : public BasicQueueVisitor<TransportQueueElement>
    {
      public:

        /// In order to construct a QueueRemoveVisitor, it must be
        /// provided with the DataSampleListElement* (used as an
        /// identifier) that should be removed from the BasicQueue<T>
        /// (the one this visitor will visit when it is passed-in
        /// to a BasicQueue<T>::accept_remove_visitor() invocation).
        QueueRemoveVisitor(const ACE_Message_Block* sample);

        /// Used to remove all control samples with the specified pub_id.
        QueueRemoveVisitor(RepoId pub_id);

        virtual ~QueueRemoveVisitor();

        /// The BasicQueue<T>::accept_remove_visitor() method will call
        /// this visit_element() method for each element in the queue.
        virtual int visit_element(TransportQueueElement* element,
                                  int&                   remove);

        /// Accessor for the status.  Called after this visitor object has
        /// been passed to BasicQueue<T>::accept_remove_visitor().
        int status() const;

        int removed_bytes() const;


      private:

        /// The sample that needs to be removed.
        const ACE_Message_Block* sample_;

        /// The publisher_id of the control samples to be removed.
        RepoId pub_id_;

        /// Holds the status of our visit.
        int status_;

        int removed_bytes_;
    };

  }

}

#if defined (__ACE_INLINE__)
#include "QueueRemoveVisitor.inl"
#endif /* __ACE_INLINE__ */

#endif  /* TAO_DCPS_QUEUEREMOVEVISITOR_H */
