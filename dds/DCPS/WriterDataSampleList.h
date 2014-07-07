/*
 * $Id$
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_WRITERDATASAMPLELIST_H
#define OPENDDS_DCPS_WRITERDATASAMPLELIST_H

#include "dds/DdsDcpsInfoUtilsC.h"
#include "Definitions.h"
#include "transport/framework/TransportDefs.h"
#include "Dynamic_Cached_Allocator_With_Overflow_T.h"
//#include "DataSampleHeader.h"
//#include "DataSampleElement.h"

#include <iterator>

namespace OpenDDS {
namespace DCPS {

class DataSampleElement;

//struct DataSampleElement;
//typedef Cached_Allocator_With_Overflow<DataSampleElement, ACE_Null_Mutex>
//  DataSampleElementAllocator;

//const int MAX_READERS_TO_RESEND = 5;


/**
* Currently we contain entire messages in a single ACE_Message_Block
* chain.
*/
//typedef ACE_Message_Block DataSample;



/**
* A list of DataSampleElement pointers to be queued by the order the
* samples are written to the DataWriter (within PRESENTATION.access_scope==TOPIC).
*/
class OpenDDS_Dcps_Export WriterDataSampleList {

 public:

  WriterDataSampleList();
  ~WriterDataSampleList(){};

  /// Reset to initial state.
  void reset();

  ssize_t size() const {return size_;};

  DataSampleElement* head() const {return head_;};

  DataSampleElement* tail() const {return tail_;};

  void enqueue_tail(const DataSampleElement* element);

  bool dequeue_head(DataSampleElement*& stale);

  bool dequeue(const DataSampleElement* stale);

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
#include "WriterDataSampleList.inl"
#endif /* __ACE_INLINE__ */

#endif  /* OPENDDS_DCPS_WRITERDATASAMPLELIST_H */
