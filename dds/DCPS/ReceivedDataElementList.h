/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_RECEIVEDDATAELEMENTLIST_H
#define OPENDDS_DCPS_RECEIVEDDATAELEMENTLIST_H

#ifdef ACE_HAS_CPP11
#  include <atomic>
#else
#  include <ace/Atomic_Op_T.h>
#endif
#include "ace/Thread_Mutex.h"

#include "dcps_export.h"
#include "DataSampleHeader.h"
#include "Definitions.h"
#include "GuidUtils.h"
#include "InstanceState.h"
#include "Time_Helper.h"
#include "unique_ptr.h"

#include "dds/DdsDcpsInfrastructureC.h"

#if !defined (ACE_LACKS_PRAGMA_ONCE)
#pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

class OpenDDS_Dcps_Export ReceivedDataElement {
public:
  ReceivedDataElement(const DataSampleHeader& header, void *received_data, ACE_Recursive_Thread_Mutex* mx)
    : pub_(header.publication_id_),
      registered_data_(received_data),
      sample_state_(DDS::NOT_READ_SAMPLE_STATE),
#ifndef OPENDDS_NO_OBJECT_MODEL_PROFILE
      coherent_change_(header.coherent_change_),
      group_coherent_(header.group_coherent_),
      publisher_id_(header.publisher_id_),
#endif
      valid_data_(received_data != 0),
      disposed_generation_count_(0),
      no_writers_generation_count_(0),
      zero_copy_cnt_(0),
      sequence_(header.sequence_),
      previous_data_sample_(0),
      next_data_sample_(0),
      ref_count_(1),
      mx_(mx)
  {
    destination_timestamp_ = SystemTimePoint::now().to_dds_time();

    source_timestamp_.sec = header.source_timestamp_sec_;
    source_timestamp_.nanosec = header.source_timestamp_nanosec_;

    /*
     * In some situations, we will not have data to give to the user and
     * valid_data is how we communcate that to the user through a SampleInfo.
     */
    if (!header.valid_data()) {
      valid_data_ = false;
    }
  }

  virtual ~ReceivedDataElement(){}

  void dec_ref()
  {
    if (0 == --this->ref_count_) {
      delete this;
    }
  }

  void inc_ref()
  {
    ++this->ref_count_;
  }

  long ref_count()
  {
#ifdef ACE_HAS_CPP11
    return this->ref_count_;
#else
    return this->ref_count_.value();
#endif
  }

  PublicationId pub_;

  /**
   * Data sample received, could only be the key fields in case we received
   * dispose and/or unregister message.
   */
  void* const registered_data_;  // ugly, but works....

  /// Sample state for this data sample:
  /// DDS::NOT_READ_SAMPLE_STATE/DDS::READ_SAMPLE_STATE
  DDS::SampleStateKind sample_state_;

  /// Source time stamp for this data sample
  DDS::Time_t source_timestamp_;

  /// Reception time stamp for this data sample
  DDS::Time_t destination_timestamp_;

#ifndef OPENDDS_NO_OBJECT_MODEL_PROFILE
  /// Sample belongs to an active coherent change set
  bool coherent_change_;

  /// Sample belongs to a group coherent changes.
  bool group_coherent_;

  /// Publisher id represent group identifier.
  RepoId publisher_id_;
#endif

  /// Do we contain valid data
  bool valid_data_;

  /// The data sample's instance's disposed_generation_count_
  /// at the time the sample was received
  size_t disposed_generation_count_;

  /// The data sample's instance's no_writers_generation_count_
  /// at the time the sample was received
  size_t no_writers_generation_count_;

  /// This is needed to know if delete DataReader should fail with
  /// PRECONDITION_NOT_MET because there are outstanding loans.
#ifdef ACE_HAS_CPP11
  std::atomic<long> zero_copy_cnt_;
#else
  ACE_Atomic_Op<ACE_Thread_Mutex, long> zero_copy_cnt_;
#endif

  /// The data sample's sequence number
  SequenceNumber sequence_;

  /// the previous data sample in the ReceivedDataElementList
  ReceivedDataElement* previous_data_sample_;

  /// the next data sample in the ReceivedDataElementList
  ReceivedDataElement* next_data_sample_;

  void* operator new(size_t size, ACE_New_Allocator& pool);
  void operator delete(void* memory);
  void operator delete(void* memory, ACE_New_Allocator& pool);

private:
#ifdef ACE_HAS_CPP11
  std::atomic<long> ref_count_;
#else
  ACE_Atomic_Op<ACE_Thread_Mutex, long> ref_count_;
#endif
protected:
  ACE_Recursive_Thread_Mutex* mx_;
}; // class ReceivedDataElement

struct ReceivedDataElementMemoryBlock
{
  ReceivedDataElement element_;
  ACE_New_Allocator* allocator_;
};


template <typename DataTypeWithAllocator>
class ReceivedDataElementWithType : public ReceivedDataElement
{
public:
  ReceivedDataElementWithType(const DataSampleHeader& header, DataTypeWithAllocator* received_data, ACE_Recursive_Thread_Mutex* mx)
    : ReceivedDataElement(header, received_data, mx)
  {
  }

  ~ReceivedDataElementWithType() {
    ACE_GUARD(ACE_Recursive_Thread_Mutex,
              guard,
              *this->mx_)
    delete static_cast<DataTypeWithAllocator*> (registered_data_);
  }
};

class OpenDDS_Dcps_Export ReceivedDataFilter {
public:
  ReceivedDataFilter() {}
  virtual ~ReceivedDataFilter() {}

  virtual bool operator()(ReceivedDataElement* data_sample) = 0;
};

class OpenDDS_Dcps_Export ReceivedDataOperation {
public:
  ReceivedDataOperation() {}
  virtual ~ReceivedDataOperation() {}

  virtual void operator()(ReceivedDataElement* data_sample) = 0;
};

class OpenDDS_Dcps_Export ReceivedDataElementList {
public:
  explicit ReceivedDataElementList(DataReaderImpl*, InstanceState_rch instance_state = InstanceState_rch());

  ~ReceivedDataElementList();

  void apply_all(ReceivedDataFilter& match, ReceivedDataOperation& func);

  // adds a data sample to the end of the list
  void add(ReceivedDataElement* data_sample);
  void add_by_timestamp(ReceivedDataElement* data_sample);

  // returns true if the instance was released
  bool remove(ReceivedDataElement* data_sample);

  // returns true if the instance was released
  bool remove(ReceivedDataFilter& match, bool eval_all);

  const ReceivedDataElement* peek_tail() { return tail_; }

  ReceivedDataElement* remove_head();
  ReceivedDataElement* remove_tail();

  size_t size() const { return size_; }

  bool has_zero_copies() const;
  bool matches(CORBA::ULong sample_states) const;
  ReceivedDataElement* get_next_match(CORBA::ULong sample_states, ReceivedDataElement* prev);

  void mark_read(ReceivedDataElement* item);
#ifndef OPENDDS_NO_OBJECT_MODEL_PROFILE
  void accept_coherent_change(ReceivedDataElement* item);
#endif

private:
  DataReaderImpl* reader_;

  /// The first element of the list.
  ReceivedDataElement* head_;

  /// The last element of the list.
  ReceivedDataElement* tail_;

  /// Number of elements in the list.
  size_t size_;

  CORBA::ULong read_sample_count_;
  CORBA::ULong not_read_sample_count_;
  CORBA::ULong sample_states_;

  void increment_read_count();
  void decrement_read_count();
  void increment_not_read_count();
  void decrement_not_read_count();
  InstanceState_rch instance_state_;

  bool sanity_check();
  bool sanity_check(ReceivedDataElement* item);
}; // ReceivedDataElementList

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#if defined (__ACE_INLINE__)
# include "ReceivedDataElementList.inl"
#endif  /* __ACE_INLINE__ */

#endif /* OPENDDS_DCPS_RECEIVEDDATAELEMENTLIST_H  */
