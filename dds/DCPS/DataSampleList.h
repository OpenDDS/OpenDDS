///*
// * $Id$
// *
// *
// * Distributed under the OpenDDS License.
// * See: http://www.opendds.org/license.html
// */
//
//#ifndef OPENDDS_DCPS_DATASAMPLELIST_H
//#define OPENDDS_DCPS_DATASAMPLELIST_H
//
//#include "dds/DdsDcpsInfoUtilsC.h"
//#include "Definitions.h"
//#include "transport/framework/TransportDefs.h"
//#include "Dynamic_Cached_Allocator_With_Overflow_T.h"
////#include "DataSampleHeader.h"
//#include "DataSampleElement.h"
//
////#include <map>
//#include <iterator>
//
//namespace OpenDDS {
//namespace DCPS {
//
////const CORBA::ULong MAX_READERS_PER_ELEM = 5;
////typedef Dynamic_Cached_Allocator_With_Overflow<ACE_Thread_Mutex>
////  TransportSendElementAllocator;
//
////class TransportCustomizedElement;
////typedef Dynamic_Cached_Allocator_With_Overflow<ACE_Thread_Mutex>
////  TransportCustomizedElementAllocator;
//
//class DataSampleElement;
//typedef Cached_Allocator_With_Overflow<DataSampleElement, ACE_Null_Mutex>
//  DataSampleElementAllocator;
//
//const int MAX_READERS_TO_RESEND = 5;
//
////class TransportSendListener;
////struct PublicationInstance;
//
///**
//* Currently we contain entire messages in a single ACE_Message_Block
//* chain.
//*/
//typedef ACE_Message_Block DataSample;
//
///**
// * @struct SendStateDataSampleListIterator
// *
// * @brief @c DataSampleList STL-style iterator implementation.
// *
// * This class implements a STL-style iterator for the OpenDDS
// * @c DataSampleList class.  The resulting iterator may be used
// * @c with the STL generic algorithms.  It is meant for iteration
// * @c over the "send samples" in a @c DataSampleList.
// */
//class OpenDDS_Dcps_Export SendStateDataSampleListIterator
//  : public std::iterator<std::bidirectional_iterator_tag, DataSampleElement> {
//public:
//
//  /// Default constructor.
//  /**
//   * This constructor is used when constructing an "end" iterator.
//   */
//
//  SendStateDataSampleListIterator(DataSampleElement* head,
//                         DataSampleElement* tail,
//                         DataSampleElement* current);
//
//  SendStateDataSampleListIterator& operator++();
//  SendStateDataSampleListIterator  operator++(int);
//  SendStateDataSampleListIterator& operator--();
//  SendStateDataSampleListIterator  operator--(int);
//  reference operator*();
//  pointer operator->();
//
//  bool
//  operator==(const SendStateDataSampleListIterator& rhs) const {
//    return this->head_ == rhs.head_
//           && this->tail_ == rhs.tail_
//           && this->current_ == rhs.current_;
//  }
//
//  bool
//  operator!=(const SendStateDataSampleListIterator& rhs) const {
//    return !(*this == rhs);
//  }
//
//private:
//  SendStateDataSampleListIterator();
//
//  DataSampleElement* head_;
//  DataSampleElement* tail_;
//  DataSampleElement* current_;
//
//};
//
///**
//* Lists include a pointer to both the head and tail elements of the
//* list.  Cache the number of elements in the list so that we don't have
//* to traverse the list to ind this information.  For most lists that
//* we manage, we append to the tail and remove from the head.  There are
//* some lists where we remove from the middle, which are not handled by
//* this list structure.
//*/
//class OpenDDS_Dcps_Export DataSampleList {
//
//public:
//
//  /// STL-style bidirectional iterator type.
//  typedef SendStateDataSampleListIterator iterator;
//
//  /// Default constructor clears the list.
//  DataSampleList();
//  virtual ~DataSampleList(){};
//
//  /// Reset to initial state.
//  void reset();
//
//  //PWO: HELPER METHODS FOR SETTING DATA MEMBERS
//  // These methods should go away
//
//  void set_head(DataSampleElement* newHead) { this->head_ = newHead;};
//  void set_tail(DataSampleElement* newTail) { this->tail_ = newTail;};
//  void set_size(size_t size) { this->size_ = size;};
//
//  //PWO: END HELPER METHODS
//
//  ssize_t size() const {return size_;};
//
//  DataSampleElement* head() const {return head_;};
//
//  DataSampleElement* tail() const {return tail_;};
//
//  virtual void enqueue_tail(const DataSampleElement* element) = 0;
//
//  virtual bool dequeue_head(DataSampleElement*& stale) = 0;
//
//  //PWO: took away the 'const' for the paramter 'stale'
//  //virtual bool dequeue(/*const*/ DataSampleElement* stale) = 0;
//
//  //virtual DataSampleElement* dequeue_sample(const DataSampleElement* stale) = 0;
//
//  /// This function assumes the list is the sending_data, sent_data,
//  /// unsent_data or released_data which is linked by the
//  /// next_sample/previous_sample.
//  //void enqueue_tail_next_sample(DataSampleElement* sample);
//
//  /// This function assumes the list is the sending_data, sent_data,
//  /// unsent_data or released_data which is linked by the
//  /// next_sample/previous_sample.
//  //bool dequeue_head_next_sample(DataSampleElement*& stale);
//
//  /// This function assumes the list is the sending_data or sent_data
//  /// which is linked by the next_send_sample.
//  //void enqueue_tail_next_send_sample(const DataSampleElement* sample);
//
//  /// This function assumes the list is the sending_data or sent_data
//  /// which is linked by the next_send_sample.
//  //bool dequeue_head_next_send_sample(DataSampleElement*& stale);
//
//  /// This function assumes the list is the instance samples that is
//  /// linked by the next_instance_sample_.
////  void enqueue_tail_next_instance_sample(DataSampleElement* sample);
//
//  /// This function assumes the list is the instance samples that is
//  /// linked by the next_instance_sample_.
////  bool dequeue_head_next_instance_sample(DataSampleElement*& stale);
//
//  /// This function assumes that the list is a list that linked using
//  /// next_sample/previous_sample but the stale element's position is
//  /// unknown.
//  //bool dequeue_next_sample(DataSampleElement* stale);
//
//  /// This function assumes that the list is a list that linked using
//  /// next_instance_sample but the stale element's position is
//  /// unknown.
////  DataSampleElement*
////  dequeue_next_instance_sample(const DataSampleElement* stale);
//
//  /// This function assumes that the list is a list that linked using
//  /// next_send_sample but the stale element's position is
//  /// unknown.
// // DataSampleElement*
// // dequeue_next_send_sample(const DataSampleElement* stale);
//
//  /// This function assumes the appended list is a list linked with
//  /// previous/next_sample_ and might be linked with next_send_sample_.
//  /// If it's not linked with the next_send_sample_ then this function
//  /// will make it linked before appending.
// // void enqueue_tail_next_send_sample(DataSampleList list);
//
//  /// Return iterator to beginning of list.
//  iterator begin();
//
//  /// Return iterator to end of list.
//  iterator end();
//
//protected:
//
//  /// The first element of the list.
//  DataSampleElement* head_;
//
//  /// The last element of the list.
//  DataSampleElement* tail_;
//
//  /// Number of elements in the list.
//  ssize_t                size_;
//  //TBD size is never negative so should be size_t but this ripples through
//  // the transport code so leave it for now. SHH
//};
//
//class OpenDDS_Dcps_Export WriterDataSampleList : public DataSampleList {
//
// public:
//
//  WriterDataSampleList() : DataSampleList(){};
//  ~WriterDataSampleList(){};
//
//  //void reset();
//
//  void enqueue_tail(const DataSampleElement* element);
//
//  bool dequeue_head(DataSampleElement*& stale);
//
//  bool dequeue(const DataSampleElement* stale);
//
//};
//
//class OpenDDS_Dcps_Export DataSampleInstanceList : public DataSampleList {
//
// public:
//
//  DataSampleInstanceList() : DataSampleList(){};
//  ~DataSampleInstanceList(){};
//
//  //void reset();
//
//  void enqueue_tail(const DataSampleElement* element);
//
//  bool dequeue_head(DataSampleElement*& stale);
//
//  bool dequeue(const DataSampleElement* stale);
//
//};
//
//class OpenDDS_Dcps_Export SendStateDataSampleList : public DataSampleList {
//
// public:
//
//  typedef SendStateDataSampleListIterator iterator;
//
//  SendStateDataSampleList() : DataSampleList(){};
//  ~SendStateDataSampleList(){};
//
//  //void reset();
//  static const SendStateDataSampleList* send_list_containing_element(const DataSampleElement* element,
//                                                                std::vector<SendStateDataSampleList*> send_lists);
//
//  void enqueue_tail(const DataSampleElement* element);
//  void enqueue_tail(SendStateDataSampleList list);
//
//  bool dequeue_head(DataSampleElement*& stale);
//
//  bool dequeue(const DataSampleElement* stale);
//
//};
//
//} // namespace DCPS
//} // namespace OpenDDS
//
//#if defined(__ACE_INLINE__)
//#include "DataSampleList.inl"
//#endif /* __ACE_INLINE__ */
//
//#endif  /* OPENDDS_DCPS_DATASAMPLELIST_H */
