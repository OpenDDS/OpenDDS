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

#include <iterator>

class DDS_TEST;

namespace OpenDDS {
namespace DCPS {

class DataSampleElement;

const int MAX_READERS_TO_RESEND = 5;

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
#ifndef ACE_LYNXOS_MAJOR
  : public std::iterator<std::bidirectional_iterator_tag, DataSampleElement>
#endif
{
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

#ifdef ACE_LYNXOS_MAJOR
  typedef DataSampleElement& reference;
  typedef DataSampleElement* pointer;
#endif

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

  friend class SendStateDataSampleListConstIterator;
};

/**
 * @struct SendStateDataSampleListConstIterator
 *
 * @brief @c SendStateDataSampleList STL-style const iterator implementation.
 *
 * This class implements a STL-style const iterator for the OpenDDS
 * @c SendStateDataSampleList class.  The resulting iterator may be used
 * @c with the STL generic algorithms.  It is meant for iteration
 * @c over the "send samples" in a @c SendStateDataSampleList.
 */
class OpenDDS_Dcps_Export SendStateDataSampleListConstIterator
{
public:
  typedef const DataSampleElement* pointer;
  typedef const DataSampleElement& reference;


  SendStateDataSampleListConstIterator(const DataSampleElement* head,
                                  const DataSampleElement* tail,
                                  const DataSampleElement* current);

  SendStateDataSampleListConstIterator(const SendStateDataSampleListIterator& iterator);

  SendStateDataSampleListConstIterator& operator++();
  SendStateDataSampleListConstIterator  operator++(int);
  SendStateDataSampleListConstIterator& operator--();
  SendStateDataSampleListConstIterator  operator--(int);
  reference operator*() const;
  pointer operator->() const;

  bool
  operator==(const SendStateDataSampleListConstIterator& rhs) const {
    return this->head_ == rhs.head_
           && this->tail_ == rhs.tail_
           && this->current_ == rhs.current_;
  }

  bool
  operator!=(const SendStateDataSampleListConstIterator& rhs) const {
    return !(*this == rhs);
  }

private:
  SendStateDataSampleListConstIterator();

  const DataSampleElement* head_;
  const DataSampleElement* tail_;
  const DataSampleElement* current_;

};

/**
* A list of DataSampleElement pointers to be queued by the order the
* samples are to be transmitted over the transport layer.
* Cache the number of elements in the list so that list traversal is
* not required to find this information.
* The Publisher may use this to maintain a list of samples to
* be sent with PRESENTATION.access_scope==GROUP by obtaining
* data from each DataWriter as it becomes available and
* concatenating the data in the order in which it was written.
* Manages DataSampleElement's previous_send_sample/next_send_sample pointers
*/
class OpenDDS_Dcps_Export SendStateDataSampleList {

  friend class ::DDS_TEST;

 public:

  /// STL-style bidirectional iterator and const-iterator types.
  typedef SendStateDataSampleListIterator iterator;
  typedef SendStateDataSampleListConstIterator const_iterator;

  /// Default constructor clears the list.
  SendStateDataSampleList();
  ~SendStateDataSampleList(){};

  /// Returns a pointer to the SendStateDataSampleList containing a
  /// given DataSampleElement for use in the typical situation where
  /// the send state of a DataSampleElement is tracked by shifting
  /// it between distinct SendStateDataSampleLists, one for each state
  static const SendStateDataSampleList* send_list_containing_element(const DataSampleElement* element,
                                                                std::vector<SendStateDataSampleList*> send_lists);

  /// Reset to initial state.
  void reset();

  ssize_t size() const;
  DataSampleElement* head() const;
  DataSampleElement* tail() const;

  void enqueue_tail(const DataSampleElement* element);
  void enqueue_tail(SendStateDataSampleList list);

  bool dequeue_head(DataSampleElement*& stale);

  bool dequeue(const DataSampleElement* stale);

  /// Return iterator to beginning of list.
  iterator begin();
  const_iterator begin() const;

  /// Return iterator to end of list.
  iterator end();
  const_iterator end() const;

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
