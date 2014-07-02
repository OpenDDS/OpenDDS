/*
 * $Id$
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_DATASAMPLESENDLIST_H
#define OPENDDS_DCPS_DATASAMPLESENDLIST_H

#include "dds/DdsDcpsInfoUtilsC.h"
#include "Definitions.h"
#include "transport/framework/TransportDefs.h"
#include "Dynamic_Cached_Allocator_With_Overflow_T.h"
//#include "DataSampleHeader.h"
//#include "DataSampleListElement.h"

#include <iterator>

class DDS_TEST;

namespace OpenDDS {
namespace DCPS {

class DataSampleListElement;

//struct DataSampleListElement;
typedef Cached_Allocator_With_Overflow<DataSampleListElement, ACE_Null_Mutex>
  DataSampleListElementAllocator;

const int MAX_READERS_TO_RESEND = 5;

/**
* Currently we contain entire messages in a single ACE_Message_Block
* chain.
*/
typedef ACE_Message_Block DataSample;

/**
 * @struct DataSampleSendListIterator
 *
 * @brief @c DataSampleSendList STL-style iterator implementation.
 *
 * This class implements a STL-style iterator for the OpenDDS
 * @c DataSampleSendList class.  The resulting iterator may be used
 * @c with the STL generic algorithms.  It is meant for iteration
 * @c over the "send samples" in a @c DataSampleSendList.
 */
class OpenDDS_Dcps_Export DataSampleSendListIterator
  : public std::iterator<std::bidirectional_iterator_tag, DataSampleListElement> {
public:

  /// Default constructor.
  /**
   * This constructor is used when constructing an "end" iterator.
   */

  DataSampleSendListIterator(DataSampleListElement* head,
                         DataSampleListElement* tail,
                         DataSampleListElement* current);

  DataSampleSendListIterator& operator++();
  DataSampleSendListIterator  operator++(int);
  DataSampleSendListIterator& operator--();
  DataSampleSendListIterator  operator--(int);
  reference operator*();
  pointer operator->();

  bool
  operator==(const DataSampleSendListIterator& rhs) const {
    return this->head_ == rhs.head_
           && this->tail_ == rhs.tail_
           && this->current_ == rhs.current_;
  }

  bool
  operator!=(const DataSampleSendListIterator& rhs) const {
    return !(*this == rhs);
  }

private:
  DataSampleSendListIterator();

  DataSampleListElement* head_;
  DataSampleListElement* tail_;
  DataSampleListElement* current_;

};

/**
* Lists include a pointer to both the head and tail elements of the
* list.  Cache the number of elements in the list so that we don't have
* to traverse the list to find this information.  For most lists that
* we manage, we append to the tail and remove from the head.
*/
class OpenDDS_Dcps_Export DataSampleSendList {

  friend class ::DDS_TEST;

 public:

  typedef DataSampleSendListIterator iterator;

  DataSampleSendList();
  ~DataSampleSendList(){};

  static const DataSampleSendList* send_list_containing_element(const DataSampleListElement* element,
                                                                std::vector<DataSampleSendList*> send_lists);

  /// Reset to initial state.
  void reset();

  ssize_t size() const {return size_;};
  DataSampleListElement* head() const {return head_;};
  DataSampleListElement* tail() const {return tail_;};

  void enqueue_tail(const DataSampleListElement* element);
  void enqueue_tail(DataSampleSendList list);

  bool dequeue_head(DataSampleListElement*& stale);

  bool dequeue(const DataSampleListElement* stale);

  /// Return iterator to beginning of list.
  iterator begin();

  /// Return iterator to end of list.
  iterator end();

 protected:

   /// The first element of the list.
   DataSampleListElement* head_;

   /// The last element of the list.
   DataSampleListElement* tail_;

   /// Number of elements in the list.
   ssize_t                size_;
   //TBD size is never negative so should be size_t but this ripples through
   // the transport code so leave it for now. SHH

};

} // namespace DCPS
} // namespace OpenDDS

#if defined(__ACE_INLINE__)
#include "DataSampleSendList.inl"
#endif /* __ACE_INLINE__ */

#endif  /* OPENDDS_DCPS_DATASAMPLESENDLIST_H */
