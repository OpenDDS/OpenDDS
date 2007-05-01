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

#ifndef ZEROCOPYINFOSEQ_H
#define ZEROCOPYINFOSEQ_H

#include /**/ "ace/pre.h"

// not needed export for templates #include "dcps_export.h"
#include "dds/DCPS/ZeroCopySeqBase.h"
#include "dds/DCPS/ZeroCopyAllocator_T.h"
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
         * Sequence of ::DDS::SampleInfos supportting zero-copy read/take operations.
         */

        template <size_t ZCS_DEFAULT_SIZE>
        class ZeroCopyInfoSeq : public ZeroCopySeqBase
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
            ZeroCopyInfoSeq(
                const size_t max_len = 0,
                const size_t init_size = ZCS_DEFAULT_SIZE,
                ACE_Allocator* alloc = 0);

            //======== CORBA sequence like methods ======

            /// read reference to the sample at the given index.
            ::DDS::SampleInfo const & operator[](CORBA::ULong i) const;

            /** Write reference to the sample at the given index.
             *
             * @note Should we even allow this?
             * The spec says the DCPS layer will not change the sample's value
             * but it does not restrict the user/application from changing it.
             */
            ::DDS::SampleInfo & operator[](CORBA::ULong i);

            /** get the current length of the sequence.
             */
            CORBA::ULong length() const;

            /** Set the length of the sequence.
             */
            void length(CORBA::ULong length);


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


        private:
            // The default allocator will be very fast for the first
            // allocation but use the standard heap for subsequent allocations
            // such as if the max_size gets bigger.
            FirstTimeFastAllocator< ::DDS::SampleInfo, ZCS_DEFAULT_SIZE> defaultAllocator_;

            //typedef ACE_Array_Base<Sample_T> Ptr_Seq_Type;
            typedef ACE_Vector< ::DDS::SampleInfo, ZCS_DEFAULT_SIZE> Info_Seq_Type;
            Info_Seq_Type info_;

        }; // class ZeroCopyInfoSeq

    } // namespace  ::DDS
} // namespace TAO

#if defined (__ACE_INLINE__)
#include "dds/DCPS/ZeroCopyInfoSeq_T.inl"
#endif /* __ACE_INLINE__ */

#if defined (ACE_TEMPLATES_REQUIRE_SOURCE)
#include "dds/DCPS/ZeroCopyInfoSeq_T.cpp"
#endif /* ACE_TEMPLATES_REQUIRE_SOURCE */

#if defined (ACE_TEMPLATES_REQUIRE_PRAGMA)
#pragma implementation ("ZeroCopyInfoSeq_T.cpp")
#endif /* ACE_TEMPLATES_REQUIRE_PRAGMA */

#include /**/ "ace/post.h"

#endif /* ZEROCOPYINFOSEQ_H  */
