/*
 * $Id$
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_SENDSTATEDATASAMPLELIST_H
#define OPENDDS_DCPS_SENDSTATEDATASAMPLELIST_H

#include "dds/DdsDcpsInfoUtilsC.h"
#include "Definitions.h"
#include "transport/framework/TransportDefs.h"
#include "Dynamic_Cached_Allocator_With_Overflow_T.h"
//#include "DataSampleHeader.h"
//#include "DataSampleElement.h"

#include <iterator>

class DDS_TEST;

namespace OpenDDS {
namespace DCPS {

class DataSampleElement;

//struct DataSampleElement;
typedef Cached_Allocator_With_Overflow<DataSampleElement, ACE_Null_Mutex>
  DataSampleElementAllocator;

const int MAX_READERS_TO_RESEND = 5;

/**
* Currently we contain entire messages in a single ACE_Message_Block
* chain.
*/
typedef ACE_Message_Block DataSample;

/**
 * @struct SendStateDataSampleListIterator
 *
 * @brief @c SendStateDataSampleList STL-style iterator implementation.
 *
 * This class implements a STL-style iterator for the OpenDDS
 * @c SendStateDataSampleList class.  The resulting iterator may be used
 * @c with the STL generic algorithms.  It is meant for iteration
 * @c over the "send samples" in a @c SendStateDataSampleList.
 */
class OpenDDS_Dcps_Export SendStateDataSampleListIterator
  : public std::iterator<std::bidirectional_iterator_tag, DataSampleElement> {
public:

  /// Default constructor.
  /**
   * This constructor is used when constructing an "end" iterator.
   */

  SendStateDataSampleListIterator(DataSampleElement* head,
                         DataSampleElement* tail,
                         DataSampleElement* current);

  SendStateDataSampleListIterator& operator++();
  SendStateDataSampleListIterator  operator++(int);
  SendStateDataSampleListIterator& operator--();
  SendStateDataSampleListIterator  operator--(int);
  reference operator*();
  pointer operator->();

  bool
  operator==(const SendStateDataSampleListIterator& rhs) const {
    return this->head_ == rhs.head_
           && this->tail_ == rhs.tail_
           && this->current_ == rhs.current_;
  }

  bool
  operator!=(const SendStateDataSampleListIterator& rhs) const {
    return !(*this == rhs);
  }

private:
  SendStateDataSampleListIterator();

  DataSampleElement* head_;
  DataSampleElement* tail_;
  DataSampleElement* current_;

};

/**
* Lists include a pointer to both the head and tail elements of the
* list.  Cache the number of elements in the list so that we don't have
* to traverse the list to find this information.  For most lists that
* we manage, we append to the tail and remove from the head.
*/
class OpenDDS_Dcps_Export SendStateDataSampleList {

  friend class ::DDS_TEST;

 public:

  typedef SendStateDataSampleListIterator iterator;

  SendStateDataSampleList();
  ~SendStateDataSampleList(){};

  static const SendStateDataSampleList* send_list_containing_element(const DataSampleElement* element,
                                                                std::vector<SendStateDataSampleList*> send_lists);

  /// Reset to initial state.
  void reset();

  ssize_t size() const {return size_;};
  DataSampleElement* head() const {return head_;};
  DataSampleElement* tail() const {return tail_;};

  void enqueue_tail(const DataSampleElement* element);
  void enqueue_tail(SendStateDataSampleList list);

  bool dequeue_head(DataSampleElement*& stale);

  bool dequeue(const DataSampleElement* stale);

  /// Return iterator to beginning of list.
  iterator begin();

  /// Return iterator to end of list.
  iterator end();

 protected:

   /// The first element of the list.
   DataSampleElement* head_;

   /// The last element of the list.
   DataSampleElement* tail_;

   /// Number of elements in the list.
   ssize_t                size_;
   //TBD size is never negative so should be size_t but this ripples through
   // the transport code so leave it for now. SHH

};

} // namespace DCPS
} // namespace OpenDDS

#if defined(__ACE_INLINE__)
#include "SendStateDataSampleList.inl"
#endif /* __ACE_INLINE__ */

#endif  /* OPENDDS_DCPS_SENDSTATEDATASAMPLELIST_H */
