/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_INSTANCEDATASAMPLELIST_H
#define OPENDDS_DCPS_INSTANCEDATASAMPLELIST_H

#include "dds/DdsDcpsInfoUtilsC.h"
#include "Definitions.h"
#include "transport/framework/TransportDefs.h"
#include "Dynamic_Cached_Allocator_With_Overflow_T.h"

#include <iterator>

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

class DataSampleElement;


/**
* A list of DataSampleElement pointers to be queued by the order the
* samples are written to the instance (within
* PRESENTAION.access_scope==INSTANCE).  It is mainly used on the
* send side to count the depth of instance data and to allow the
* removal of elements by instance.
* Manages DataSampleElement's next_instance_sample pointer
*/
class OpenDDS_Dcps_Export InstanceDataSampleList {

 public:
  InstanceDataSampleList();
  ~InstanceDataSampleList(){}

  /// Reset to initial state.
  void reset();

  ssize_t size() const;
  DataSampleElement* head() const;
  DataSampleElement* tail() const;

  static bool on_some_list(const DataSampleElement* iter);
  static DataSampleElement* next(const DataSampleElement* iter);
  static DataSampleElement* prev(const DataSampleElement* iter);

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


OPENDDS_END_VERSIONED_NAMESPACE_DECL

#if defined(__ACE_INLINE__)
#include "InstanceDataSampleList.inl"
#endif /* __ACE_INLINE__ */

#endif  /* OPENDDS_DCPS_INSTANCEDATASAMPLELIST_H */
