
namespace TAO
{
  namespace DCPS
  {
    ACE_INLINE
    DataSampleListElement::DataSampleListElement (
      PublicationId           publication_id,
      TransportSendListener*  send_listner,
      PublicationInstance*    handle,
      TransportSendElementAllocator* allocator)
      : sample_ (0),
        publication_id_ (publication_id), 
        group_id_ (0),
        previous_sample_ (0),
        next_sample_ (0),
        next_instance_sample_ (0),
        next_send_sample_ (0),
        send_listener_ (send_listner),
        space_available_ (false),
        handle_(handle),
        transport_send_element_allocator_(allocator)
    {
    }

    ACE_INLINE
    DataSampleListElement::~DataSampleListElement ()
    {
      if (sample_ != 0)
        {
          sample_->release ();
        }
    }
 
    ACE_INLINE
    DataSampleList::DataSampleList() 
      : head_( 0), 
        tail_( 0), 
        size_( 0) 
    { 
    }

    ACE_INLINE
    void DataSampleList::reset ()
    {
      head_ = tail_ = 0;
      size_ = 0;
    }

    ACE_INLINE
    void
    DataSampleList::enqueue_tail_next_sample (DataSampleListElement* sample)
    {
      sample->previous_sample_ = 0;
      sample->next_sample_ = 0;
      
      ++size_ ;
      if( head_ == 0) 
        {
          // First sample in the list.
          head_ = tail_ = sample ;
        } 
      else 
        {
          // Add to existing list.
          tail_->next_sample_ = sample ;
          sample->previous_sample_ = tail_;
          tail_ = sample;
        }    
    }

    ACE_INLINE
    bool
    DataSampleList::dequeue_head_next_sample (DataSampleListElement*& stale)
    {
      //
      // Remove the oldest sample from the list.
      //
      stale = head_;

      if (head_ == 0)
        {
          return false;
        }
      else
        {
          --size_ ;
          head_ = head_->next_sample_;

          if (head_ == 0)
            {
              tail_ = 0;
            }
          else 
            {
              head_->previous_sample_ = 0;
            }

          return true;
        }
    }

    ACE_INLINE
    void
    DataSampleList::enqueue_tail_next_send_sample (DataSampleListElement* sample)
    {
      sample->previous_sample_ = 0;
      sample->next_sample_ = 0;
      sample->next_send_sample_ = 0;

      ++ size_ ;
      if(head_ == 0) 
        {
          // First sample in list.
          head_ = tail_ = sample ;
        } 
      else 
        {
          // Add to existing list.
          sample->previous_sample_ = tail_;
          tail_->next_sample_ = sample;
          tail_->next_send_sample_ = sample ;
          tail_ = sample ;
        }
    }

    ACE_INLINE
    bool
    DataSampleList::dequeue_head_next_send_sample (DataSampleListElement*& stale)
    {
      //
      // Remove the oldest sample from the instance list.
      //
      stale = head_;

      if (head_ == 0)
        {
          return false;
        }
      else
        {
          --size_ ;
          head_ = head_->next_send_sample_ ;
          if (head_ == 0)
            {
              tail_ = 0;
            }
          else 
            {
              head_->previous_sample_ = 0;
            }
          return true;
        }
    }

    ACE_INLINE
    void
    DataSampleList::enqueue_tail_next_instance_sample (DataSampleListElement* sample)
    {
      sample->next_instance_sample_ = 0;

      ++ size_ ;
      if(head_ == 0) 
        {
          // First sample on queue.
          head_ = tail_ = sample ;
        } 
      else 
        {
          // Another sample on an existing queue.
          tail_->next_instance_sample_ = sample ;
          tail_ = sample ;
        }
    }

    ACE_INLINE
    bool
    DataSampleList::dequeue_head_next_instance_sample (DataSampleListElement*& stale)
    {
      //
      // Remove the oldest sample from the instance list.
      //
      stale = head_;

      if (head_ == 0)
        { 
          // try to dequeue empty instance list.
          return false;
        }
      else 
        {
          --size_ ;
          head_ = head_->next_instance_sample_ ;
          if (head_ == 0)
            {
              tail_ = 0;
            }
          return true;
        }
    }

    ACE_INLINE
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
        }

      return found;
    }

    ACE_INLINE
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
              break;
            }
          previous = item;
        }
      
      return found;
    }

    ACE_INLINE
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
          stale->previous_sample_->next_sample_ = stale->next_sample_ ;
          stale->previous_sample_->next_send_sample_ = stale->next_send_sample_ ;

          //
          // Remove from the next element.
          //
          if( stale->next_sample_ != 0) 
            {
              // Remove from the inside of the list.
              stale->next_sample_->previous_sample_ = stale->previous_sample_ ;
            } 
          else 
            {
              // Remove from the tail of the list.
              tail_ = stale->previous_sample_ ;
            }
        }

      return found;
    }


    ACE_INLINE
    void
    DataSampleList::enqueue_tail_next_send_sample (DataSampleList list)
     {
       // Make the appended list linked with next_send_sample_ first.
       DataSampleListElement* cur = list.head_;

       if (list.size_ > 1 && cur->next_send_sample_ == 0)
        {
          for (ssize_t i = 0; i < list.size_; i ++)
            {
              cur->next_send_sample_ = cur->next_sample_;
              cur = cur->next_sample_;
            }
        }

       if (head_ == 0)
        {
          head_ = list.head_;
          tail_ = list.tail_;
          size_ = list.size_;
        }
       else 
        {
          tail_->next_send_sample_ 
            = tail_->next_sample_ 
            = list.head_;
          list.head_->previous_sample_ = tail_;
          tail_ = list.tail_;
          size_ = size_ + list.size_;
        }
     }
  }  /* namespace DCPS */
}  /* namespace TAO */

