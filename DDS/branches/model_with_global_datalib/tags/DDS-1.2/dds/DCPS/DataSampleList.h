/// -*- C++ -*-
///
/// $Id$
#ifndef OPENDDS_DCPS_DATASAMPLELIST_H
#define OPENDDS_DCPS_DATASAMPLELIST_H

#include "dds/DdsDcpsInfoUtilsC.h"
#include "Definitions.h"
#include "Dynamic_Cached_Allocator_With_Overflow_T.h"

#include <iterator>

namespace OpenDDS
{

  namespace DCPS
  {
    const CORBA::ULong MAX_READERS_PER_ELEM = 5;
    typedef Dynamic_Cached_Allocator_With_Overflow<ACE_Thread_Mutex>
                                              TransportSendElementAllocator;
    
    const int MAX_READERS_TO_RESEND = 5;

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
    *   next_send_sample_/previous_send_sample_
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
    struct OpenDDS_Dcps_Export DataSampleListElement 
    {
      DataSampleListElement (PublicationId           publication_id,
                             TransportSendListener*  send_listner,
                             PublicationInstance*    handle,
                             TransportSendElementAllocator* allocator);

      DataSampleListElement (const DataSampleListElement& elem);

//remove check all calling locations of the above and rename to send both
      ~DataSampleListElement ();

      DataSampleListElement & operator= (DataSampleListElement const & elem);

      /// Message being sent which includes the DataSampleHeader message block
      /// and DataSample message block.
      DataSample*            sample_ ;

      /// Publication Id used downstream.
      PublicationId          publication_id_ ;
      CORBA::ULong           num_subs_;
      OpenDDS::DCPS::RepoId      subscription_ids_[OpenDDS::DCPS::MAX_READERS_PER_ELEM];

      /// Group Id used downstream.
      /// This is not used in the first implementation (INSTANCE level)
      CoherencyGroup         group_id_ ;

      /// Timestamp for the sample when it was first written.
      ::DDS::Time_t source_timestamp_;

      /// Used to make removal from the
      /// container _much_ more efficient.
      
      /// Thread of all data within a DataWriter.
      DataSampleListElement* previous_sample_ ;
      DataSampleListElement* next_sample_ ;

      /// Thread of data within the instance.
      DataSampleListElement* next_instance_sample_ ;

      /// Thread of data being unsent/sending/sent/released.
      DataSampleListElement* next_send_sample_ ;
      DataSampleListElement* previous_send_sample_;

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

      /// Allocator for the TransportSendElement. 
      TransportSendElementAllocator* transport_send_element_allocator_;
    };

    /**
     * @struct DataSampleListIterator
     *
     * @brief @c DataSampleList STL-style iterator implementation.
     *
     * This class implements a STL-style iterator for the OpenDDS
     * @c DataSampleList class.  The resulting iterator may be used
     * @c with the STL generic algorithms.  It is meant for iteration
     * @c over the "send samples" in a @c DataSampleList.
     */
    class OpenDDS_Dcps_Export DataSampleListIterator
      : public std::iterator<std::bidirectional_iterator_tag,
                             DataSampleListElement>
    {
    public:

      /// Default constructor.
      /**
       * This constructor is used when constructing an "end" iterator.
       */
      DataSampleListIterator();

      DataSampleListIterator(DataSampleListElement* head,
                             DataSampleListElement* tail,
                             DataSampleListElement* current);
      DataSampleListIterator(DataSampleListIterator const & rhs);

      DataSampleListIterator& operator=(DataSampleListIterator const & rhs);
      bool operator==(DataSampleListIterator& rhs) const;
      bool operator!=(DataSampleListIterator& rhs) const;
      DataSampleListIterator& operator++();
      DataSampleListIterator  operator++(int);
      DataSampleListIterator& operator--();
      DataSampleListIterator  operator--(int);
      reference operator*();
      pointer operator->();
      
    private:

      DataSampleListElement* head_;
      DataSampleListElement* tail_;
      DataSampleListElement* current_;

    };

    /**
    * Lists include a pointer to both the head and tail elements of the
    * list.  Cache the number of elements in the list so that we don't have
    * to traverse the list to ind this information.  For most lists that
    * we manage, we append to the tail and remove from the head.  There are
    * some lists where we remove from the middle, which are not handled by
    * this list structure.
    */
    class OpenDDS_Dcps_Export DataSampleList {

    public:

      /// STL-style bidirectional iterator type.
      typedef DataSampleListIterator iterator;

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

      /// Return iterator to beginning of list.
      iterator begin();

      /// Return iterator to end of list.
      iterator end();

      /// The first element of the list.
      DataSampleListElement* head_ ;

      /// The last element of the list.
      DataSampleListElement* tail_ ;

      /// Number of elements in the list.
      ssize_t                size_ ;
      //TBD size is never negative so should be size_t but this ripples through
      // the transport code so leave it for now. SHH

    };

    /// Used to allocator the DataSampleListElement object.
    typedef Cached_Allocator_With_Overflow<DataSampleListElement, ACE_Null_Mutex>  
      DataSampleListElementAllocator;

  }  /* namespace DCPS */

}  /* namespace OpenDDS */

#if defined(__ACE_INLINE__)
#include "DataSampleList.inl"
#endif /* __ACE_INLINE__ */

#endif  /* OPENDDS_DCPS_DATASAMPLELIST_H */
