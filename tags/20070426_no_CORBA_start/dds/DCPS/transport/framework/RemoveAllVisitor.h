// -*- C++ -*-
//
// $Id$
#ifndef TAO_DCPS_REMOVEALLVISITOR_H
#define TAO_DCPS_REMOVEALLVISITOR_H

#include "dds/DCPS/dcps_export.h"
#include "BasicQueueVisitor_T.h"


namespace TAO
{

  namespace DCPS
  {

    class TransportQueueElement;

    class TAO_DdsDcps_Export RemoveAllVisitor : public BasicQueueVisitor<TransportQueueElement>
    {
      public:

        RemoveAllVisitor();

        virtual ~RemoveAllVisitor();

        // The using declaration is added to resolve the "hides virtual functions"
        // compilation warnings on Solaris.
        using BasicQueueVisitor<TransportQueueElement>::visit_element;

        /// The BasicQueue<T>::accept_remove_visitor() method will call
        /// this visit_element() method for each element in the queue.
        virtual int visit_element(TransportQueueElement* element,
                                  int&                   remove);

        /// Accessor for the status.  Called after this visitor object has
        /// been passed to BasicQueue<T>::accept_remove_visitor().
        int status() const;

        int removed_bytes() const;


      private:

        /// Holds the status of our visit.
        int status_;

        int removed_bytes_;
    };

  }

}

#if defined (__ACE_INLINE__)
#include "RemoveAllVisitor.inl"
#endif /* __ACE_INLINE__ */

#endif  /* TAO_DCPS_REMOVEALLVISITOR_H */
