///*
// * $Id$
// *
// *
// * Distributed under the OpenDDS License.
// * See: http://www.opendds.org/license.html
// */
//
#ifndef OPENDDS_DCPS_DATASAMPLEELEMENT_H
#define OPENDDS_DCPS_DATASAMPLEELEMENT_H

#include "dds/DdsDcpsInfoUtilsC.h"
#include "Definitions.h"
#include "transport/framework/TransportDefs.h"
#include "Dynamic_Cached_Allocator_With_Overflow_T.h"
#include "DataSampleHeader.h"

#include <map>

namespace OpenDDS {
namespace DCPS {

const CORBA::ULong MAX_READERS_PER_ELEM = 5;

typedef Cached_Allocator_With_Overflow<DataSampleElement, ACE_Null_Mutex>
  DataSampleElementAllocator;

typedef Dynamic_Cached_Allocator_With_Overflow<ACE_Thread_Mutex>
  TransportSendElementAllocator;

class TransportCustomizedElement;
typedef Dynamic_Cached_Allocator_With_Overflow<ACE_Thread_Mutex>
  TransportCustomizedElementAllocator;

class TransportSendListener;
struct PublicationInstance;

class DDS_TEST;

/**
* Currently we contain entire messages in a single ACE_Message_Block
* chain.
*/
typedef ACE_Message_Block DataSample;

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
*       concatenating the data in the order in which it was written.
*
* We thread this single element rather than having multiple smaller
* lists in order to allow us to allocate once and have the element
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
*       buffer/Data_Block/Message_Block/DataSampleElement at the
*       same time, we only pay once for all of the allocations.  They
*       are all presumably from a cache (for the most part) anyway, so
*       it should be fairly quick.
*/
class OpenDDS_Dcps_Export DataSampleElement {


public:
  DataSampleElement(PublicationId                   publication_id,
                        TransportSendListener*          send_listner,
                        PublicationInstance*            handle,
                        TransportSendElementAllocator*  tse_allocator,
                        TransportCustomizedElementAllocator* tce_allocator);

  DataSampleElement(const DataSampleElement& elem);
  DataSampleElement& operator=(const DataSampleElement& elem);

  ~DataSampleElement();

  /// The OpenDDS DCPS header for this sample
  DataSampleHeader       header_;

  /// Message being sent which includes the DataSampleHeader message block
  /// and DataSample message block.
  DataSample*            sample_;

  /// Publication Id used downstream.
  PublicationId          publication_id_;
  CORBA::ULong           num_subs_;
  OpenDDS::DCPS::RepoId  subscription_ids_[OpenDDS::DCPS::MAX_READERS_PER_ELEM];

  /// Pointer to object that will be informed when the data has
  /// been delivered.  This needs to be set prior to using the
  /// TransportClient to send().
  TransportSendListener* send_listener_;

  /// The flag indicates space availability for this waiting DataSample.
  bool space_available_;

  /// The pointer to the object that contains the instance information
  /// and data sample list.
  /// The client holds this as an InstanceHandle_t.
  PublicationInstance*   handle_;

  /// Allocator for the TransportSendElement.
  TransportSendElementAllocator* transport_send_element_allocator_;

  /// Allocator for TransportCustomizedElement
  TransportCustomizedElementAllocator* transport_customized_element_allocator_;

  //{@
  /// tracking for Content-Filtering data
  GUIDSeq_var filter_out_;
  std::map<DataLinkIdType, GUIDSeq_var> filter_per_link_;
  //@}

private:

  DataSampleElement* get_next_send_sample() const;

  void set_next_send_sample(DataSampleElement* next_send_sample);

  // DataSampleList is in charge of managing list placement therefore needs access to pointers
  //friend class DataSampleList;
  friend class SendStateDataSampleList;
  friend class WriterDataSampleList;
  friend class InstanceDataSampleList;
  friend class TransportClient;
  friend class DDS_TEST;
  // Iterators needs access to prev/next pointers for iteration
  friend class SendStateDataSampleListIterator;
  friend class SendStateDataSampleListConstIterator;


  /// Used to make removal from the
  /// container _much_ more efficient.

  /// Thread of all data within a DataWriter.
  mutable DataSampleElement* previous_sample_;
  mutable DataSampleElement* next_sample_;

  /// Thread of data within the instance.
  mutable DataSampleElement* next_instance_sample_;

  /// Thread of data being unsent/sending/sent/released.
  mutable DataSampleElement* next_send_sample_;
  mutable DataSampleElement* previous_send_sample_;
};


} // namespace DCPS
} // namespace OpenDDS

#if defined(__ACE_INLINE__)
#include "DataSampleElement.inl"
#endif /* __ACE_INLINE__ */

#endif  /* OPENDDS_DCPS_DATASAMPLEELEMENT_H */
