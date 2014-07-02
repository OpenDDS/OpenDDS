/*
 * $Id$
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_DATASAMPLEINSTANCELIST_H
#define OPENDDS_DCPS_DATASAMPLEINSTANCELIST_H

#include "dds/DdsDcpsInfoUtilsC.h"
#include "Definitions.h"
#include "transport/framework/TransportDefs.h"
#include "Dynamic_Cached_Allocator_With_Overflow_T.h"
//#include "DataSampleHeader.h"
//#include "DataSampleListElement.h"

#include <iterator>

namespace OpenDDS {
namespace DCPS {

class DataSampleListElement;

//struct DataSampleListElement;
typedef Cached_Allocator_With_Overflow<DataSampleListElement, ACE_Null_Mutex>
  DataSampleListElementAllocator;

//const int MAX_READERS_TO_RESEND = 5;

/**
* Currently we contain entire messages in a single ACE_Message_Block
* chain.
*/
typedef ACE_Message_Block DataSample;


/**
* Lists include a pointer to both the head and tail elements of the
* list.  Cache the number of elements in the list so that we don't have
* to traverse the list to ind this information.  For most lists that
* we manage, we append to the tail and remove from the head.
*/
class OpenDDS_Dcps_Export DataSampleInstanceList {

 public:

  DataSampleInstanceList();
  ~DataSampleInstanceList(){};

  void reset();

  ssize_t size() const {return size_;};
  DataSampleListElement* head() const {return head_;};
  DataSampleListElement* tail() const {return tail_;};

  void enqueue_tail(const DataSampleListElement* element);

  bool dequeue_head(DataSampleListElement*& stale);

  bool dequeue(const DataSampleListElement* stale);

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
#include "DataSampleInstanceList.inl"
#endif /* __ACE_INLINE__ */

#endif  /* OPENDDS_DCPS_DATASAMPLEINSTANCELIST_H */
