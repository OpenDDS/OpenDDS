// -*- C++ -*-
//
// $Id$
#ifndef TAO_DCPS_TRANSPORTQUEUEELEMENT_H
#define TAO_DCPS_TRANSPORTQUEUEELEMENT_H

#include  "dds/DCPS/dcps_export.h"
#include  "dds/DCPS/Definitions.h"
#include  "ace/Synch.h"

class ACE_Message_Block;


namespace TAO
{

  namespace DCPS
  {

    struct DataSampleListElement;


    /**
     * @class TransportQueueElement
     *
     * @brief Base wrapper class around a data/control sample to be sent.
     *
     * This class serves as the base class for different types of samples
     * that can be sent.  For example, there are data samples and control
     * samples.  A subclass of TransportQueueElement exists for each of
     * these types of samples.
     *
     * This class maintains a counter that, when decremented to 0, will
     * trigger some logic (defined in the subclass) that will "return
     * the loan" of the sample.  The sample is "loaned" to the transport
     * via a send() or send_control() call on the TransportInterface.
     * This wrapper object will "return the loan" when all DataLinks have
     * "returned" their sub-loans.
     */
    class TAO_DdsDcps_Export TransportQueueElement
    {
      public:

        /// Dtor
        virtual ~TransportQueueElement();

        /// Returns true if the supplied sample matches the wrapped sample.
        bool operator==(const ACE_Message_Block* sample) const;

        /// Invoked when the sample is dropped from a DataLink due to a
        /// remove_sample() call.
        void data_dropped();

        /// Invoked when the sample has been sent by a DataLink.
        void data_delivered();

        /// Does the sample require an exclusive transport packet?
        virtual bool requires_exclusive_packet() const;

        /// Accessor for the publisher id that sent the sample.
        virtual RepoId publication_id() const = 0;

        /// The marshalled sample (sample header + sample data)
        virtual const ACE_Message_Block* msg() const = 0;

        /// Is the element a "control" sample from the specified pub_id?
        virtual bool is_control(RepoId pub_id) const;


      protected:

        /// Ctor.  The initial_count is the number of DataLinks to which
        /// this TransportQueueElement will be sent.
        TransportQueueElement(int initial_count);

        /// Invoked when the counter reaches 0.
        virtual void release_element() = 0;

        /// May be used by subclass' implementation of release_element()
        /// to determine if any DataLinks dropped the data instead of
        /// delivering it.
        bool was_dropped() const;


      private:

        /// Common logic for data_dropped() and data_delivered().
        void decision_made();

        /// Thread lock type
        typedef ACE_SYNCH_MUTEX LockType;

        /// Thread guard type
        typedef ACE_Guard<LockType> GuardType;

        /// Lock for the count_ data member
        LockType lock_;

        /// Counts the number of outstanding sub-loans.
        int count_;

        /// Flag flipped to true if any DataLink dropped the sample.
        bool dropped_;
    };

  }

}

#if defined(__ACE_INLINE__)
#include "TransportQueueElement.inl"
#endif /* __ACE_INLINE__ */

#endif  /* TAO_DCPS_TRANSPORTQUEUEELEMENT_H */
