// -*- C++ -*-
// ============================================================================
/**
 *  @file   ZeroCopySeqBase.h
 *
 *  $Id$
 *
 *
 */
// ============================================================================

#ifndef ZEROCOPYSEQBASE_H
#define ZEROCOPYSEQBASE_H

#include /**/ "ace/pre.h"

#include "dcps_export.h"
#include "tao/corbafwd.h"

#if !defined (ACE_LACKS_PRAGMA_ONCE)
# pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

#define DCPS_ZERO_COPY_SEQ_DEFAULT_SIZE 20

namespace TAO
{
    namespace DCPS
    {


        /// common code for sample data and info zero-copy sequences.
        class TAO_DdsDcps_Export ZeroCopySeqBase
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
             * @param alloc The allocator used to allocate the array of pointers
             *              to samples. If zero then use the default allocator.
             *
             */
            ZeroCopySeqBase(const size_t max_len = 0);

            //======== DDS specification inspired methods =====
            /**
             * The maximum length of the sequence.
             */
            CORBA::ULong max_len() const;

            /**
             * Set the maximum length of the sequence.
             */
            void max_len(CORBA::ULong length);


            /**
             * Return the ownership.
             * If true then the samples are copies.
             * If false then the samples are "on loan" and should be returned
             * via a call to xxxDataReader::return_loan.
             */
            bool owns() const;

            /**
             * Set the ownership of the sequence.
             */
            void owns(bool owns);

            //======== CORBA sequence like methods ======

            /**
             * See owns().
             *
             * @note "owns" does not map one-to-one with the
             *   CORBA sequence.  This method may be bogus.
             */
            CORBA::Boolean release() const;


        protected:
            /// current number of samples in the sequence.
            CORBA::ULong length_;

            /// maximum length defined by the user
            CORBA::ULong max_len_;

            /// true if true then this sequence owns the data;
            /// otherwise it is on loan from the DCPS framework.
            bool owns_;

        }; // class ZeroCopySeqBase


    } // namespace  ::DDS
} // namespace TAO

#if defined (__ACE_INLINE__)
#include "dds/DCPS/ZeroCopySeqBase.inl"
#endif /* __ACE_INLINE__ */

#include /**/ "ace/post.h"

#endif /* ZEROCOPYSEQBASE_H  */
