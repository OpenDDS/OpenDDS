/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_WRITERDATASAMPLELIST_H
#define OPENDDS_DCPS_WRITERDATASAMPLELIST_H

#include "dcps_export.h"
#include <cstring>

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

class DataSampleElement;

/**
* A list of DataSampleElement pointers to be queued by the order the
* samples are written to the DataWriter (within PRESENTATION.access_scope==TOPIC).
* Cache the number of elements in the list so that list traversal is
* not required to find this information.
* Manages DataSampleElement's previous_writer_sample/next_writer_sample pointers
*/
class OpenDDS_Dcps_Export WriterDataSampleList {

 public:

  /// Default constructor clears the list.
  WriterDataSampleList();
  ~WriterDataSampleList(){}

  /// Reset to initial state.
  void reset();

  ssize_t size() const;
  DataSampleElement* head() const;
  DataSampleElement* tail() const;

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
#include "WriterDataSampleList.inl"
#endif /* __ACE_INLINE__ */

#endif  /* OPENDDS_DCPS_WRITERDATASAMPLELIST_H */
