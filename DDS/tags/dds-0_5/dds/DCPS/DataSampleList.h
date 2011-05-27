/// -*- C++ -*-
///
/// $Id$
#ifndef TAO_DCPS_DATASAMPLELIST_H
#define TAO_DCPS_DATASAMPLELIST_H

#include  "dds/DdsDcpsInfoUtilsC.h"
#include  "Definitions.h"

namespace TAO
{

  namespace DCPS
  {
  
    class TransportSendListener;
    struct PublicationInstance;

    /**
    * Currently we contain entire messages in a single ACE_Message_Block
    * chain.
    */
    typedef ACE_Message_Block DataSample ;

    /**
    * List elements include the marshaled message, the publication Id and
    * Instance handle for downstream processing, and three separate threads
    * allowing the elements to reside simultaneously on four different
    * lists of data:
    *
    *   next_sample_ / previous_sample
    *     - the next sample of data in the DataWriter.  This thread is used
    *       to traverse elements in the order in which they were written to
    *       the DataWriter (within PRESENTATION.access_scope==TOPIC).
    *       This thread will be used to hold the element in one of four
    *       different lists in the WriteDataContainer: the unsent_data_
    *       list, the sending_data_ list, the sent_data_ list, or the 
    *       released_data_ list at different times during its lifetime.  
    *       This has a double link to allow removal of elements from internal
    *       locations in the list.
    *
    *   next_instance_sample_
    *     - the next sample of data in the instance within the DataWriter.
    *       This thread is used to traverse elements in the
    *       order in which they were written to the instance (within
    *       PRESENTAION.access_scope==INSTANCE).  It is mainly used on the
    *       send side to count the depth of instance data and to allow the
    *       removal of elements by instance.
    *
    *   next_send_sample_
    *     - the next sample of data to be sent.  This thread is used
    *       external to the container to maintain a list of data samples
    *       that are to be transmitted over the transport layer. The
    *       Publisher may use this thread to maintain a list of samples to
    *       be sent with PRESENTATION.access_scope==GROUP by obtaining
    *       data from each DataWriter as it becomes available and
    *       concatentating the data in the order in which it was written.
    *
    * We thread this single element rather than having multiple smaller
    * lists in order to allow us to allocat once and have the element
    * contained in all of the lists in which it will be held during its
    * lifetime.  These three threads will at times hold the element in
    * three separate lists simultaneously.  The next_sample_ thread will be
    * used in the container to hold the element in one of three different
    * lists at different times, so a single thread is all that is required
    * for all of those lists.
    *
    * NOTE: this is what we want to pass into the enqueue method of the
    *       container, since we want to centralize the
    *       allocation/deallocation so that we can minimize locking.  By
    *       grabbing a single lock, allocating the
    *       buffer/Data_Block/Message_Block/DataSampleListElement at the
    *       same time, we only pay once for all of the allocations.  They
    *       are all presumably from a cache (for the most part) anyway, so
    *       it should be fairly quick.
    */
    struct TAO_DdsDcps_Export DataSampleListElement 
    {
      DataSampleListElement (PublicationId           publication_id,
                             TransportSendListener*  send_listner,
                             PublicationInstance*    handle);

      ~DataSampleListElement ();

      /// Message being sent which includes the DataSampleHeader message block
      /// and DataSample message block.
      DataSample*            sample_ ;

      /// Publication Id used downstream.
      PublicationId          publication_id_ ;

      /// Group Id used downstream.
      /// This is not used in the first implementation (INSTANCE level)
      CoherencyGroup         group_id_ ;

      /// Used to make removal from the
      /// container _much_ more efficient.
      DataSampleListElement* previous_sample_ ;

      /// Thread of data within the DataWriter.
      DataSampleListElement* next_sample_ ;

      /// Thread of data within the instance.
      DataSampleListElement* next_instance_sample_ ;

      /// Thread of data being sent.
      DataSampleListElement* next_send_sample_ ;

      /// Pointer to object that will be informed when the data has
      /// been delivered.  This needs to be set prior to using the
      /// TransportInterface to send().
      TransportSendListener* send_listener_;

      /// The flag indicates space availability for this waiting DataSample. 
      bool space_available_;

      /// The pointer to the object that contains the instance information 
      /// and data sample list.
      /// The client holds this as an InstanceHandle_t.
      PublicationInstance*   handle_;
    };

    /**
    * Lists include a pointer to both the head and tail elements of the
    * list.  Cache the number of elements in the list so that we don't have
    * to traverse the list to find this information.  For most lists that
    * we manage, we append to the tail and remove from the head.  There are
    * some lists where we remove from the middle, which are not handled by
    * this list structure.
    */
    class TAO_DdsDcps_Export DataSampleList {

    public:
      /// Default constructor clears the list.
      DataSampleList();
      
      /// Reset to initial state.
      void reset ();

      /// This function assumes the list is the sending_data, sent_data,
      /// unsent_data or released_data which is linked by the 
      /// next_sample/previous_sample.
      void enqueue_tail_next_sample (DataSampleListElement* sample);

      /// This function assumes the list is the sending_data, sent_data,
      /// unsent_data or released_data which is linked by the 
      /// next_sample/previous_sample.
      bool dequeue_head_next_sample (DataSampleListElement*& stale);

      /// This function assumes the list is the sending_data or sent_data
      /// which is linked by the next_send_sample.
      void enqueue_tail_next_send_sample (DataSampleListElement* sample);

      /// This function assumes the list is the sending_data or sent_data
      /// which is linked by the next_send_sample.
      bool dequeue_head_next_send_sample (DataSampleListElement*& stale);

      /// This function assumes the list is the instance samples that is
      /// linked by the next_instance_sample_.
      void enqueue_tail_next_instance_sample (DataSampleListElement* sample);

      /// This function assumes the list is the instance samples that is
      /// linked by the next_instance_sample_.
      bool dequeue_head_next_instance_sample (DataSampleListElement*& stale);

      /// This function assumes that the list is a list that linked using 
      /// next_sample/previous_sample but the stale element's position is 
      /// unknown.
      bool dequeue_next_sample (DataSampleListElement* stale);

      /// This function assumes that the list is a list that linked using 
      /// next_instance_sample but the stale element's position is 
      /// unknown.
      bool dequeue_next_instance_sample (DataSampleListElement* stale);

      /// This function assumes that the list is a list that linked using 
      /// next_send_sample but the stale element's position is 
      /// unknown.
      bool dequeue_next_send_sample (DataSampleListElement* stale);

      /// This function assumes the appended list is a list linked with 
      /// previous/next_sample_ and might be linked with next_send_sample_.
      /// If it's not linked with the next_send_sample_ then this function
      /// will make it linked before appending.
      void enqueue_tail_next_send_sample (DataSampleList list);

      /// The first element of the list.
      DataSampleListElement* head_ ;

      /// The last element of the list.
      DataSampleListElement* tail_ ;

      /// Number of elements in the list.
      ssize_t                size_ ;
      //TBD size is never negative so should be size_t but this ripples through
      // the transport code so leave it for now. SHH

    } ;

    /// Used to allocator the DataSampleListElement object.
    typedef Cached_Allocator_With_Overflow<DataSampleListElement, ACE_Null_Mutex>  
      DataSampleListElementAllocator;

  }  /* namespace DCPS */

}  /* namespace TAO */

#if defined(__ACE_INLINE__)
#include "DataSampleList.inl"
#endif /* __ACE_INLINE__ */

#endif  /* TAO_DCPS_DATASAMPLELIST_H */
