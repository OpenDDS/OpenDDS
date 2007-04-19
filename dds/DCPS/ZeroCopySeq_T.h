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

#include <ace/Vector_T.h>

#define DCPS_ZERO_COPY_SEQ_DEFAULT_SIZE 10

namespace TAO
{
    namespace DCPS
    {

        template<class T, std::size_t N>
            // This allocator is "Fast" because it's pool can be on the stack (if the object is on the stack 
            //               and hence it does not require the cost of allocating and deallocating on the heap.
            //!!!! The object using this allocator must not have a scope smaller than this object.
        class FirstTimeFastAllocator : public ACE_Allocator
        {
        public:
            FirstTimeFastAllocator() : firstTime_(1) {};
            virtual void *malloc (size_t nbytes) { 
                if (firstTime_ && nbytes <= N * sizeof(T)) {
                    firstTime_ = 0;
                    return (void*) pool_;
                }
                else {
                    return ACE_OS::malloc(nbytes);
                }
            };
            virtual void free (void *ptr) {
                if (ptr != (void*) pool_) {
                    ACE_OS::free(ptr);
                }
            };

            /// These methods are no-ops.
            virtual void *calloc (size_t nbytes, char initial_value = '\0') 
            {/* no-op */ 
              ACE_UNUSED_ARG (nbytes);
              ACE_UNUSED_ARG (initial_value);
              return (void*)0;
            };
            virtual void *calloc (size_t n_elem, size_t elem_size, char initial_value = '\0')
            {/* no-op */ 
              ACE_UNUSED_ARG (n_elem);
              ACE_UNUSED_ARG (elem_size);
              ACE_UNUSED_ARG (initial_value);
              return (void*)0;
            };

            virtual int remove (void)
            {/* no-op */ 
              return -1; 
            };
            virtual int bind (const char *name, void *pointer, int duplicates = 0)
            {/* no-op */ 
              ACE_UNUSED_ARG (name);
              ACE_UNUSED_ARG (pointer);
              ACE_UNUSED_ARG (duplicates);
              return -1; 
            };
            virtual int trybind (const char *name, void *&pointer)
            {/* no-op */ 
              ACE_UNUSED_ARG (name);
              ACE_UNUSED_ARG (pointer);
              return -1; 
            };
            virtual int find (const char *name, void *&pointer)
            {/* no-op */ 
              ACE_UNUSED_ARG (name);
              ACE_UNUSED_ARG (pointer);
              return -1; 
            };
            virtual int find (const char *name)
            {/* no-op */ 
              ACE_UNUSED_ARG (name);
              return -1; 
            };
            virtual int unbind (const char *name)
            {/* no-op */ 
              ACE_UNUSED_ARG (name);
              return -1; 
            };
            virtual int unbind (const char *name, void *&pointer)
            {/* no-op */ 
              ACE_UNUSED_ARG (name);
              ACE_UNUSED_ARG (pointer);
              return -1; 
            };
            virtual int sync (ssize_t len = -1, int flags = MS_SYNC)
            {/* no-op */ 
              ACE_UNUSED_ARG (len);
              ACE_UNUSED_ARG (flags);
              return -1; 
            };
            virtual int sync (void *addr, size_t len, int flags = MS_SYNC)
            {/* no-op */ 
              ACE_UNUSED_ARG (addr);
              ACE_UNUSED_ARG (len);
              ACE_UNUSED_ARG (flags);
              return -1; 
            };
            virtual int protect (ssize_t len = -1, int prot = PROT_RDWR)
            {/* no-op */ 
              ACE_UNUSED_ARG (len);
              ACE_UNUSED_ARG (prot);
              return -1; 
            };
            virtual int protect (void *addr, size_t len, int prot = PROT_RDWR)
            {/* no-op */ 
              ACE_UNUSED_ARG (addr);
              ACE_UNUSED_ARG (len);
              ACE_UNUSED_ARG (prot);
              return -1; 
            };
#if defined (ACE_HAS_MALLOC_STATS)
            virtual void print_stats (void) const
            {/* no-op */ };
#endif /* ACE_HAS_MALLOC_STATS */
            virtual void dump (void) const
            {/* no-op */ };

        private:
            // do not allow copies.
            FirstTimeFastAllocator(const FirstTimeFastAllocator&);
            FirstTimeFastAllocator& operator=(const FirstTimeFastAllocator&);

            int firstTime_;
            T pool_[N];
        };


        /// common code for sample data and info zero-copy sequences.
        class ZeroCopySeqBase
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
            ZeroCopySeqBase(
                const size_t max_len = 0)
                : length_(0)
                , max_len_(max_len)
                , owns_(max_len ? true : false)
            {};

            //======== DDS specification inspired methods =====
            /**
             * The maximum length of the sequence.
             */
            inline CORBA::ULong max_len() const {
                return this->max_len_;
            };

            /**
             * Set the maximum length of the sequence.
             */
            inline void max_len(CORBA::ULong length) {
                this->max_len_ = length;
            };


            /**
             * Return the ownership.
             * If true then the samples are copies.
             * If false then the samples are "on loan" and should be returned
             * via a call to xxxDataReader::return_loan.
             */
            inline bool owns() const {
                return this->owns_;
            };

            /**
             * Set the ownership of the sequence.
             */
            inline void owns(bool owns) {
                this->owns_ = owns;
            };

            //======== CORBA sequence like methods ======

            /**
             * See owns().
             *
             * @note "owns" does not map one-to-one with the
             *   CORBA sequence.  This method may be bogus.
             */
            inline CORBA::Boolean release() const {
                return this->owns_;
            }


        protected:
            /// current number of samples in the sequence.
            CORBA::ULong length_;

            /// maximum length defined by the user
            CORBA::ULong max_len_;

            /// true if true then this sequence owns the data;
            /// otherwise it is on loan from the DCPS framework.
            bool owns_;

        }; // class ZeroCopySeqBase

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
             * @note The order of the constructor parameters allows for the best
             *       default behavior when switching between a normal sequence 
             *       and a zero-copy read sequence.
             *
             */
            ZeroCopyDataSeq(
                const size_t init_size = ZCS_DEFAULT_SIZE,
                const size_t max_len = 0,
                ACE_Allocator* alloc = 0) 
                : ZeroCopySeqBase(max_len)
                , ptrs_(init_size, alloc ? alloc : &defaultAllocator_)
            {};

            //======== DDS specification inspired methods =====
            /** get the current length of the sequence.
             */
            inline CORBA::ULong length() const {
                return this->length_;
            };

            /** Set the length of the sequence.
             */
            inline void length(CORBA::ULong length) {
                // TBD - how to support spec saying:
                // To avoid potential memory leaks, the implementation of the Data and SampleInfo 
                // collections should disallow changing the length of a collection for which owns==FALSE.

                // NOTE: +20 helps avoid copying every time the Seq length is incremented
                // and we know that they are pointers so 20 more is no big deal
                CORBA::ULong nextLen = length+20;
                if (this->max_len_)
                {
                    if (nextLen > this->max_len_) {
                        nextLen = length;
                    }
                    if (nextLen > this->max_len_) {
                        ACE_ERROR((LM_ERROR,"ZeroCopySeq::length(%d) setting length > max_len %d -- no allowed.\n", 
                            length, this->max_len_));
                        // TBD - it would be nice to tell the calling code that this did not work.
                        return;  
                    }
                }
                if (this->length_ > ptrs_.size()) {
                    this->ptrs_.resize(nextLen, (Sample_T*)0); 
                }
                this->length_ = length;
            };

            //======== CORBA sequence like methods ======

            /// read reference to the sample at the given index.
            inline Sample_T const & operator[](CORBA::ULong i) const {
                return *(this->ptrs_[i]);
            };

            /** Write reference to the sample at the given index.
             *
             * @note Should we even allow this?
             * The spec says the DCPS layer will not change the sample's value
             * but it does not restrict the user/application from changing it.
             */
            inline Sample_T & operator[](CORBA::ULong i) {
                return *(this->ptrs_[i]);
            };

            /**
             * Current allocated number of sample slots.
             * 
             * @note The DDS specification's use of max_len=0 to designate 
             *       zero-copy read request requires some
             *       way of knowing the internally allocated slots 
             *       for sample pointers that is not "max_len".
             */
            inline CORBA::ULong maximum() const {
                return this->ptrs_.max_size();
            };

            // Note: We are not supporting replace, get_buffer, allocbuf 
            //       and freebuf because they do not make sense for a zero-copy
            //       sequence.

            // ==== OpenDDS unique methods =====

            // TBD - make this operator not available to the DDS user.
            void assignPtr(::CORBA::ULong ii, Sample_T* ptr) {
                ptrs_[ii] = ptr;
            }

        private:
            // The default allocator will be very fast for the first
            // allocation but use the standard heap for subsequent allocations
            // such as if the max_size gets bigger.
            FirstTimeFastAllocator<Sample_T*, ZCS_DEFAULT_SIZE> defaultAllocator_;

            //typedef ACE_Array_Base<Sample_T> Ptr_Seq_Type;
            typedef ACE_Vector<Sample_T*, ZCS_DEFAULT_SIZE> Ptr_Seq_Type;
            Ptr_Seq_Type ptrs_;

        }; // class ZeroCopyDataSeq

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
             * @note The order of the constructor parameters allows for the best
             *       default behavior when switching between a normal sequence 
             *       and a zero-copy read sequence.
             *

             */
            ZeroCopyInfoSeq(
                const size_t init_size = ZCS_DEFAULT_SIZE,
                const size_t max_len = 0,
                ACE_Allocator* alloc = 0) 
                : ZeroCopySeqBase(max_len)
                , info_(init_size, alloc ? alloc : &defaultAllocator_)
            {};

            //======== CORBA sequence like methods ======

            /// read reference to the sample at the given index.
            inline ::DDS::SampleInfo const & operator[](CORBA::ULong i) const {
                return this->info_[i];
            };

            /** Write reference to the sample at the given index.
             *
             * @note Should we even allow this?
             * The spec says the DCPS layer will not change the sample's value
             * but it does not restrict the user/application from changing it.
             */
            inline ::DDS::SampleInfo & operator[](CORBA::ULong i) {
                return this->info_[i];
            };

            /** get the current length of the sequence.
             */
            inline CORBA::ULong length() const {
                return this->length_;
            };

            /** Set the length of the sequence.
             */
            inline void length(CORBA::ULong length) {
                // TBD - how to support spec saying:
                // To avoid potential memory leaks, the implementation of the Data and SampleInfo 
                // collections should disallow changing the length of a collection for which owns==FALSE.

                // NOTE: +20 helps avoid copying every time the Seq length is incremented
                // and we know that they are pointers so 20 more is no big deal
                CORBA::ULong nextLen = length+20;
                if (this->max_len_)
                {
                    if (nextLen > this->max_len_) {
                        nextLen = length;
                    }
                    if (nextLen > this->max_len_) {
                        ACE_ERROR((LM_ERROR,"ZeroCopySeq::length(%d) setting length > max_len %d -- no allowed.\n", 
                            length, this->max_len_));
                        // TBD - it would be nice to tell the calling code that this did not work.
                        return;  
                    }
                }
                if (this->length_ > info_.size()) {
                    // TBD - is using the default constructor OK?
                    this->info_.resize(nextLen, ::DDS::SampleInfo()); 
                }
                this->length_ = length;
            };

            /**
             * Current allocated number of sample slots.
             * 
             * @note The DDS specification's use of max_len=0 to designate 
             *       zero-copy read request requires some
             *       way of knowing the internally allocated slots 
             *       for sample pointers that is not "max_len".
             */
            inline CORBA::ULong maximum() const {
                return this->info_.max_size();
            };


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

#endif /* ZEROCOPYSEQ_H  */
