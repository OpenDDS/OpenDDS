// -*- C++ -*-
// ============================================================================
/**
 *  @file   ZeroCopySeq_T.h
 *
 *  $Id$
 *
 *
 */
// ============================================================================

#ifndef ZEROCOPYSEQ_H
#define ZEROCOPYSEQ_H

#include /**/ "ace/pre.h"

// not needed export for templates #include "dcps_export.h"
#include "dds/DCPS/ZeroCopySeqBase.h"
#include "dds/DCPS/ZeroCopyAllocator_T.h"
#include "dds/DCPS/ReceivedDataElementList.h"
#include <ace/Vector_T.h>

#if !defined (ACE_LACKS_PRAGMA_ONCE)
# pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

#define DCPS_ZERO_COPY_SEQ_DEFAULT_SIZE 10

namespace TAO
{
    namespace DCPS
    {

        /**
        * Provides [] operators returning sample references
        *     like the the CORBA sequence used by the normal
        *     take and read operations.  But it is is implemented as
        *     an "array" of pointers to the samples so they can be
        *     "loaned" to the application code.
        *
        * @NOTE:  I did not inherit from ACE_Vector or ACE_Array_base because
        * I needed to override the [] operator and I did not
        * want to confuse things by having the inherited methods/operators
        * return pointers while the overridden methods/operators 
        * returned a reference to the Sample.
        *
        * @note: this class should have a similar interface to TAO's
        * CORBA sequence implementation but allow [] operator to
        * return a reference to Sample_T instead of the pointer.
        *
        * @note: Also have an interface as described in the DDS specification.
        *
        * @note Open issues:
        * 1) How to support spec saying: "To avoid potential memory leaks, 
        *    the implementation of the Data and SampleInfo collections should 
        *    disallow changing the length of a collection for which owns==FALSE."
        * 2) What does spec mean by: "Furthermore, deleting a collection for which 
        *    owns==FALSE should be considered an error."
        *
        */
        template <class Sample_T, size_t ZCS_DEFAULT_SIZE>
        class ZeroCopyDataSeq : public ZeroCopySeqBase
        {
        public:

            /**
             * Construct a sequence of sample data values that is supports
             * zero-copy reads.
             *
             * @param max_len Maximum number of samples to insert into the sequence.
             *                If == 0 then use zero-copy reading.
             *                Defaults to zero hence supporting zero-copy reads/takes.
             *
             * @param init_size Initial size of the sequence.
             *
             * @param alloc The allocator used to allocate the array of pointers
             *              to samples. If zero then use the default allocator.
             *
             */
            ZeroCopyDataSeq(
                const size_t max_len = 0,
                const size_t init_size = ZCS_DEFAULT_SIZE,
                ACE_Allocator* alloc = 0);


            ~ZeroCopyDataSeq();

            //======== DDS specification inspired methods =====
            /** get the current length of the sequence.
             */
            CORBA::ULong length() const;

            /** Set the length of the sequence. (for user code)
             */
            void length(CORBA::ULong length);

            //======== CORBA sequence like methods ======

            /// read reference to the sample at the given index.
            Sample_T const & operator[](CORBA::ULong i) const;

            /** Write reference to the sample at the given index.
             *
             * @note Should we even allow this?
             * The spec says the DCPS layer will not change the sample's value
             * but it does not restrict the user/application from changing it.
             */
            Sample_T & operator[](CORBA::ULong i);

            /**
             * The CORBA sequence like version of max_len();
             */
            CORBA::ULong maximum() const;

            /**
             * Current allocated number of sample slots.
             * 
             * @note The DDS specification's use of max_len=0 to designate 
             *       zero-copy read request requires some
             *       way of knowing the internally allocated slots 
             *       for sample pointers that is not "max_len".
             */
            CORBA::ULong max_slots() const;

            // TBD: ?support replace, get_buffer, allocbuf 
            //       and freebuf 

            // ==== OpenDDS internal methods =====
            //!!!!!! User code should not use this methods.

            // ?TBD - make this operator not available to the DDS user.
            void assign_ptr(::CORBA::ULong ii, 
                            ::TAO::DCPS::ReceivedDataElement* item,
                            ::TAO::DCPS::DataReaderImpl* loaner);

            TAO::DCPS::ReceivedDataElement* getPtr(::CORBA::ULong ii) const;

            void assign_sample(::CORBA::ULong ii, const Sample_T& sample);

            /**
             * This method is like length(len) but is for 
             * internal DCPS layer use to avoid the resizing when owns=false safty check.
             */
            void set_length(CORBA::ULong length);

        private:

            /// true if this sequence is supporting zero-copy reads/takes
            bool is_zero_copy_;

            ::TAO::DCPS::DataReaderImpl* loaner_;

            // The default allocator will be very fast for the first
            // allocation but use the standard heap for subsequent allocations
            // such as if the max_size gets bigger.
            FirstTimeFastAllocator<Sample_T*, ZCS_DEFAULT_SIZE> defaultAllocator_;

            //typedef ACE_Array_Base<Sample_T> Ptr_Seq_Type;
            typedef ACE_Vector<TAO::DCPS::ReceivedDataElement*, ZCS_DEFAULT_SIZE> Ptr_Seq_Type;
            Ptr_Seq_Type ptrs_;

            // Note: use default size of zero but constructor may override that
            //   It is overriden if max_len > 0.
            typedef ACE_Vector<Sample_T, 0> Sample_Seq_Type;
            Sample_Seq_Type samples_;
        }; // class ZeroCopyDataSeq

    } // namespace  ::DDS
} // namespace TAO

#if defined (__ACE_INLINE__)
#include "dds/DCPS/ZeroCopySeq_T.inl"
#endif /* __ACE_INLINE__ */

#if defined (ACE_TEMPLATES_REQUIRE_SOURCE)
#include "dds/DCPS/ZeroCopySeq_T.cpp"
#endif /* ACE_TEMPLATES_REQUIRE_SOURCE */

#if defined (ACE_TEMPLATES_REQUIRE_PRAGMA)
#pragma implementation ("ZeroCopySeq_T.cpp")
#endif /* ACE_TEMPLATES_REQUIRE_PRAGMA */

#include /**/ "ace/post.h"

#endif /* ZEROCOPYSEQ_H  */
