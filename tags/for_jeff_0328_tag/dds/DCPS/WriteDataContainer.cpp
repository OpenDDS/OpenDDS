// -*- C++ -*-
//
// $Id$
#include "DCPS/DdsDcps_pch.h"
#include  "WriteDataContainer.h"
#include  "Service_Participant.h"
#include  "DataSampleList.h"
#include  "DataWriterImpl.h"
#include  "PublicationInstance.h"
#include  "dds/DCPS/transport/framework/TransportSendElement.h"
#include  "tao/debug.h"


namespace TAO
{
  namespace DCPS
  {
    WriteDataContainer::WriteDataContainer(
      CORBA::Long    depth,
      bool           should_block ,
      ACE_Time_Value max_blocking_time,
      size_t         n_chunks
    ) 
    : depth_ (depth),
      should_block_ (should_block),
      max_blocking_time_ (max_blocking_time),
      waiting_on_release_ (false),
      condition_ (lock_),
      n_chunks_ (n_chunks),
      sample_list_element_allocator_(n_chunks_),
      transport_send_element_allocator_(n_chunks_, sizeof (TAO::DCPS::TransportSendElement)),
      shutdown_ (false),
      next_handle_(1)
    {
    }

    
    WriteDataContainer::~WriteDataContainer()
    {
      if (! shutdown_)
        {
          ACE_ERROR((LM_ERROR, 
                     ACE_TEXT("(%P|%t) ERROR: ")
                     ACE_TEXT("WriteDataContainer::~WriteDataContainer, ")
                     ACE_TEXT("The container has not be cleaned.\n")));
        }
    }

    
    // This method preassumes that instance list has space for this sample.
    ::DDS::ReturnCode_t
    WriteDataContainer::enqueue(
        DataSampleListElement* sample,
        ::DDS::InstanceHandle_t       instance_handle)
    {
      // Get the PublicationInstance pointer from InstanceHandle_t.
      PublicationInstance* instance =
            get_handle_instance (instance_handle);
      // Extract the instance queue.
      DataSampleList& instance_list = instance->samples_;        

      //
      // Enqueue to the next_send_sample_ thread of unsent_data_
      // will link samples with the next_sample/previous_sample and 
      // also next_send_sample_. 
      // This would save time when we actually send the data.
      
      unsent_data_.enqueue_tail_next_send_sample (sample);
 
      //
      // Add this sample to the INSTANCE scope list.
      instance_list.enqueue_tail_next_instance_sample (sample);

      return ::DDS::RETCODE_OK;
    }

    ::DDS::ReturnCode_t
    WriteDataContainer::reenqueue_all(DataWriterImpl* writer)
    {
      ACE_GUARD_RETURN (ACE_Recursive_Thread_Mutex, 
                        guard, 
                        this->lock_, 
                        ::DDS::RETCODE_ERROR);

      while (sending_data_.size_ > 0)
        {
          // The remove_sample results a data_dropped call
          // which will move the sample from sending_data_
          // to unsent_data_.
          writer->remove_sample (sending_data_.head_);
        }

      // All sending samples are now in unsent_data_ list.
      // Now move "sent" samples into unsent_data_ list.
      unsent_data_.enqueue_tail_next_send_sample (sent_data_);
      // Clear the sent_data_ list.
      sent_data_.reset ();

      return ::DDS::RETCODE_OK;
    }

    ::DDS::ReturnCode_t
    WriteDataContainer::register_instance(
        ::DDS::InstanceHandle_t&      instance_handle,
        DataSample*&                  registered_sample)
      ACE_THROW_SPEC ((
        CORBA::SystemException
      ))
    {
      PublicationInstance* instance;
     
      if (instance_handle == ::DDS::HANDLE_NIL)
        {
          // registered the instance for the first time.
          ACE_NEW_RETURN (instance,
                          PublicationInstance (registered_sample),
                          ::DDS::RETCODE_ERROR);

          instance_handle = get_next_handle();

          int insert_attempt = instances_.bind(instance_handle, instance);

          if (0 != insert_attempt)
            {
              ACE_ERROR ((LM_ERROR, 
                                 ACE_TEXT("(%P|%t) ERROR: ")
                                 ACE_TEXT("WriteDataContainer::register_instance, ")
                                 ACE_TEXT("failed to insert instance handle=%X\n"), 
                                 instance));
              delete instance;
              return ::DDS::RETCODE_ERROR;
            } // if (0 != insert_attempt)
          instance->instance_handle_ = instance_handle;
        }
      else
        {
          int find_attempt = instances_.find(instance_handle, instance);

          if (0 != find_attempt)
            {
              ACE_ERROR ((LM_ERROR, 
                          ACE_TEXT("(%P|%t) ERROR: ")
                          ACE_TEXT("WriteDataContainer::register_instance, ")
                          ACE_TEXT("The provided instance handle=%X is not a valid"
                          "handle.\n"), instance_handle));
              delete instance;
              return ::DDS::RETCODE_ERROR;
            } // if (0 != insert_attempt)

          // don't need this - the PublicationInstances already has a sample.
          registered_sample->release ();

          instance->unregistered_ = false;
        }

      // The registered_sample is shallow copied.
      registered_sample = instance->registered_sample_->duplicate ();

      return ::DDS::RETCODE_OK;
    }

    ::DDS::ReturnCode_t
    WriteDataContainer::unregister(
        ::DDS::InstanceHandle_t   instance_handle,
        DataSample*&              registered_sample,
        DataWriterImpl*           writer,
        bool                      dup_registered_sample)
      ACE_THROW_SPEC ((
        CORBA::SystemException
      ))
    {
      PublicationInstance* instance = 0;

      int find_attempt = instances_.find(instance_handle, instance);

      if (0 != find_attempt)
        {
          ACE_ERROR_RETURN ((LM_ERROR, 
                             ACE_TEXT("(%P|%t) ERROR: ")
                             ACE_TEXT("WriteDataContainer::unregister, ")
                             ACE_TEXT("The instance(handle=%X) is not registered yet.\n"),
                             instance_handle),
                             ::DDS::RETCODE_PRECONDITION_NOT_MET);
        } // if (0 != insert_attempt)

      instance->unregistered_ = true;

      if (dup_registered_sample)
        {
          // The registered_sample is shallow copied.
          registered_sample = instance->registered_sample_->duplicate ();
        }

      // Unregister the instance with typed DataWriter.
      writer->unregistered (instance_handle);

      return ::DDS::RETCODE_OK;
    }

    ::DDS::ReturnCode_t
    WriteDataContainer::dispose(
        ::DDS::InstanceHandle_t       instance_handle,
        DataSample*&                  registered_sample,
        bool                          dup_registered_sample)
      ACE_THROW_SPEC ((
        CORBA::SystemException
      ))
    {
      PublicationInstance* instance = 0;

      int find_attempt = instances_.find(instance_handle, instance);

      if (0 != find_attempt)
        {
          ACE_ERROR_RETURN ((LM_ERROR, 
                             ACE_TEXT("(%P|%t) ERROR: ")
                             ACE_TEXT("WriteDataContainer::dispose, ")
                             ACE_TEXT("The instance(handle=%X) is not registered yet.\n"),
                             instance_handle),
                             ::DDS::RETCODE_PRECONDITION_NOT_MET);
        }

      if (dup_registered_sample)
        {
          // The registered_sample is shallow copied.
          registered_sample = instance->registered_sample_->duplicate ();
        }

      // Note: The DDS specification is unclear as to if samples in the process
      // of being sent should be removed or not.
      // The advantage of calling remove_sample() on them is that the
      // cached allocator memory for them is freed.  The disadvantage
      // is that the slow reader may see multiple disposes without 
      // any write sample between them and hence not temporarily move into the
      // Alive state.
      // We have choosen to NOT remove the sending samples.

      DataSampleList& instance_list = instance->samples_;

      while (instance_list.size_ > 0)
        {
          bool released = false; 
          ::DDS::ReturnCode_t ret 
            = remove_oldest_sample (instance_list, released);
          ACE_UNUSED_ARG (released);

          if (ret != ::DDS::RETCODE_OK)
            {
              return ret;
            }
        }

      return ::DDS::RETCODE_OK;
    }

    ::DDS::ReturnCode_t 
    WriteDataContainer::num_samples (
        ::DDS::InstanceHandle_t handle, 
        size_t&                 size
      )
    {
      ACE_GUARD_RETURN (ACE_Recursive_Thread_Mutex, 
                        guard, 
                        this->lock_, 
                        ::DDS::RETCODE_ERROR);
      PublicationInstance* instance = 0;

      int find_attempt = instances_.find(handle, instance);

      if (0 != find_attempt)
        {
          return ::DDS::RETCODE_ERROR;
        }
      else
        {
          size = instance->samples_.size_;
          return ::DDS::RETCODE_OK;
        }
    }


    DataSampleList
    WriteDataContainer::get_unsent_data()
    {
      //
      // The samples in unsent_data are added to the sending_data
      // during enqueue.
      //
      DataSampleList list = this->unsent_data_ ;
      
      //
      // The unsent_data_ already linked with the  
      // next_send_sample during enqueue.
      // Append the unsent_data_ to current sending_data_
      // list.
      sending_data_.enqueue_tail_next_send_sample(list);

      //
      // Clear the unsent data list.
      //
      this->unsent_data_.reset ();
      //
      // Return the moved list.
      //
      return list ;
    }


    DataSampleList
    WriteDataContainer::get_sending_data()
    {
      return this->sending_data_ ;
    }


    DataSampleList
    WriteDataContainer::get_sent_data()
    {
      return this->sent_data_ ;
    }


    void
    WriteDataContainer::data_delivered (DataSampleListElement* sample)
    {
      ACE_GUARD (ACE_Recursive_Thread_Mutex, 
                 guard, 
                 this->lock_);
     

      // Delivered samples _must_ be on sending_data_ or released_data_
      // list. If it is not found in one of the lists, an invariant 
      // exception is declared. 

      // The element now needs to be removed from the sending_data_ 
      // list, and appended to the end of the sent_data_ list.

      bool in_release_list = false;
      //
      // Search the sending_data_ list first.
      //
      if (sending_data_.dequeue_next_send_sample (sample))
        {
          // in sending_data_ list
        }
      //
      // Search the released_data_ list last.  Data is moved to this list
      // when it is removed from the sample list, but it is still in used
      // by transport.  We are now been notified by transport, so we can 
      // now release the element in that list (and remove it from the list, 
      // of course).
      //
      else if (released_data_.dequeue_next_sample (sample))
        {
          // We will release it.
          in_release_list = true;
       }
      else  
        {
          // The sample is neither in the sending_data_ nor the 
          // released_data_.
          ACE_ERROR((LM_ERROR, 
                     ACE_TEXT("(%P|%t) ERROR: ")
                     ACE_TEXT("WriteDataContainer::data_delivered, ")
                     ACE_TEXT("The dropped sample is not in sending_data_ and ")
                     ACE_TEXT("released_data_ list.\n")));
        }          
//remove fix this one
      PublicationInstance* instance = sample->handle_;
      
      if (instance->waiting_list_.head_ != 0)
        {
          if (! in_release_list)
            {
              // Remove the delivered sample from the instance sample list
              // and release.
              if (instance->samples_.dequeue_next_instance_sample(sample) == false)
                {
                  ACE_ERROR((LM_ERROR, 
                             ACE_TEXT("(%P|%t) ERROR: ")
                             ACE_TEXT("WriteDataContainer::data_delivered, ")
                             ACE_TEXT("dequeue_next_instance_sample from instance ")
                             ACE_TEXT("list failed\n")));
                }
              release_buffer (sample);
            }
          // Mark the first waiting sample will be next to add to instance
          // list.
          instance->waiting_list_.head_->space_available_ = true;      
          // Remove this waiting sample from waiting list.
          DataSampleListElement* stale = 0;
          if (instance->waiting_list_.dequeue_head_next_instance_sample (stale) == false)
            {
              ACE_ERROR((LM_ERROR, 
                          ACE_TEXT("(%P|%t) ERROR: ")
                          ACE_TEXT("WriteDataContainer::data_delivered, ")
                          ACE_TEXT("dequeue_head_next_instance_sample from waiting ")
                          ACE_TEXT("list failed\n")));
            }
          ACE_UNUSED_ARG (stale);
       }
      else
        {
          if (! in_release_list)
            {
              sent_data_.enqueue_tail_next_sample (sample);
            }
        }
      
      if (in_release_list )
        {          
          release_buffer( sample) ;
        }

      if (waiting_on_release_)
        {
          waiting_on_release_ = false;
          // Broadcast the blocked enqueue threads.
          condition_.broadcast();
        }
    }


    void 
    WriteDataContainer::data_dropped (DataSampleListElement* sample)
    {    
      //In current implementation, the data_dropped call results 
      //from the remove_sample, hence it's called in the same 
      //thread that calls remove_sample which is already guarded.  
      //But we might use this method for the transport to tell lost
      //samples due to lost connection. In that case we need to 
      //acquire lock here. 

      ACE_GUARD (ACE_Recursive_Thread_Mutex, 
                 guard, 
                 this->lock_);

      // This is called by the transport to notify that the data sample
      // was dropped from transport. It is the result of remove_sample.
      
      // The dropped sample is either in released_data_ list or
      // sending_data_ list. Otherwise an exception will be raised.
      //
      // We are now been notified by transport, so we can 
      // now release the sample from released_data_ list and 
      // keep the sample from the sending_data_ list still in
      // sample list since we will send it.

      if (sending_data_.dequeue_next_send_sample (sample) == true)
        {
          // The remove_sample is called from reenqueue_all()
          // which supports the TRANSIENT_LOCAL qos.
          // The samples sending by transport are dropped from
          // transport and will be moved to the unsent list for 
          // resend.
          unsent_data_.enqueue_tail_next_send_sample (sample);
          
        }          
      else if (released_data_.dequeue_next_sample (sample) == true)
        {
          // The remove_sample is requested when sample list size  
          // reaches limit. In this case, the oldest sample is 
          // moved to released_data_ already.
          release_buffer (sample);
        }
      else
        {
          // The sample is neither in not in the 
          // released_data_ list.
          ACE_ERROR((LM_ERROR, 
                     ACE_TEXT("(%P|%t) ERROR: ")
                     ACE_TEXT("WriteDataContainer::data_dropped, ")
                     ACE_TEXT("The dropped sample is not in released_data_ list.\n")));
        }          
    }

    
    ::DDS::ReturnCode_t
    WriteDataContainer::remove_oldest_sample (
      DataSampleList& instance_list, 
      bool& released)
    {
      DataSampleListElement* stale = 0;
      //
      // Remove the oldest sample from the instance list.
      //
      if (instance_list.dequeue_head_next_instance_sample (stale) == false)
        {
          ACE_ERROR_RETURN ((LM_ERROR, 
                              ACE_TEXT("(%P|%t) ERROR: ")
                              ACE_TEXT("WriteDataContainer::remove_oldest_sample, ")
                              ACE_TEXT("dequeue_head_next_instance_sample failed\n")),
                              ::DDS::RETCODE_ERROR);
        }

      //
      // Remove the stale data from the next_sample_ list.  The
      // sending_data_/next_send_sample_ list is not managed within the
      // container, it is only used external to the container and does
      // not need to be managed internally.
      //
      // The next_sample_ link is being used in one of the sent_data_,
      // sending_data_, or unsent_data lists.  Removal from the doubly
      // linked list needs to repair the list only when the stale sample
      // is either the head or tail of the list.
      //

      //
      // Locate the head of the list that the stale data is in.
      //
      DataSampleListElement* head = stale ;
      while( head->previous_sample_ != 0) 
        {
          head = head->previous_sample_ ;
        }

      //
      // Identify the list that the stale data is in.
      // The stale data should be in one of the sent_data_, sending_data_
      // or unsent_data_. It should not be in released_data_ list since 
      // this function is the only place a sample is moved from 
      // sending_data_ to released_data_ list.

      // Remove the element from the internal list.
      bool result = false;

      if( head == this->sending_data_.head_) 
        {
          //
          // Move the element to the released_data_ list since it is still
          // in use, and we need to wait until it is told by the transport.
          //
          result = this->sending_data_.dequeue_next_send_sample (stale);
          released_data_.enqueue_tail_next_sample (stale);
          released = false;
        }
      else if( head == this->sent_data_.head_)         
        {
          //
          // No one is using the data sample, so we can release it back to
          // its allocator.
          //
          result = this->sent_data_.dequeue_next_sample (stale) ;
          release_buffer(stale) ;
          released = true;
        }
      else if( head == this->unsent_data_.head_)
        {
          //
          // No one is using the data sample, so we can release it back to
          // its allocator.
          //
          result = this->unsent_data_.dequeue_next_sample (stale) ;
          release_buffer(stale) ;
          released = true;
        }
      else 
        { 
          ACE_ERROR_RETURN ((LM_ERROR, 
                              ACE_TEXT("(%P|%t) ERROR: ")
                              ACE_TEXT("WriteDataContainer::remove_oldest_sample, ")
                              ACE_TEXT("The oldest sample is not in any internal list.\n")),
                              ::DDS::RETCODE_ERROR);
        }

      if (result == false)
        {
          ACE_ERROR_RETURN ((LM_ERROR, 
                             ACE_TEXT("(%P|%t) ERROR: ")
                             ACE_TEXT("WriteDataContainer::remove_oldest_sample, ")
                             ACE_TEXT("dequeue_next_sample from internal list failed.\n")),
                             ::DDS::RETCODE_ERROR);
        }

      return ::DDS::RETCODE_OK;
    }

    ::DDS::ReturnCode_t
    WriteDataContainer::obtain_buffer (
        DataSampleListElement*& element, 
        ::DDS::InstanceHandle_t handle,
        DataWriterImpl*         writer)
    {
      PublicationInstance* instance = 
            get_handle_instance (handle);

      ACE_NEW_MALLOC_RETURN (element,
                             static_cast<DataSampleListElement*> (
                             sample_list_element_allocator_.malloc (
                             sizeof (DataSampleListElement))),
                             DataSampleListElement (publication_id_,
                                                    writer,
                                                    instance,
                                                    &transport_send_element_allocator_),
                             ::DDS::RETCODE_ERROR);


      // Extract the current instance queue.
      DataSampleList& instance_list = instance->samples_;
      ::DDS::ReturnCode_t ret = ::DDS::RETCODE_OK;

      bool oldest_released = true;
      DataSampleListElement* stale = instance_list.head_;
      // Release the oldest sample if the size reaches the max size of 
      // the sample list.
      if (instance_list.size_ > depth_)
        {
          ACE_ERROR ((LM_ERROR, 
                      ACE_TEXT("(%P|%t) ERROR: ")
                      ACE_TEXT("WriteDataContainer::obtain_buffer, ")
                      ACE_TEXT("The instance list size %d exceeds depth %d\n"),
                      instance_list.size_, depth_));
          ret = ::DDS::RETCODE_ERROR;
        }
      else if (instance_list.size_ == depth_)
        {
          // The remove_oldest_sample removes the oldest sample from 
          // instance list and removes it from the internal lists.
          ret = remove_oldest_sample (instance_list, oldest_released);
        }

      if ( ret != ::DDS::RETCODE_OK)
        {
          release_buffer (element);
          return ret;
        }

      // Each write/enqueue just writes one element and hence the number 
      // of samples will reach the size of sample list at some point. 
      // We need remove enough to accomodate the new element.

      if (should_block_)
        {
          // Need wait when waiting list is not empty or the oldest sample
          // is still being used.
          bool need_wait 
            = instance->waiting_list_.head_ != 0 || ! oldest_released ? true : false;

          if (need_wait) 
            {
              // Add the newly allocated sample to waiting list.
              instance->waiting_list_.enqueue_tail_next_instance_sample (element);

              // wait for all "released" samples to be delivered
              // Timeout value from Qos.RELIBBILITY.max_blocking_time 
              ACE_Time_Value abs_timeout 
                = ACE_OS::gettimeofday () + max_blocking_time_;


              while (! shutdown_ && ACE_OS::gettimeofday () < abs_timeout)
                {
                  waiting_on_release_ = true; // reduces broadcast to only when waiting

                  // lock is released while waiting and aquired before returning from wait.
                  int wait_result = condition_.wait (&abs_timeout);
                  if (wait_result == 0) // signalled
                    {
                      if (element->space_available_ == true)
                        {
                           break;
                        } // else continue wait
                    }
                  else 
                    {
                      // Remove from the waiting list if wait() timed out or return 
                      // other errors.
                      if (instance->waiting_list_.dequeue_next_instance_sample (element) == false)
                        {
                          ACE_ERROR_RETURN ((LM_ERROR, 
                                            ACE_TEXT("(%P|%t) ERROR: ")
                                            ACE_TEXT("WriteDataContainer::obtain_buffer, ")
                                            ACE_TEXT("dequeue_next_instance_sample from waiting ")
                                            ACE_TEXT("list failed\n")),
                                            ::DDS::RETCODE_ERROR);
                        }

                      if (errno == ETIME) 
                        {
                          ret = ::DDS::RETCODE_TIMEOUT;
                        }
                      else
                        {
                          // Other errors from wait.
                          ret = ::DDS::RETCODE_ERROR;
                        }
                      break;
                    }
                }
            }
        }
      else 
        {
          if (! oldest_released)
            {
              // The oldest sample is still being used by the transport, 
              // then we need force the Transport to drop the oldest sample. 
              // The transport will call data_dropped to remove the oldest
              // sample from the released_data_ list.
              writer->remove_sample (stale);
            }
        }

      // Cleanup
      if ( ret != ::DDS::RETCODE_OK)
        {
          release_buffer (element);
        }
      return ret;
    }

    void
    WriteDataContainer::release_buffer (DataSampleListElement* element)
    {
      // Release the memeory to the allocator.
      ACE_DES_FREE (element,
                    sample_list_element_allocator_.free,
                    DataSampleListElement);
    }

    void
    WriteDataContainer::unregister_all (DataWriterImpl* writer)
    {
      shutdown_ = true;
      
      // Tell transport remove all control messages currently
      // transport is processing.
      int result = writer->remove_all_control_msgs ();
      ACE_UNUSED_ARG (result);

      while (sending_data_.size_ > 0)
        {
          // Tell transport remove all samples currently
          // transport is processing.
          writer->remove_sample (sending_data_.head_);
        }
 
      // Broadcast to wake up all waiting threads.
      if (waiting_on_release_)
        {
          condition_.broadcast ();
        }

      ::DDS::ReturnCode_t ret;
      DataSample* registered_sample;
      PublicationInstanceMapType::iterator it = instances_.begin ();
   
      while (it != instances_.end ())
        {
          // Release the instance data.
          ret = dispose ((*it).ext_id_, registered_sample, false); 
          if (ret != ::DDS::RETCODE_OK)
            {
              ACE_ERROR((LM_ERROR, 
                         ACE_TEXT("(%P|%t) ERROR: ")
                         ACE_TEXT("WriteDataContainer::unregister_all, ")
                         ACE_TEXT("dispose instance %X failed\n"), 
                         (*it).ext_id_));
            }

          // Mark the instance unregistered.
          ret = unregister ((*it).ext_id_, registered_sample, writer, false);
          if (ret != ::DDS::RETCODE_OK)
            {
              ACE_ERROR ((LM_ERROR, 
                          ACE_TEXT("(%P|%t) ERROR: ")
                          ACE_TEXT("WriteDataContainer::unregister_all, ")
                          ACE_TEXT("unregister instance %X failed\n"), 
                          (*it).ext_id_));
            }    

          PublicationInstance* instance = (*it).int_id_;

          delete instance;

          // Get the next iterator before erase the instance handle.      
          PublicationInstanceMapType::iterator it_next = it;
          it_next ++;
          // Remove the instance from the instance list.
          instances_.unbind ((*it).ext_id_);
          it = it_next;
        }
      
      ACE_UNUSED_ARG (registered_sample);
    }


    PublicationInstance*
    WriteDataContainer::get_handle_instance (::DDS::InstanceHandle_t handle)
    {
        PublicationInstance* instance = 0;
        if (0 != instances_.find(handle, instance))
          {
            ACE_ERROR ((LM_ERROR, 
                        ACE_TEXT("(%P|%t) ERROR: ")
                        ACE_TEXT("WriteDataContainer::get_handle_instance, ")
                        ACE_TEXT("lookup for %d failed\n"), 
                        handle));
          }

        return instance;
    }


    ::DDS::InstanceHandle_t
    WriteDataContainer::get_next_handle ()
    {
      ACE_GUARD_RETURN (ACE_Recursive_Thread_Mutex, 
                        guard, 
                        this->lock_,
                        0);
      return next_handle_++;
    }

  } // namespace TAO
} // namespace DCPS


