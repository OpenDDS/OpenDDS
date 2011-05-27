// -*- C++ -*-
//
// $Id$
#include "DCPS/DdsDcps_pch.h" //Only the _pch include should start with DCPS/
#include "DataSampleList.h"
#include "Definitions.h"
#include "PublicationInstance.h"

#include "dds/DCPS/transport/framework/TransportSendListener.h"

#if ! defined (__ACE_INLINE__)
#include "DataSampleList.inl"
#endif /* __ACE_INLINE__ */

namespace TAO
{
  namespace DCPS
  {

    bool
    DataSampleList::dequeue_next_sample (DataSampleListElement* stale)
    {
      if (head_ == 0)
        {
          return false;
        }

      if (stale == head_)
        {
          return dequeue_head_next_sample (stale);
        }

      // Search from head_->next_sample_.
      bool found = false;
      for( DataSampleListElement* item = head_->next_sample_ ;
           item != 0 ;
           item = item->next_sample_ )
        {
          if (item == stale)
            {
              found = true;
              break;
            }
        }

      if (found)
        {
          // Adjust list size.
          -- size_ ;
          
          //
          // Remove from the previous element.
          //
          if( stale->previous_sample_ != 0) 
            {
              // Remove from inside of the list.
              stale->previous_sample_->next_sample_ = stale->next_sample_ ;
            } 
          else 
            {
              // Remove from the head of the list.
              head_ = stale->next_sample_ ;
              if (head_ != 0)
                {
                  head_->previous_sample_ = 0;
                }
            }

          //
          // Remove from the next element.
          //
          if( stale->next_sample_ != 0) 
            {
              // Remove the inside of the list.
              stale->next_sample_->previous_sample_ = stale->previous_sample_ ;
            } 
          else 
            {
              // Remove from the tail of the list.
              tail_ = stale->previous_sample_ ;
            }

          stale->next_sample_ = 0;
          stale->previous_sample_ = 0;
        }

      return found;
    }


    bool
    DataSampleList::dequeue_next_instance_sample (DataSampleListElement* stale)
    {
      if (head_ == 0)
        {
          return false;
        }
      // Same as dequeue from head.
      if (stale == head_)
        {
          return dequeue_head_next_instance_sample (stale);
        }

      // Search from head_->next_instance_sample_.
      bool found = false;
      DataSampleListElement* previous = head_;
      for( DataSampleListElement* item = head_->next_instance_sample_ ;
        item != 0 ;
        item = item->next_instance_sample_ )
        {
          if (item == stale)
            {
              found = true;  
              previous->next_instance_sample_ = stale->next_instance_sample_;
              -- size_ ;

              stale->next_instance_sample_ = 0;

              break;
            }
          previous = item;
        }
      
      return found;
    }


    bool
    DataSampleList::dequeue_next_send_sample (DataSampleListElement* stale)
    {
      if (head_ == 0)
        {
          return false;
        }

      // Same as dequeue from head.
      if (stale == head_)
        {
          return dequeue_head_next_send_sample (stale);
        }

      // Search from head_->next_send_sample_.
      bool found = false;
      for( DataSampleListElement* item = head_->next_send_sample_ ;
        item != 0 ;
        item = item->next_send_sample_ )
        {
          if (item == stale)
            {
              found = true;
              break;
            }
        }
      
      if (found)
        {
          // Adjust size.
          size_ --;
          //
          // Remove from the previous element.
          //
          //stale->previous_sample_->next_sample_ = stale->next_sample_ ;
          //stale->previous_sample_->next_send_sample_ = stale->next_send_sample_ ;
          stale->previous_send_sample_->next_send_sample_ = stale->next_send_sample_ ;

          //
          // Remove from the next element.
          //
          if( stale->next_send_sample_ != 0) 
            {
              // Remove from the inside of the list.
              stale->next_send_sample_->previous_send_sample_ = stale->previous_send_sample_ ;
            } 
          else 
            {
              stale->previous_send_sample_->next_send_sample_ = 0;
              // Remove from the tail of the list.
              tail_ = stale->previous_send_sample_ ;
            }

          stale->next_send_sample_ = 0;
          stale->previous_send_sample_ = 0;
          ////
          //// Remove from the next element.
          ////
          //if( stale->next_sample_ != 0) 
          //  {
          //    // Remove from the inside of the list.
          //    stale->next_sample_->previous_sample_ = stale->previous_sample_ ;
          //  } 
          //else 
          //  {
          //    // Remove from the tail of the list.
          //    tail_ = stale->previous_sample_ ;
          //  }
        }

      return found;
    }


    void
    DataSampleList::enqueue_tail_next_send_sample (DataSampleList list)
     {
       //// Make the appended list linked with next_send_sample_ first.
       //DataSampleListElement* cur = list.head_;

       //if (list.size_ > 1 && cur->next_send_sample_ == 0)
       // {
       //   for (ssize_t i = 0; i < list.size_; i ++)
       //     {
       //       cur->next_send_sample_ = cur->next_sample_;
       //       cur = cur->next_sample_;
       //     }
       // }

       if (head_ == 0)
        {
          head_ = list.head_;
          tail_ = list.tail_;
          size_ = list.size_;
        }
       else 
        {
          tail_->next_send_sample_ 
            //= tail_->next_sample_ 
            = list.head_;
          list.head_->previous_send_sample_ = tail_;
          //list.head_->previous_sample_ = tail_;
          tail_ = list.tail_;
          size_ = size_ + list.size_;
        }
     }

  }
}
