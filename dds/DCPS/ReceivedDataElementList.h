// -*- C++ -*-
//
// $Id$

#ifndef TAO_DDS_DCPS_RECEIVEDDATAELEMENTLIST_H
#define TAO_DDS_DCPS_RECEIVEDDATAELEMENTLIST_H

#include "dcps_export.h"
#include "dds/DdsDcpsInfrastructureC.h"

#if !defined (ACE_LACKS_PRAGMA_ONCE)
#pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

namespace OpenDDS 
{
  namespace DCPS
  {
    class InstanceState ;

    class OpenDDS_Dcps_Export ReceivedDataElement
    {
    public:
      ReceivedDataElement(void *received_data) :
            registered_data_(received_data),
            sample_state_(::DDS::NOT_READ_SAMPLE_STATE),
            disposed_generation_count_(0),
            no_writers_generation_count_(0),
            ref_count_(0),
            zero_copy_cnt_(0),
            sequence_(0),
            previous_data_sample_(0),
            next_data_sample_(0)
      {
      }

      int dec_ref()
      {
          // since we do not know the type of the sample
          // we let the caller cleanup this object
          // (including the sample) after returning.
          return --this->ref_count_;
      }

      int inc_ref()
      {
          return ++this->ref_count_;
      }

      /// Data sample received
      void *registered_data_;  // ugly, but works....

      /// Sample state for this data sample:
      /// ::DDS::NOT_READ_SAMPLE_STATE/::DDS::READ_SAMPLE_STATE
      ::DDS::SampleStateKind sample_state_ ;

      ///Source time stamp for this data sample
      ::DDS::Time_t source_timestamp_;

      /// The data sample's instance's disposed_generation_count_
      /// at the time the sample was received
      size_t disposed_generation_count_ ;

      /// The data sample's instance's no_writers_generation_count_
      /// at the time the sample was received
      size_t no_writers_generation_count_ ;

      /// Reference count > 1 if it has been loaned
      int ref_count_;

      /// This is needed to know if delete DataReader should fail with
      /// PRECONDITION_NOT_MET because there are outstanding loans.
      int zero_copy_cnt_;

      /// The data sample's sequence number
      ACE_INT16   sequence_ ;

      /// the previous data sample in the ReceivedDataElementList
      ReceivedDataElement *previous_data_sample_ ;


      /// the next data sample in the ReceivedDataElementList
      ReceivedDataElement *next_data_sample_ ;
    } ; // class ReceivedDataElement

    class OpenDDS_Dcps_Export ReceivedDataElementList
    {
    public:
      ReceivedDataElementList(InstanceState *instance_state = 0) ;

      ~ReceivedDataElementList() ;
 
      // adds a data sample to the end of the list
      void add(ReceivedDataElement *data_sample) ;

      bool remove(ReceivedDataElement *data_sample) ;

      ReceivedDataElement *remove_head() ;
      ReceivedDataElement *remove_tail() ;

      /// The first element of the list.
      ReceivedDataElement* head_ ;

      /// The last element of the list.
      ReceivedDataElement* tail_ ;

      /// Number of elements in the list.
      ssize_t                size_ ;

    private:
      InstanceState *instance_state_ ;
    } ; // ReceivedDataElementList
  } // namespace DCPS
} // namespace OpenDDS

#if defined (__ACE_INLINE__)
# include "ReceivedDataElementList.inl"
#endif  /* __ACE_INLINE__ */

#endif /* TAO_DDS_DCPS_RECEIVEDDATAELEMENTLIST_H  */
