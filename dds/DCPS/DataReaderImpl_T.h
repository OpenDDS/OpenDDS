#ifndef OPENDDS_DCPS_DATAREADERIMPL_T_H
#define OPENDDS_DCPS_DATAREADERIMPL_T_H

#include <ace/config-lite.h>

#ifdef ACE_HAS_CPP11
#  define OPENDDS_HAS_STD_SHARED_PTR
#endif

#include "MultiTopicImpl.h"
#include "RakeResults_T.h"
#include "SubscriberImpl.h"
#include "BuiltInTopicUtils.h"
#include "Util.h"
#include "TypeSupportImpl.h"
#include "dcps_export.h"
#include "GuidConverter.h"
#include "XTypes/DynamicDataAdapter.h"

#ifndef OPENDDS_HAS_STD_SHARED_PTR
#  include <ace/Bound_Ptr.h>
#endif
#include <ace/Time_Value.h>

#ifndef OPENDDS_HAS_STD_SHARED_PTR
#  include <memory>
#endif

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
  namespace DCPS {

  /** Servant for DataReader interface of Traits::MessageType data type.
   *
   * See the DDS specification, OMG formal/2015-04-10, for a description of
   * this interface.
   *
   */
  template <typename MessageType>
  class
#if ( __GNUC__ == 4 && __GNUC_MINOR__ == 1)
    OpenDDS_Dcps_Export
#endif
  DataReaderImpl_T
    : public virtual LocalObject<typename DDSTraits<MessageType>::DataReaderType>
    , public virtual DataReaderImpl
    , public ValueWriterDispatcher
  {
  public:
    typedef DDSTraits<MessageType> TraitsType;
    typedef MarshalTraits<MessageType> MarshalTraitsType;
    typedef typename TraitsType::MessageSequenceType MessageSequenceType;

    typedef OPENDDS_MAP_CMP_T(MessageType, DDS::InstanceHandle_t,
                              typename TraitsType::LessThanType) InstanceMap;
    typedef OPENDDS_MAP(DDS::InstanceHandle_t, typename InstanceMap::iterator) ReverseInstanceMap;

    class SharedInstanceMap
      : public virtual RcObject
      , public InstanceMap
    {
    };

    typedef RcHandle<SharedInstanceMap> SharedInstanceMap_rch;

    class MessageTypeWithAllocator
      : public MessageType
      , public EnableContainerSupportedUniquePtr<MessageTypeWithAllocator>
    {
    public:
      void* operator new(size_t size, ACE_New_Allocator& pool);
      void operator delete(void* memory, ACE_New_Allocator& pool);
      void operator delete(void* memory);

      MessageTypeWithAllocator(){}
      MessageTypeWithAllocator(const MessageType& other)
        : MessageType(other)
      {
      }

      const MessageType* message() const { return this; }
    };

    struct MessageTypeMemoryBlock {
      MessageTypeWithAllocator element_;
      ACE_New_Allocator* allocator_;
    };

    typedef OpenDDS::DCPS::Cached_Allocator_With_Overflow<MessageTypeMemoryBlock, ACE_Thread_Mutex>  DataAllocator;

    typedef typename TraitsType::DataReaderType Interface;

    DataReaderImpl_T (void)
      : filter_delayed_sample_task_(make_rch<DRISporadicTask>(TheServiceParticipant->time_source(), TheServiceParticipant->interceptor(), rchandle_from(this), &DataReaderImpl_T::filter_delayed))
      , marshal_skip_serialize_(false)
    {
      initialize_lookup_maps();
    }

    virtual ~DataReaderImpl_T (void)
    {
      filter_delayed_sample_task_->cancel();

      for (typename InstanceMap::iterator it = instance_map_.begin();
           it != instance_map_.end(); ++it)
        {
          OpenDDS::DCPS::SubscriptionInstance_rch ptr = get_handle_instance(it->second);
          if (!ptr) continue;
          purge_data(ptr);
        }
      //X SHH release the data samples in the instance_map_.
    }

    /**
     * Do parts of enable specific to the datatype.
     * Called by DataReaderImpl::enable().
     */
    virtual DDS::ReturnCode_t enable_specific ()
    {
      data_allocator().reset(new DataAllocator(get_n_chunks ()));
      if (OpenDDS::DCPS::DCPS_debug_level >= 2)
        ACE_DEBUG((LM_DEBUG,
                   ACE_TEXT("(%P|%t) %CDataReaderImpl::")
                   ACE_TEXT("enable_specific-data")
                   ACE_TEXT(" Cached_Allocator_With_Overflow ")
                   ACE_TEXT("%x with %d chunks\n"),
                   TraitsType::type_name(),
                   data_allocator().get(),
                   get_n_chunks ()));

      return DDS::RETCODE_OK;
    }

    virtual const ValueWriterDispatcher* get_value_writer_dispatcher() const { return this; }

    void write(ValueWriter& value_writer, const void* data) const
    {
      vwrite(value_writer, *static_cast<const MessageType*>(data));
    }

    virtual DDS::ReturnCode_t read (
                                    MessageSequenceType & received_data,
                                    DDS::SampleInfoSeq & info_seq,
                                    ::CORBA::Long max_samples,
                                    DDS::SampleStateMask sample_states,
                                    DDS::ViewStateMask view_states,
                                    DDS::InstanceStateMask instance_states)
    {
      DDS::ReturnCode_t const precond =
        check_inputs("read", received_data, info_seq, max_samples);
      if (DDS::RETCODE_OK != precond)
        {
          return precond;
        }

      ACE_GUARD_RETURN (ACE_Recursive_Thread_Mutex,
                        guard,
                        sample_lock_,
                        DDS::RETCODE_ERROR);

      return read_i(received_data, info_seq, max_samples, sample_states,
                    view_states, instance_states, 0);
    }

    virtual DDS::ReturnCode_t take (
                                      MessageSequenceType & received_data,
                                      DDS::SampleInfoSeq & info_seq,
                                      ::CORBA::Long max_samples,
                                      DDS::SampleStateMask sample_states,
                                      DDS::ViewStateMask view_states,
                                      DDS::InstanceStateMask instance_states)
    {
      DDS::ReturnCode_t const precond =
        check_inputs("take", received_data, info_seq, max_samples);
      if (DDS::RETCODE_OK != precond)
        {
          return precond;
        }

      ACE_GUARD_RETURN (ACE_Recursive_Thread_Mutex,
                        guard,
                        sample_lock_,
                        DDS::RETCODE_ERROR);

      return take_i(received_data, info_seq, max_samples, sample_states,
                    view_states, instance_states, 0);
    }

    virtual DDS::ReturnCode_t read_w_condition (
                                                  MessageSequenceType & received_data,
                                                  DDS::SampleInfoSeq & sample_info,
                                                  ::CORBA::Long max_samples,
                                                  DDS::ReadCondition_ptr a_condition)
    {
      DDS::ReturnCode_t const precond =
        check_inputs("read_w_condition", received_data, sample_info, max_samples);
      if (DDS::RETCODE_OK != precond)
        {
          return precond;
        }

      ACE_GUARD_RETURN (ACE_Recursive_Thread_Mutex, guard, sample_lock_,
                        DDS::RETCODE_ERROR);

      if (!has_readcondition(a_condition))
        {
          return DDS::RETCODE_PRECONDITION_NOT_MET;
        }

      return read_i(received_data, sample_info, max_samples,
                    a_condition->get_sample_state_mask(),
                    a_condition->get_view_state_mask(),
                    a_condition->get_instance_state_mask(),
#ifndef OPENDDS_NO_QUERY_CONDITION
                    dynamic_cast< DDS::QueryCondition_ptr >(a_condition));
#else
      0);
#endif
  }

    virtual DDS::ReturnCode_t take_w_condition (
                                                  MessageSequenceType & received_data,
                                                  DDS::SampleInfoSeq & sample_info,
                                                  ::CORBA::Long max_samples,
                                                  DDS::ReadCondition_ptr a_condition)
    {
      DDS::ReturnCode_t const precond =
        check_inputs("take_w_condition", received_data, sample_info, max_samples);
      if (DDS::RETCODE_OK != precond)
        {
          return precond;
        }

      ACE_GUARD_RETURN (ACE_Recursive_Thread_Mutex, guard, sample_lock_,
                        DDS::RETCODE_ERROR);

      if (!has_readcondition(a_condition))
        {
          return DDS::RETCODE_PRECONDITION_NOT_MET;
        }

      return take_i(received_data, sample_info, max_samples,
                    a_condition->get_sample_state_mask(),
                    a_condition->get_view_state_mask(),
                    a_condition->get_instance_state_mask(),
#ifndef OPENDDS_NO_QUERY_CONDITION
                    dynamic_cast< DDS::QueryCondition_ptr >(a_condition)
#else
                    0
#endif
                    );
    }

  virtual DDS::ReturnCode_t read_next_sample(MessageType& received_data,
                                             DDS::SampleInfo& sample_info_ref)
  {
    bool found_data = false;
    ACE_GUARD_RETURN(ACE_Recursive_Thread_Mutex, guard, sample_lock_, DDS::RETCODE_ERROR);

    const Observer_rch observer = get_observer(Observer::e_SAMPLE_READ);

    const CORBA::ULong sample_states = DDS::NOT_READ_SAMPLE_STATE;
    const HandleSet& matches = lookup_matching_instances(sample_states, DDS::ANY_VIEW_STATE, DDS::ANY_INSTANCE_STATE);
    for (HandleSet::const_iterator it = matches.begin(), next = it; it != matches.end(); it = next) {
      ++next; // pre-increment iterator, in case updates cause changes to match set
      const DDS::InstanceHandle_t handle = *it;
      const SubscriptionInstance_rch inst = get_handle_instance(handle);
      if (!inst) continue;

      bool most_recent_generation = false;
      for (ReceivedDataElement* item = inst->rcvd_samples_.get_next_match(sample_states, 0);
           !found_data && item; item = inst->rcvd_samples_.get_next_match(sample_states, item)) {
        if (item->registered_data_) {
          received_data = *static_cast<MessageType*>(item->registered_data_);
        }
        inst->instance_state_->sample_info(sample_info_ref, item);
        inst->rcvd_samples_.mark_read(item);

        const ValueWriterDispatcher* vwd = get_value_writer_dispatcher();
        if (observer && item->registered_data_ && vwd) {
          Observer::Sample s(sample_info_ref.instance_handle, sample_info_ref.instance_state, *item, *vwd);
          observer->on_sample_read(this, s);
        }

        if (!most_recent_generation) {
          most_recent_generation = inst->instance_state_->most_recent_generation(item);
        }

        found_data = true;
      }

      if (found_data) {
        if (most_recent_generation) {
          inst->instance_state_->accessed();
        }
        // Get the sample_ranks, generation_ranks, and
        // absolute_generation_ranks for this info_seq
        sample_info(sample_info_ref, inst->rcvd_samples_.peek_tail());

        break;
      }
    }

    post_read_or_take();
    return found_data ? DDS::RETCODE_OK : DDS::RETCODE_NO_DATA;
  }

  virtual DDS::ReturnCode_t take_next_sample(MessageType& received_data,
                                             DDS::SampleInfo& sample_info_ref)
  {
    bool found_data = false;
    ACE_GUARD_RETURN(ACE_Recursive_Thread_Mutex, guard, sample_lock_, DDS::RETCODE_ERROR);

    const Observer_rch observer = get_observer(Observer::e_SAMPLE_TAKEN);

    const CORBA::ULong sample_states = DDS::NOT_READ_SAMPLE_STATE;
    const HandleSet& matches = lookup_matching_instances(sample_states, DDS::ANY_VIEW_STATE, DDS::ANY_INSTANCE_STATE);
    for (HandleSet::const_iterator it = matches.begin(), next = it; it != matches.end(); it = next) {
      ++next; // pre-increment iterator, in case updates cause changes to match set
      const DDS::InstanceHandle_t handle = *it;
      const SubscriptionInstance_rch inst = get_handle_instance(handle);
      if (!inst) continue;

      bool most_recent_generation = false;
      ReceivedDataElement* item = inst->rcvd_samples_.get_next_match(sample_states, 0);
      if (item) {
        if (item->registered_data_) {
          received_data = *static_cast<MessageType*>(item->registered_data_);
        }
        inst->instance_state_->sample_info(sample_info_ref, item);
        inst->rcvd_samples_.mark_read(item);

        const ValueWriterDispatcher* vwd = get_value_writer_dispatcher();
        if (observer && item->registered_data_ && vwd) {
          Observer::Sample s(sample_info_ref.instance_handle, sample_info_ref.instance_state, *item, *vwd);
          observer->on_sample_taken(this, s);
        }

        if (!most_recent_generation) {
          most_recent_generation = inst->instance_state_->most_recent_generation(item);
        }

        if (most_recent_generation) {
          inst->instance_state_->accessed();
        }

        // Get the sample_ranks, generation_ranks, and
        // absolute_generation_ranks for this info_seq
        sample_info(sample_info_ref, inst->rcvd_samples_.peek_tail());

        inst->rcvd_samples_.remove(item);
        item->dec_ref();
        item = 0;

        found_data = true;

        break;
      }
    }

    post_read_or_take();
    return found_data ? DDS::RETCODE_OK : DDS::RETCODE_NO_DATA;
  }

  virtual DDS::ReturnCode_t read_instance (
                                             MessageSequenceType & received_data,
                                             DDS::SampleInfoSeq & info_seq,
                                             ::CORBA::Long max_samples,
                                             DDS::InstanceHandle_t a_handle,
                                             DDS::SampleStateMask sample_states,
                                             DDS::ViewStateMask view_states,
                                             DDS::InstanceStateMask instance_states)
  {
    DDS::ReturnCode_t const precond =
      check_inputs("read_instance", received_data, info_seq, max_samples);
    if (DDS::RETCODE_OK != precond)
      {
        return precond;
      }

    ACE_GUARD_RETURN (ACE_Recursive_Thread_Mutex,
                      guard,
                      sample_lock_,
                      DDS::RETCODE_ERROR);
    return read_instance_i(received_data, info_seq, max_samples, a_handle,
                           sample_states, view_states, instance_states, 0);
  }

  virtual DDS::ReturnCode_t take_instance (
                                             MessageSequenceType & received_data,
                                             DDS::SampleInfoSeq & info_seq,
                                             ::CORBA::Long max_samples,
                                             DDS::InstanceHandle_t a_handle,
                                             DDS::SampleStateMask sample_states,
                                             DDS::ViewStateMask view_states,
                                             DDS::InstanceStateMask instance_states)
  {
    DDS::ReturnCode_t const precond =
      check_inputs("take_instance", received_data, info_seq, max_samples);
    if (DDS::RETCODE_OK != precond)
      {
        return precond;
      }

    ACE_GUARD_RETURN (ACE_Recursive_Thread_Mutex,
                      guard,
                      sample_lock_,
                      DDS::RETCODE_ERROR);
    return take_instance_i(received_data, info_seq, max_samples, a_handle,
                           sample_states, view_states, instance_states, 0);
  }

  virtual DDS::ReturnCode_t read_instance_w_condition (
                                                       MessageSequenceType & received_data,
                                                       DDS::SampleInfoSeq & info_seq,
                                                       ::CORBA::Long max_samples,
                                                       DDS::InstanceHandle_t a_handle,
                                                       DDS::ReadCondition_ptr a_condition)
  {
    DDS::ReturnCode_t const precond =
      check_inputs("read_instance_w_condition", received_data, info_seq,
                   max_samples);
    if (DDS::RETCODE_OK != precond)
      {
        return precond;
      }

    ACE_GUARD_RETURN (ACE_Recursive_Thread_Mutex, guard, sample_lock_,
                      DDS::RETCODE_ERROR);

    if (!has_readcondition(a_condition))
      {
        return DDS::RETCODE_PRECONDITION_NOT_MET;
      }

#ifndef OPENDDS_NO_QUERY_CONDITION
    DDS::QueryCondition_ptr query_condition =
        dynamic_cast< DDS::QueryCondition_ptr >(a_condition);
#endif

    return read_instance_i(received_data, info_seq, max_samples, a_handle,
                           a_condition->get_sample_state_mask(),
                           a_condition->get_view_state_mask(),
                           a_condition->get_instance_state_mask(),
#ifndef OPENDDS_NO_QUERY_CONDITION
                           query_condition
#else
                           0
#endif
                           );
  }

  virtual DDS::ReturnCode_t take_instance_w_condition (
                                                       MessageSequenceType & received_data,
                                                       DDS::SampleInfoSeq & info_seq,
                                                       ::CORBA::Long max_samples,
                                                       DDS::InstanceHandle_t a_handle,
                                                       DDS::ReadCondition_ptr a_condition)
  {
    DDS::ReturnCode_t const precond =
      check_inputs("take_instance_w_condition", received_data, info_seq,
                   max_samples);
    if (DDS::RETCODE_OK != precond)
      {
        return precond;
      }

    ACE_GUARD_RETURN (ACE_Recursive_Thread_Mutex, guard, sample_lock_,
                      DDS::RETCODE_ERROR);

    if (!has_readcondition(a_condition))
      {
        return DDS::RETCODE_PRECONDITION_NOT_MET;
      }

#ifndef OPENDDS_NO_QUERY_CONDITION
    DDS::QueryCondition_ptr query_condition =
        dynamic_cast< DDS::QueryCondition_ptr >(a_condition);
#endif

    return take_instance_i(received_data, info_seq, max_samples, a_handle,
                           a_condition->get_sample_state_mask(),
                           a_condition->get_view_state_mask(),
                           a_condition->get_instance_state_mask(),
#ifndef OPENDDS_NO_QUERY_CONDITION
                           query_condition
#else
                           0
#endif
                           );
  }

  virtual DDS::ReturnCode_t read_next_instance (
                                                  MessageSequenceType & received_data,
                                                  DDS::SampleInfoSeq & info_seq,
                                                  ::CORBA::Long max_samples,
                                                  DDS::InstanceHandle_t a_handle,
                                                  DDS::SampleStateMask sample_states,
                                                  DDS::ViewStateMask view_states,
                                                  DDS::InstanceStateMask instance_states)
  {
    DDS::ReturnCode_t const precond =
      check_inputs("read_next_instance", received_data, info_seq, max_samples);
    if (DDS::RETCODE_OK != precond)
      {
        return precond;
      }

    return read_next_instance_i(received_data, info_seq, max_samples, a_handle,
                                sample_states, view_states, instance_states, 0);
  }

  virtual DDS::ReturnCode_t take_next_instance (
                                                  MessageSequenceType & received_data,
                                                  DDS::SampleInfoSeq & info_seq,
                                                  ::CORBA::Long max_samples,
                                                  DDS::InstanceHandle_t a_handle,
                                                  DDS::SampleStateMask sample_states,
                                                  DDS::ViewStateMask view_states,
                                                  DDS::InstanceStateMask instance_states)
  {
    DDS::ReturnCode_t const precond =
      check_inputs("take_next_instance", received_data, info_seq, max_samples);
    if (DDS::RETCODE_OK != precond)
      {
        return precond;
      }

    return take_next_instance_i(received_data, info_seq, max_samples, a_handle,
                                sample_states, view_states, instance_states, 0);
  }

  virtual DDS::ReturnCode_t read_next_instance_w_condition (
                                                              MessageSequenceType & received_data,
                                                              DDS::SampleInfoSeq & info_seq,
                                                              ::CORBA::Long max_samples,
                                                              DDS::InstanceHandle_t a_handle,
                                                              DDS::ReadCondition_ptr a_condition)
  {
    DDS::ReturnCode_t const precond =
      check_inputs("read_next_instance_w_condition", received_data, info_seq,
                   max_samples);
    if (DDS::RETCODE_OK != precond)
      {
        return precond;
      }

    ACE_GUARD_RETURN (ACE_Recursive_Thread_Mutex, guard, sample_lock_,
                      DDS::RETCODE_ERROR);

    if (!has_readcondition(a_condition))
      {
        return DDS::RETCODE_PRECONDITION_NOT_MET;
      }

#ifndef OPENDDS_NO_QUERY_CONDITION
    DDS::QueryCondition_ptr query_condition =
        dynamic_cast< DDS::QueryCondition_ptr >(a_condition);
#endif

    return read_next_instance_i(received_data, info_seq, max_samples, a_handle,
                                a_condition->get_sample_state_mask(),
                                a_condition->get_view_state_mask(),
                                a_condition->get_instance_state_mask(),
#ifndef OPENDDS_NO_QUERY_CONDITION
                                query_condition
#else
                                0
#endif
                                );
  }

  virtual DDS::ReturnCode_t take_next_instance_w_condition (
                                                              MessageSequenceType & received_data,
                                                              DDS::SampleInfoSeq & info_seq,
                                                              ::CORBA::Long max_samples,
                                                              DDS::InstanceHandle_t a_handle,
                                                              DDS::ReadCondition_ptr a_condition)
  {
    DDS::ReturnCode_t const precond =
      check_inputs("take_next_instance_w_condition", received_data, info_seq,
                   max_samples);
    if (DDS::RETCODE_OK != precond)
      {
        return precond;
      }

    ACE_GUARD_RETURN (ACE_Recursive_Thread_Mutex, guard, sample_lock_,
                      DDS::RETCODE_ERROR);

    if (!has_readcondition(a_condition))
      {
        return DDS::RETCODE_PRECONDITION_NOT_MET;
      }

#ifndef OPENDDS_NO_QUERY_CONDITION
    DDS::QueryCondition_ptr query_condition =
        dynamic_cast< DDS::QueryCondition_ptr >(a_condition);
#endif

    return take_next_instance_i(received_data, info_seq, max_samples, a_handle,
                                a_condition->get_sample_state_mask(),
                                a_condition->get_view_state_mask(),
                                a_condition->get_instance_state_mask(),
#ifndef OPENDDS_NO_QUERY_CONDITION
                                query_condition
#else
                                0
#endif
                                );
  }

  virtual DDS::ReturnCode_t return_loan (
                                           MessageSequenceType & received_data,
                                           DDS::SampleInfoSeq & info_seq)
  {
    // Some incomplete tests to see that the data and info are from the
    // same read.
    if (received_data.length() != info_seq.length())
      {
        return DDS::RETCODE_PRECONDITION_NOT_MET;
      }

    if (received_data.release())
      {
        // nothing to do because this is not zero-copy data
        return DDS::RETCODE_OK;
      }
    else
      {
        info_seq.length(0);
        received_data.length(0);
      }
    return DDS::RETCODE_OK;
  }

  virtual DDS::ReturnCode_t get_key_value(MessageType& key_holder,
                                          DDS::InstanceHandle_t handle)
  {
    ACE_Guard<ACE_Recursive_Thread_Mutex> guard(sample_lock_);

    const typename ReverseInstanceMap::const_iterator pos = reverse_instance_map_.find(handle);
    if (pos != reverse_instance_map_.end()) {
      key_holder = pos->second->first;
      return DDS::RETCODE_OK;
    }

    return DDS::RETCODE_BAD_PARAMETER;
  }

  virtual DDS::InstanceHandle_t lookup_instance(const MessageType& instance_data)
  {
    ACE_Guard<ACE_Recursive_Thread_Mutex> guard(sample_lock_);

    const typename InstanceMap::const_iterator it = instance_map_.find(instance_data);
    if (it != instance_map_.end()) {
      return it->second;
    }
    return DDS::HANDLE_NIL;
  }

  virtual DDS::ReturnCode_t auto_return_loan(void* seq)
  {
    MessageSequenceType& received_data =
      *static_cast< MessageSequenceType*> (seq);

    if (!received_data.release())
      {
        // release_loan(received_data);
        received_data.length(0);
      }
    return DDS::RETCODE_OK;
  }

  void release_loan (MessageSequenceType & received_data)
  {
    received_data.length(0);
  }

#ifndef OPENDDS_NO_CONTENT_SUBSCRIPTION_PROFILE
  bool contains_sample_filtered(DDS::SampleStateMask sample_states,
                                DDS::ViewStateMask view_states,
                                DDS::InstanceStateMask instance_states,
                                const OpenDDS::DCPS::FilterEvaluator& evaluator,
                                const DDS::StringSeq& params)
  {
    using namespace OpenDDS::DCPS;
    ACE_GUARD_RETURN(ACE_Recursive_Thread_Mutex, guard, sample_lock_, false);
    ACE_GUARD_RETURN(ACE_Recursive_Thread_Mutex, instance_guard, instances_lock_, false);

    const bool filter_has_non_key_fields =
      evaluator.has_non_key_fields(getMetaStruct<MessageType>());

    const HandleSet& matches = lookup_matching_instances(sample_states, view_states, instance_states);
    for (HandleSet::const_iterator it = matches.begin(), next = it; it != matches.end(); it = next) {
      ++next; // pre-increment iterator, in case updates cause changes to match set
      const DDS::InstanceHandle_t handle = *it;
      const SubscriptionInstance_rch inst = get_handle_instance(handle);
      if (!inst) continue;

      for (ReceivedDataElement* item = inst->rcvd_samples_.get_next_match(sample_states, 0); item;
           item = inst->rcvd_samples_.get_next_match(sample_states, item)) {
        if (!item->registered_data_ || (!item->valid_data_ && filter_has_non_key_fields)) {
          continue;
        }
        if (evaluator.eval(*static_cast<MessageType*>(item->registered_data_), params)) {
          return true;
        }
      }
    }

    return false;
  }

  DDS::ReturnCode_t read_generic(
                                   OpenDDS::DCPS::DataReaderImpl::GenericBundle& gen,
                                   DDS::SampleStateMask sample_states, DDS::ViewStateMask view_states,
                                   DDS::InstanceStateMask instance_states,
                                   bool adjust_ref_count=false)
  {

    MessageSequenceType data;
    DDS::ReturnCode_t rc;
    ACE_GUARD_RETURN (ACE_Recursive_Thread_Mutex,
                      guard,
                      sample_lock_,
                      DDS::RETCODE_ERROR);
    {
      rc = read_i(data, gen.info_,
                  DDS::LENGTH_UNLIMITED,
                  sample_states, view_states, instance_states, 0);
      if (true == adjust_ref_count ) {
        data.increment_references();
      }
    }
    gen.samples_.reserve(data.length());
    for (CORBA::ULong i = 0; i < data.length(); ++i) {
      gen.samples_.push_back(&data[i]);
    }
    return rc;

  }

  DDS::InstanceHandle_t lookup_instance_generic(const void* data)
  {
    return lookup_instance(*static_cast<const MessageType*>(data));
  }

  virtual DDS::ReturnCode_t take(
                                 OpenDDS::DCPS::AbstractSamples& samples,
                                 DDS::SampleStateMask sample_states, DDS::ViewStateMask view_states,
                                 DDS::InstanceStateMask instance_states)
  {
    ACE_GUARD_RETURN (ACE_Recursive_Thread_Mutex,
                      guard,
                      sample_lock_,
                      DDS::RETCODE_ERROR);

    MessageSequenceType data;
    DDS::SampleInfoSeq infos;
    DDS::ReturnCode_t rc = take_i(data, infos, DDS::LENGTH_UNLIMITED,
                                  sample_states, view_states, instance_states, 0);

    samples.reserve(data.length());

    for (CORBA::ULong i = 0; i < data.length(); ++i) {
      samples.push_back(infos[i], &data[i]);
    }

    return rc;
  }

  DDS::ReturnCode_t read_instance_generic(void*& data,
                                          DDS::SampleInfo& info, DDS::InstanceHandle_t instance,
                                          DDS::SampleStateMask sample_states, DDS::ViewStateMask view_states,
                                          DDS::InstanceStateMask instance_states)
  {
    MessageSequenceType dataseq;
    DDS::SampleInfoSeq infoseq;
    const DDS::ReturnCode_t rc = read_instance_i(dataseq, infoseq,
                                                 DDS::LENGTH_UNLIMITED, instance, sample_states, view_states,
                                                 instance_states, 0);
    if (rc != DDS::RETCODE_NO_DATA)
      {
        const CORBA::ULong last = dataseq.length() - 1;
        data = new MessageType(dataseq[last]);
        info = infoseq[last];
      }
    return rc;
  }

  DDS::ReturnCode_t read_next_instance_generic(void*& data,
                                               DDS::SampleInfo& info, DDS::InstanceHandle_t previous_instance,
                                               DDS::SampleStateMask sample_states, DDS::ViewStateMask view_states,
                                               DDS::InstanceStateMask instance_states)
  {
    MessageSequenceType dataseq;
    DDS::SampleInfoSeq infoseq;
    const DDS::ReturnCode_t rc = read_next_instance_i(dataseq, infoseq,
                                                      DDS::LENGTH_UNLIMITED, previous_instance, sample_states, view_states,
                                                      instance_states, 0);
    if (rc != DDS::RETCODE_NO_DATA)
      {
        const CORBA::ULong last = dataseq.length() - 1;
        data = new MessageType(dataseq[last]);
        info = infoseq[last];
      }
    return rc;
  }

#endif

  DDS::InstanceHandle_t store_synthetic_data(const MessageType& sample,
                                             DDS::ViewStateKind view,
                                             const SystemTimePoint& timestamp = SystemTimePoint::now())
  {
    using namespace OpenDDS::DCPS;
    ACE_GUARD_RETURN(ACE_Recursive_Thread_Mutex, guard, sample_lock_,
                     DDS::HANDLE_NIL);
#ifndef OPENDDS_NO_MULTI_TOPIC
    DDS::TopicDescription_var descr = get_topicdescription();
    if (MultiTopicImpl* mt = dynamic_cast<MultiTopicImpl*>(descr.in())) {
      if (!mt->filter(sample)) {
        return DDS::HANDLE_NIL;
      }
    }
#endif

    get_subscriber_servant()->data_received(this);

    DDS::InstanceHandle_t inst = lookup_instance(sample);
    bool filtered = false;
    SubscriptionInstance_rch instance;

    const DDS::Time_t now = timestamp.to_dds_time();
    DataSampleHeader header;
    header.source_timestamp_sec_ = now.sec;
    header.source_timestamp_nanosec_ = now.nanosec;

    // Call store_instance_data() once or twice, depending on if we need to
    // process the INSTANCE_REGISTRATION.  In either case, store_instance_data()
    // owns the memory for the sample and it must come from the correct allocator.
    for (int i = 0; i < 2; ++i) {
      if (i == 0 && inst != DDS::HANDLE_NIL) continue;

      const int msg = i ? SAMPLE_DATA : INSTANCE_REGISTRATION;
      header.message_id_ = static_cast<char>(msg);

      bool just_registered;
      unique_ptr<MessageTypeWithAllocator> data(new (*data_allocator()) MessageTypeWithAllocator(sample));
      store_instance_data(move(data), DDS::HANDLE_NIL, header, instance, just_registered, filtered);
      if (instance) inst = instance->instance_handle_;
    }

    if (!filtered) {
      if (view == DDS::NOT_NEW_VIEW_STATE) {
        if (instance) instance->instance_state_->accessed();
      }
      notify_read_conditions();
    }

    const ValueWriterDispatcher* vwd = get_value_writer_dispatcher();
    const Observer_rch observer = get_observer(Observer::e_SAMPLE_RECEIVED);
    if (observer && vwd) {
      Observer::Sample s(instance ? instance->instance_handle_ : DDS::HANDLE_NIL, header.instance_state(), now, header.sequence_, &sample, *vwd);
      observer->on_sample_received(this, s);
    }

    return inst;
  }

  Extensibility get_max_extensibility()
  {
    return MarshalTraitsType::max_extensibility_level();
  }

  void set_instance_state_i(DDS::InstanceHandle_t instance,
                            DDS::InstanceHandle_t publication_handle,
                            DDS::InstanceStateKind state,
                            const SystemTimePoint& timestamp,
                            const GUID_t& publication_id)
  {
    // sample_lock_ must be held.
    using namespace OpenDDS::DCPS;

    SubscriptionInstance_rch si = get_handle_instance(instance);
    if (si && state != DDS::ALIVE_INSTANCE_STATE) {
      const DDS::Time_t now = timestamp.to_dds_time();
      DataSampleHeader header;
      header.publication_id_ = publication_id;
      header.source_timestamp_sec_ = now.sec;
      header.source_timestamp_nanosec_ = now.nanosec;
      const int msg = (state == DDS::NOT_ALIVE_DISPOSED_INSTANCE_STATE)
        ? DISPOSE_INSTANCE : UNREGISTER_INSTANCE;
      header.message_id_ = static_cast<char>(msg);
      bool just_registered, filtered;
      unique_ptr<MessageTypeWithAllocator> data(new (*data_allocator()) MessageTypeWithAllocator);
      get_key_value(*data, instance);
      store_instance_data(move(data), publication_handle, header, si, just_registered, filtered);
      if (!filtered) {
        notify_read_conditions();
      }
    }
  }

  virtual void lookup_instance(const OpenDDS::DCPS::ReceivedDataSample& sample,
                               OpenDDS::DCPS::SubscriptionInstance_rch& instance)
  {
    //!!! caller should already have the sample_lock_
    const bool encapsulated = sample.header_.cdr_encapsulation_;
    OpenDDS::DCPS::Serializer ser(
      sample.sample_.get(),
      encapsulated ? Encoding::KIND_XCDR1 : Encoding::KIND_UNALIGNED_CDR,
      static_cast<Endianness>(sample.header_.byte_order_));

    if (encapsulated) {
      EncapsulationHeader encap;
      if (!(ser >> encap)) {
        if (DCPS_debug_level > 0) {
          ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR ")
            ACE_TEXT("%CDataReaderImpl::lookup_instance: ")
            ACE_TEXT("deserialization of encapsulation header failed.\n"),
            TraitsType::type_name()));
        }
        return;
      }
      Encoding encoding;
      if (!encap.to_encoding(encoding, MarshalTraitsType::extensibility())) {
        return;
      }

      if (decoding_modes_.find(encoding.kind()) == decoding_modes_.end()) {
        if (DCPS_debug_level >= 1) {
          ACE_DEBUG((LM_WARNING, ACE_TEXT("(%P|%t) WARNING ")
            ACE_TEXT("%CDataReaderImpl::lookup_instance: ")
            ACE_TEXT("Encoding kind of the received sample (%C) does not ")
            ACE_TEXT("match the ones specified by DataReader.\n"),
            TraitsType::type_name(),
            Encoding::kind_to_string(encoding.kind()).c_str()));
        }
        return;
      }
      if (DCPS_debug_level >= 8) {
        ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) ")
          ACE_TEXT("%CDataReaderImpl::lookup_instance: ")
          ACE_TEXT("Deserializing with encoding kind %C.\n"),
          TraitsType::type_name(),
          Encoding::kind_to_string(encoding.kind()).c_str()));
      }

      ser.encoding(encoding);
    }

    bool ser_ret = true;
    MessageType data;
    if (sample.header_.key_fields_only_) {
      ser_ret = ser >> OpenDDS::DCPS::KeyOnly<MessageType>(data);
    } else {
      ser_ret = ser >> data;
    }
    if (!ser_ret) {
      if (ser.get_construction_status() != Serializer::ConstructionSuccessful) {
        if (DCPS_debug_level > 1) {
          ACE_DEBUG((LM_WARNING, ACE_TEXT("(%P|%t) %CDataReaderImpl::lookup_instance ")
                     ACE_TEXT("object construction failure, dropping sample.\n"),
                     TraitsType::type_name()));
        }
      } else {
        if (DCPS_debug_level > 0) {
          ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) %CDataReaderImpl::lookup_instance ")
                    ACE_TEXT("deserialization failed.\n"),
                    TraitsType::type_name()));
        }
      }
      return;
    }

    DDS::InstanceHandle_t handle(DDS::HANDLE_NIL);
    typename InstanceMap::const_iterator const it = instance_map_.find(data);
    if (it != instance_map_.end()) {
      handle = it->second;
    }

    if (handle == DDS::HANDLE_NIL) {
      instance.reset();
    } else {
      instance = get_handle_instance(handle);
    }
  }

  virtual void qos_change(const DDS::DataReaderQos& qos)
  {
    // reliability is not changeable, just time_based_filter
    if (qos.reliability.kind == DDS::RELIABLE_RELIABILITY_QOS) {
      if (qos.time_based_filter.minimum_separation != qos_.time_based_filter.minimum_separation) {
        const DDS::Duration_t zero = { DDS::DURATION_ZERO_SEC, DDS::DURATION_ZERO_NSEC };
        if (qos_.time_based_filter.minimum_separation != zero) {
          if (qos.time_based_filter.minimum_separation != zero) {
            const MonotonicTimePoint now = MonotonicTimePoint::now();
            const TimeDuration interval(qos_.time_based_filter.minimum_separation);
            FilterDelayedSampleQueue queue;

            ACE_GUARD(ACE_Recursive_Thread_Mutex, guard, sample_lock_);
            for (typename FilterDelayedSampleMap::iterator pos = filter_delayed_sample_map_.begin(), limit = filter_delayed_sample_map_.end(); pos != limit; ++pos) {
              FilterDelayedSample& sample = pos->second;
              sample.expiration_time = now + (interval - (sample.expiration_time - now));
              queue.insert(std::make_pair(sample.expiration_time, pos->first));
            }
            std::swap(queue, filter_delayed_sample_queue_);

            if (!filter_delayed_sample_queue_.empty()) {
              filter_delayed_sample_task_->cancel();
              filter_delayed_sample_task_->schedule(interval);
            }

          } else {
            filter_delayed_sample_task_->cancel();
            ACE_GUARD(ACE_Recursive_Thread_Mutex, guard, sample_lock_);
            filter_delayed_sample_map_.clear();
            filter_delayed_sample_queue_.clear();
          }
        }
        // else no existing timers to change/cancel
      }
      // else no qos change so nothing to change
    }

    DataReaderImpl::qos_change(qos);
  }

  void set_marshal_skip_serialize(bool value)
  {
    marshal_skip_serialize_ = value;
  }

  bool get_marshal_skip_serialize() const
  {
    return marshal_skip_serialize_;
  }

  void release_all_instances()
  {
    ACE_GUARD(ACE_Recursive_Thread_Mutex, guard, sample_lock_);

    const typename InstanceMap::iterator end = instance_map_.end();
    typename InstanceMap::iterator it = instance_map_.begin();
    while (it != end) {
      const DDS::InstanceHandle_t handle = it->second;
      ++it; // it will be invalid, so iterate now.
      release_instance(handle);
    }
  }

protected:

  virtual RcHandle<MessageHolder> dds_demarshal(const OpenDDS::DCPS::ReceivedDataSample& sample,
                                                DDS::InstanceHandle_t publication_handle,
                                                OpenDDS::DCPS::SubscriptionInstance_rch& instance,
                                                bool& just_registered,
                                                bool& filtered,
                                                OpenDDS::DCPS::MarshalingType marshaling_type,
                                                bool full_copy)
  {
    unique_ptr<MessageTypeWithAllocator> data(new (*data_allocator()) MessageTypeWithAllocator);
    RcHandle<MessageHolder> message_holder;

    if (marshal_skip_serialize_) {
      if (!MarshalTraitsType::from_message_block(*data, *sample.sample_)) {
        if (DCPS_debug_level > 0) {
          ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: DataReaderImpl::dds_demarshal: ")
                    ACE_TEXT("attempting to skip serialize but bad from_message_block. Returning from demarshal.\n")));
        }
        return message_holder;
      }
      store_instance_data(move(data), publication_handle, sample.header_, instance, just_registered, filtered);
      return message_holder;
    }
    const bool encapsulated = sample.header_.cdr_encapsulation_;

    OpenDDS::DCPS::Serializer ser(
      sample.sample_.get(),
      encapsulated ? Encoding::KIND_XCDR1 : Encoding::KIND_UNALIGNED_CDR,
      static_cast<Endianness>(sample.header_.byte_order_));

    if (encapsulated) {
      EncapsulationHeader encap;
      if (!(ser >> encap)) {
        if (DCPS_debug_level > 0) {
          ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR ")
            ACE_TEXT("%CDataReaderImpl::dds_demarshal: ")
            ACE_TEXT("deserialization of encapsulation header failed.\n"),
            TraitsType::type_name()));
        }
        return message_holder;
      }
      Encoding encoding;
      if (!encap.to_encoding(encoding, MarshalTraitsType::extensibility())) {
        return message_holder;
      }

      if (decoding_modes_.find(encoding.kind()) == decoding_modes_.end()) {
        if (DCPS_debug_level >= 1) {
          ACE_DEBUG((LM_WARNING, ACE_TEXT("(%P|%t) WARNING ")
            ACE_TEXT("%CDataReaderImpl::dds_demarshal: ")
            ACE_TEXT("Encoding kind %C of the received sample does not ")
            ACE_TEXT("match the ones specified by DataReader.\n"),
            TraitsType::type_name(),
            Encoding::kind_to_string(encoding.kind()).c_str()));
        }
        return message_holder;
      }
      if (DCPS_debug_level >= 8) {
        ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) ")
          ACE_TEXT("%CDataReaderImpl::dds_demarshal: ")
          ACE_TEXT("Deserializing with encoding kind %C.\n"),
          TraitsType::type_name(),
          Encoding::kind_to_string(encoding.kind()).c_str()));
      }

      ser.encoding(encoding);
    }

    const bool key_only_marshaling =
      marshaling_type == OpenDDS::DCPS::KEY_ONLY_MARSHALING;

    bool ser_ret = true;
    if (key_only_marshaling) {
      ser_ret = ser >> OpenDDS::DCPS::KeyOnly<MessageType>(*data);
    } else {
      ser_ret = ser >> *data;
      if (full_copy) {
        message_holder = make_rch<MessageHolder_T<MessageType> >(*data);
      }
    }
    if (!ser_ret) {
      if (ser.get_construction_status() != Serializer::ConstructionSuccessful) {
        if (DCPS_debug_level > 1) {
          ACE_DEBUG((LM_WARNING, ACE_TEXT("(%P|%t) %CDataReaderImpl::dds_demarshal ")
                     ACE_TEXT("object construction failure, dropping sample.\n"),
                     TraitsType::type_name()));
        }
      } else {
        if (DCPS_debug_level > 0) {
          ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR %CDataReaderImpl::dds_demarshal ")
                    ACE_TEXT("deserialization failed, dropping sample.\n"),
                    TraitsType::type_name()));
        }
      }
      return message_holder;
    }

#ifndef OPENDDS_NO_CONTENT_FILTERED_TOPIC
    /*
     * If sample.header_.content_filter_ is true, the writer has already
     * filtered.
     */
    if (!sample.header_.content_filter_) {
      ACE_Guard<ACE_Thread_Mutex> guard(content_filtered_topic_mutex_);
      if (content_filtered_topic_) {
        const bool sample_only_has_key_fields = !sample.header_.valid_data();
        if (key_only_marshaling != sample_only_has_key_fields) {
          if (DCPS_debug_level > 0) {
            ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR ")
              ACE_TEXT("%CDataReaderImpl::dds_demarshal: ")
              ACE_TEXT("Mismatch between the key only and valid data properties ")
              ACE_TEXT("of a %C message of a content filtered topic!\n"),
              TraitsType::type_name(),
              to_string(static_cast<MessageId>(sample.header_.message_id_))));
          }
          filtered = true;
          message_holder.reset();
          return message_holder;
        }
        const MessageType& type = static_cast<MessageType&>(*data);
        if (!content_filtered_topic_->filter(type, sample_only_has_key_fields)) {
          filtered = true;
          message_holder.reset();
          return message_holder;
        }
      }
    }
#endif

    store_instance_data(move(data), publication_handle, sample.header_, instance, just_registered, filtered);
    return message_holder;
  }

  virtual void dispose_unregister(const OpenDDS::DCPS::ReceivedDataSample& sample,
                                  DDS::InstanceHandle_t publication_handle,
                                  OpenDDS::DCPS::SubscriptionInstance_rch& instance)
  {
    //!!! caller should already have the sample_lock_

    // The data sample in this dispose message does not contain any valid data.
    // What it needs here is the key value to identify the instance to dispose.
    // The demarshal push this "sample" to received sample list so the user
    // can be notified the dispose event.
    bool just_registered = false;
    bool filtered = false;
    OpenDDS::DCPS::MarshalingType marshaling = OpenDDS::DCPS::FULL_MARSHALING;
    if (sample.header_.key_fields_only_) {
      marshaling = OpenDDS::DCPS::KEY_ONLY_MARSHALING;
    }
    dds_demarshal(sample, publication_handle, instance, just_registered, filtered, marshaling, false);
  }

  virtual void purge_data(OpenDDS::DCPS::SubscriptionInstance_rch instance)
  {
    drop_sample(instance->instance_handle_);


    instance->instance_state_->cancel_release();

    while (instance->rcvd_samples_.size() > 0)
      {
        OpenDDS::DCPS::ReceivedDataElement* head =
          instance->rcvd_samples_.remove_head();
        head->dec_ref();
      }
  }

  virtual void release_instance_i(DDS::InstanceHandle_t handle)
  {
    const typename ReverseInstanceMap::iterator pos = reverse_instance_map_.find(handle);
    if (pos != reverse_instance_map_.end()) {
      remove_from_lookup_maps(handle);
      instance_map_.erase(pos->second);
      reverse_instance_map_.erase(pos);
    }
  }

  virtual void state_updated_i(DDS::InstanceHandle_t handle)
  {
    const typename SubscriptionInstanceMapType::iterator pos = instances_.find(handle);
    if (pos != instances_.end()) {
      update_lookup_maps(pos);
    }
  }

private:

  bool store_instance_data_check(unique_ptr<MessageTypeWithAllocator>& instance_data,
                                 DDS::InstanceHandle_t publication_handle,
                                 const OpenDDS::DCPS::DataSampleHeader& header,
                                 OpenDDS::DCPS::SubscriptionInstance_rch& instance_ptr)
  {
#if defined(OPENDDS_SECURITY) && !defined(OPENDDS_NO_CONTENT_SUBSCRIPTION_PROFILE)
    const bool is_dispose_msg =
      header.message_id_ == OpenDDS::DCPS::DISPOSE_INSTANCE ||
      header.message_id_ == OpenDDS::DCPS::DISPOSE_UNREGISTER_INSTANCE;

    if (!is_bit() && security_config_) {
      if (header.message_id_ == SAMPLE_DATA ||
          header.message_id_ == INSTANCE_REGISTRATION) {

        // Pubulisher has already gone through the check.
        if (instance_ptr &&
            instance_ptr->instance_state_ &&
            instance_ptr->instance_state_->writes_instance(header.publication_id_)) {
          return true;
        }

        DDS::Security::SecurityException ex;
        const RepoId local_participant = make_id(get_repo_id(), ENTITYID_PARTICIPANT);
        const RepoId remote_participant = make_id(header.publication_id_, ENTITYID_PARTICIPANT);
        const DDS::Security::ParticipantCryptoHandle remote_participant_permissions_handle = security_config_->get_handle_registry(local_participant)->get_remote_participant_permissions_handle(remote_participant);
        // Construct a DynamicData around the deserialized sample.
        XTypes::DynamicDataAdapter<MessageType> dda(dynamic_type_, getMetaStruct<MessageType>(), *instance_data);
        // The remote participant might not be using security.
        if (remote_participant_permissions_handle != DDS::HANDLE_NIL &&
            !security_config_->get_access_control()->check_remote_datawriter_register_instance(remote_participant_permissions_handle, this, publication_handle, &dda, ex)) {
          if (log_level >= LogLevel::Warning) {
            ACE_ERROR((LM_WARNING,
                       "(%P|%t) WARNING: DataReaderImpl_T::store_instance_data_check: unable to register instance SecurityException[%d.%d]: %C\n",
                       ex.code, ex.minor_code, ex.message.in()));
          }
          return false;
        }
      } else if (is_dispose_msg) {

        DDS::Security::SecurityException ex;
        const RepoId local_participant = make_id(get_repo_id(), ENTITYID_PARTICIPANT);
        const RepoId remote_participant = make_id(header.publication_id_, ENTITYID_PARTICIPANT);
        const DDS::Security::ParticipantCryptoHandle remote_participant_permissions_handle = security_config_->get_handle_registry(local_participant)->get_remote_participant_permissions_handle(remote_participant);
        // Construct a DynamicData around the deserialized sample.
        XTypes::DynamicDataAdapter<MessageType> dda(dynamic_type_, getMetaStruct<MessageType>(), *instance_data);
        // The remote participant might not be using security.
        if (remote_participant_permissions_handle != DDS::HANDLE_NIL &&
            !security_config_->get_access_control()->check_remote_datawriter_dispose_instance(remote_participant_permissions_handle, this, publication_handle, &dda, ex)) {
          if (log_level >= LogLevel::Warning) {
            ACE_ERROR((LM_WARNING,
                       "(%P|%t) WARNING: DataReaderImpl_T::store_instance_data_check: unable to dispose instance SecurityException[%d.%d]: %C\n",
                       ex.code, ex.minor_code, ex.message.in()));
          }
          return false;
        }
      }
    }
#else
    ACE_UNUSED_ARG(instance_data);
    ACE_UNUSED_ARG(publication_handle);
    ACE_UNUSED_ARG(header);
    ACE_UNUSED_ARG(instance_ptr);
#endif

    return true;
  }

  DDS::ReturnCode_t read_i(MessageSequenceType& received_data,
                           DDS::SampleInfoSeq& info_seq,
                           CORBA::Long max_samples,
                           DDS::SampleStateMask sample_states,
                           DDS::ViewStateMask view_states,
                           DDS::InstanceStateMask instance_states,
#ifndef OPENDDS_NO_QUERY_CONDITION
                           DDS::QueryCondition_ptr a_condition)
#else
    int)
#endif
{

  typename MessageSequenceType::PrivateMemberAccess received_data_p(received_data);

#ifndef OPENDDS_NO_OBJECT_MODEL_PROFILE
  if (subqos_.presentation.access_scope == DDS::GROUP_PRESENTATION_QOS && !coherent_) {
    return DDS::RETCODE_PRECONDITION_NOT_MET;
  }

  const bool group_coherent_ordered =
    subqos_.presentation.access_scope == DDS::GROUP_PRESENTATION_QOS
    && subqos_.presentation.coherent_access
    && subqos_.presentation.ordered_access;

  if (group_coherent_ordered && coherent_) {
    max_samples = 1;
  }
#endif

  RakeResults<MessageSequenceType> results(this, received_data, info_seq, max_samples, subqos_.presentation,
#ifndef OPENDDS_NO_QUERY_CONDITION
                                           a_condition,
#endif
                                           DDS_OPERATION_READ);

  const Observer_rch observer = get_observer(Observer::e_SAMPLE_READ);

#ifndef OPENDDS_NO_OBJECT_MODEL_PROFILE
  if (!group_coherent_ordered) {
#endif
    const HandleSet& matches = lookup_matching_instances(sample_states, view_states, instance_states);
    for (HandleSet::const_iterator it = matches.begin(), next = it; it != matches.end(); it = next) {
      ++next; // pre-increment iterator, in case updates cause changes to match set
      const DDS::InstanceHandle_t handle = *it;
      const SubscriptionInstance_rch inst = get_handle_instance(handle);
      if (!inst) continue;

      size_t i(0);
      for (ReceivedDataElement* item = inst->rcvd_samples_.get_next_match(sample_states, 0); item;
           item = inst->rcvd_samples_.get_next_match(sample_states, item)) {
        results.insert_sample(item, &inst->rcvd_samples_, inst, ++i);

        const ValueWriterDispatcher* vwd = get_value_writer_dispatcher();
        if (observer && item->registered_data_ && vwd) {
          Observer::Sample s(handle, inst->instance_state_->instance_state(), *item, *vwd);
          observer->on_sample_read(this, s);
        }
      }
    }
#ifndef OPENDDS_NO_OBJECT_MODEL_PROFILE
  } else {
    const RakeData item = group_coherent_ordered_data_.get_data();
    results.insert_sample(item.rde_, item.rdel_, item.si_, item.index_in_instance_);
    const ValueWriterDispatcher* vwd = get_value_writer_dispatcher();
    if (observer && item.rde_->registered_data_ && vwd) {
      typename InstanceMap::iterator i = instance_map_.begin();
      const DDS::InstanceHandle_t handle = (i != instance_map_.end()) ? i->second : DDS::HANDLE_NIL;
      Observer::Sample s(handle, item.si_->instance_state_->instance_state(), *item.rde_, *vwd);
      observer->on_sample_read(this, s);
    }
  }
#endif

  results.copy_to_user();

  DDS::ReturnCode_t ret = DDS::RETCODE_NO_DATA;
  if (received_data.length()) {
    ret = DDS::RETCODE_OK;
    if (received_data.maximum() == 0) { // using ZeroCopy
      received_data_p.set_loaner(this);
    }
  }

  post_read_or_take();
  return ret;
}

DDS::ReturnCode_t take_i(MessageSequenceType& received_data,
                         DDS::SampleInfoSeq& info_seq,
                         CORBA::Long max_samples,
                         DDS::SampleStateMask sample_states,
                         DDS::ViewStateMask view_states,
                         DDS::InstanceStateMask instance_states,
#ifndef OPENDDS_NO_QUERY_CONDITION
                         DDS::QueryCondition_ptr a_condition)
#else
  int)
#endif
{
  typename MessageSequenceType::PrivateMemberAccess received_data_p(received_data);

#ifndef OPENDDS_NO_OBJECT_MODEL_PROFILE
  if (subqos_.presentation.access_scope == DDS::GROUP_PRESENTATION_QOS && !coherent_) {
    return DDS::RETCODE_PRECONDITION_NOT_MET;
  }

  const bool group_coherent_ordered =
    subqos_.presentation.access_scope == DDS::GROUP_PRESENTATION_QOS
    && subqos_.presentation.coherent_access
    && subqos_.presentation.ordered_access;

  if (group_coherent_ordered && coherent_) {
    max_samples = 1;
  }
#endif

  RakeResults<MessageSequenceType> results(this, received_data, info_seq, max_samples, subqos_.presentation,
#ifndef OPENDDS_NO_QUERY_CONDITION
                                           a_condition,
#endif
                                           DDS_OPERATION_TAKE);

  const Observer_rch observer = get_observer(Observer::e_SAMPLE_TAKEN);

#ifndef OPENDDS_NO_OBJECT_MODEL_PROFILE
  if (!group_coherent_ordered) {
#endif
    const HandleSet& matches = lookup_matching_instances(sample_states, view_states, instance_states);
    for (HandleSet::const_iterator it = matches.begin(), next = it; it != matches.end(); it = next) {
      ++next; // pre-increment iterator, in case updates cause changes to match set
      const DDS::InstanceHandle_t handle = *it;
      const SubscriptionInstance_rch inst = get_handle_instance(handle);
      if (!inst) continue;

      size_t i(0);
      for (ReceivedDataElement* item = inst->rcvd_samples_.get_next_match(sample_states, 0); item;
           item = inst->rcvd_samples_.get_next_match(sample_states, item)) {
        results.insert_sample(item, &inst->rcvd_samples_, inst, ++i);

        const ValueWriterDispatcher* vwd = get_value_writer_dispatcher();
        if (observer && item->registered_data_ && vwd) {
          Observer::Sample s(handle, inst->instance_state_->instance_state(), *item, *vwd);
          observer->on_sample_taken(this, s);
        }
      }
    }
#ifndef OPENDDS_NO_OBJECT_MODEL_PROFILE
  } else {
    const RakeData item = group_coherent_ordered_data_.get_data();
    results.insert_sample(item.rde_, item.rdel_, item.si_, item.index_in_instance_);
  }
#endif

  results.copy_to_user();

  DDS::ReturnCode_t ret = DDS::RETCODE_NO_DATA;
  if (received_data.length()) {
    ret = DDS::RETCODE_OK;
    if (received_data.maximum() == 0) { // using ZeroCopy
      received_data_p.set_loaner(this);
    }
  }

  post_read_or_take();
  return ret;
}

DDS::ReturnCode_t read_instance_i(MessageSequenceType& received_data,
                                  DDS::SampleInfoSeq& info_seq,
                                  CORBA::Long max_samples,
                                  DDS::InstanceHandle_t a_handle,
                                  DDS::SampleStateMask sample_states,
                                  DDS::ViewStateMask view_states,
                                  DDS::InstanceStateMask instance_states,
#ifndef OPENDDS_NO_QUERY_CONDITION
                                  DDS::QueryCondition_ptr a_condition)
#else
  int)
#endif
{
  const SubscriptionInstance_rch inst = get_handle_instance(a_handle);
  if (!inst) return DDS::RETCODE_BAD_PARAMETER;

  typename MessageSequenceType::PrivateMemberAccess received_data_p(received_data);

  RakeResults<MessageSequenceType> results(this, received_data, info_seq, max_samples, subqos_.presentation,
#ifndef OPENDDS_NO_QUERY_CONDITION
                                           a_condition,
#endif
                                           DDS_OPERATION_READ);

  const InstanceState_rch state_obj = inst->instance_state_;
  if (state_obj->match(view_states, instance_states)) {
    const Observer_rch observer = get_observer(Observer::e_SAMPLE_READ);
    size_t i(0);
    for (ReceivedDataElement* item = inst->rcvd_samples_.get_next_match(sample_states, 0); item;
         item = inst->rcvd_samples_.get_next_match(sample_states, item)) {
      results.insert_sample(item, &inst->rcvd_samples_, inst, ++i);
      const ValueWriterDispatcher* vwd = get_value_writer_dispatcher();
      if (observer && item->registered_data_ && vwd) {
        Observer::Sample s(a_handle, inst->instance_state_->instance_state(), *item, *vwd);
        observer->on_sample_read(this, s);
      }
    }
  } else if (DCPS_debug_level >= 8) {
    OPENDDS_STRING msg;
    if ((state_obj->view_state() & view_states) == 0) {
      msg = "view state is not valid";
    }
    if ((state_obj->instance_state() & instance_states) == 0) {
      if (!msg.empty()) msg += " and ";
      msg += "instance state is ";
      msg += state_obj->instance_state_string();
      msg += " while the validity mask is " + InstanceState::instance_state_mask_string(instance_states);
    }
    ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) DataReaderImpl_T::read_instance_i: ")
               ACE_TEXT("will return no data reading sub %C because:\n  %C\n"),
               LogGuid(get_repo_id()).c_str(), msg.c_str()));
  }

  results.copy_to_user();

  DDS::ReturnCode_t ret = DDS::RETCODE_NO_DATA;
  if (received_data.length()) {
    ret = DDS::RETCODE_OK;
    if (received_data.maximum() == 0) { // using ZeroCopy
      received_data_p.set_loaner(this);
    }
  }

  post_read_or_take();
  return ret;
}

DDS::ReturnCode_t take_instance_i(MessageSequenceType& received_data,
                                  DDS::SampleInfoSeq& info_seq,
                                  CORBA::Long max_samples,
                                  DDS::InstanceHandle_t a_handle,
                                  DDS::SampleStateMask sample_states,
                                  DDS::ViewStateMask view_states,
                                  DDS::InstanceStateMask instance_states,
#ifndef OPENDDS_NO_QUERY_CONDITION
                                  DDS::QueryCondition_ptr a_condition)
#else
  int)
#endif
{
  const SubscriptionInstance_rch inst = get_handle_instance(a_handle);
  if (!inst) return DDS::RETCODE_BAD_PARAMETER;

  typename MessageSequenceType::PrivateMemberAccess received_data_p(received_data);

  RakeResults<MessageSequenceType> results(this, received_data, info_seq, max_samples, subqos_.presentation,
#ifndef OPENDDS_NO_QUERY_CONDITION
                                           a_condition,
#endif
                                           DDS_OPERATION_TAKE);

  const InstanceState_rch state_obj = inst->instance_state_;
  if (state_obj->match(view_states, instance_states)) {
    const Observer_rch observer = get_observer(Observer::e_SAMPLE_TAKEN);
    size_t i(0);
    for (ReceivedDataElement* item = inst->rcvd_samples_.get_next_match(sample_states, 0); item;
         item = inst->rcvd_samples_.get_next_match(sample_states, item)) {
      results.insert_sample(item, &inst->rcvd_samples_, inst, ++i);
      const ValueWriterDispatcher* vwd = get_value_writer_dispatcher();
      if (observer && item->registered_data_ && vwd) {
        Observer::Sample s(a_handle, inst->instance_state_->instance_state(), *item, *vwd);
        observer->on_sample_taken(this, s);
      }
    }
  }

  results.copy_to_user();

  DDS::ReturnCode_t ret = DDS::RETCODE_NO_DATA;
  if (received_data.length()) {
    ret = DDS::RETCODE_OK;
    if (received_data.maximum() == 0) { // using ZeroCopy
      received_data_p.set_loaner(this);
    }
  }

  post_read_or_take();
  return ret;
}

DDS::ReturnCode_t read_next_instance_i(MessageSequenceType& received_data,
                                       DDS::SampleInfoSeq& info_seq,
                                       CORBA::Long max_samples,
                                       DDS::InstanceHandle_t a_handle,
                                       DDS::SampleStateMask sample_states,
                                       DDS::ViewStateMask view_states,
                                       DDS::InstanceStateMask instance_states,
#ifndef OPENDDS_NO_QUERY_CONDITION
                                       DDS::QueryCondition_ptr a_condition)
#else
  int)
#endif
{
  ACE_GUARD_RETURN(ACE_Recursive_Thread_Mutex, guard, sample_lock_, DDS::RETCODE_ERROR);

  typename InstanceMap::iterator it = instance_map_.begin();
  const typename InstanceMap::iterator the_end = instance_map_.end();
  if (a_handle != DDS::HANDLE_NIL) {
    const typename ReverseInstanceMap::const_iterator pos = reverse_instance_map_.find(a_handle);
    if (pos != reverse_instance_map_.end()) {
      it = pos->second;
      ++it;
    } else {
      it = the_end;
    }
  }

  DDS::InstanceHandle_t handle(DDS::HANDLE_NIL);
  for (; it != the_end; ++it) {
    handle = it->second;
    const DDS::ReturnCode_t status =
      read_instance_i(received_data, info_seq, max_samples, handle,
                      sample_states, view_states, instance_states,
#ifndef OPENDDS_NO_QUERY_CONDITION
                      a_condition);
#else
      0);
#endif
    if (status != DDS::RETCODE_NO_DATA) {
      post_read_or_take();
      return status;
    }
  }

  post_read_or_take();
  return DDS::RETCODE_NO_DATA;
}

DDS::ReturnCode_t take_next_instance_i(MessageSequenceType& received_data,
                                       DDS::SampleInfoSeq& info_seq,
                                       CORBA::Long max_samples,
                                       DDS::InstanceHandle_t a_handle,
                                       DDS::SampleStateMask sample_states,
                                       DDS::ViewStateMask view_states,
                                       DDS::InstanceStateMask instance_states,
#ifndef OPENDDS_NO_QUERY_CONDITION
                                       DDS::QueryCondition_ptr a_condition)
#else
  int)
#endif
{
  ACE_GUARD_RETURN(ACE_Recursive_Thread_Mutex, guard, sample_lock_, DDS::RETCODE_ERROR);

  typename InstanceMap::iterator it = instance_map_.begin();
  const typename InstanceMap::iterator the_end = instance_map_.end();
  if (a_handle != DDS::HANDLE_NIL) {
    const typename ReverseInstanceMap::const_iterator pos = reverse_instance_map_.find(a_handle);
    if (pos != reverse_instance_map_.end()) {
      it = pos->second;
      ++it;
    } else {
      it = the_end;
    }
  }

  DDS::InstanceHandle_t handle(DDS::HANDLE_NIL);
  for (; it != the_end; ++it) {
    handle = it->second;
    const DDS::ReturnCode_t status =
      take_instance_i(received_data, info_seq, max_samples, handle,
                      sample_states, view_states, instance_states,
#ifndef OPENDDS_NO_QUERY_CONDITION
                      a_condition);
#else
      0);
#endif
    if (status != DDS::RETCODE_NO_DATA) {
      total_samples();  // see if we are empty
      post_read_or_take();
      return status;
    }
  }

  post_read_or_take();
  return DDS::RETCODE_NO_DATA;
}

void store_instance_data(unique_ptr<MessageTypeWithAllocator> instance_data,
                         DDS::InstanceHandle_t publication_handle,
                         const OpenDDS::DCPS::DataSampleHeader& header,
                         OpenDDS::DCPS::SubscriptionInstance_rch& instance_ptr,
                         bool& just_registered,
                         bool& filtered)
{
  ACE_UNUSED_ARG(publication_handle);

  const bool is_dispose_msg =
    header.message_id_ == OpenDDS::DCPS::DISPOSE_INSTANCE ||
    header.message_id_ == OpenDDS::DCPS::DISPOSE_UNREGISTER_INSTANCE;
  const bool is_unregister_msg =
    header.message_id_ == OpenDDS::DCPS::UNREGISTER_INSTANCE ||
    header.message_id_ == OpenDDS::DCPS::DISPOSE_UNREGISTER_INSTANCE;

  if (!store_instance_data_check(instance_data, publication_handle, header, instance_ptr)) {
    return;
  }

  // not filtering any data, except what is specifically identified as filtered below
  filtered = false;

  DDS::InstanceHandle_t handle(DDS::HANDLE_NIL);

  //!!! caller should already have the sample_lock_
  //We will unlock it before calling into listeners

  typename InstanceMap::const_iterator const it = instance_map_.find(*instance_data);

  if (it == instance_map_.end()) {
    if (is_dispose_msg || is_unregister_msg) {
      return;
    }

    std::size_t instances_size = 0;
    {
      ACE_GUARD(ACE_Recursive_Thread_Mutex, instance_guard, instances_lock_);
      instances_size = instances_.size();
    }
    if ((qos_.resource_limits.max_instances != DDS::LENGTH_UNLIMITED) &&
      ((::CORBA::Long) instances_size >= qos_.resource_limits.max_instances))
    {
      DDS::DataReaderListener_var listener
        = listener_for (DDS::SAMPLE_REJECTED_STATUS);

      set_status_changed_flag (DDS::SAMPLE_REJECTED_STATUS, true);

      sample_rejected_status_.last_reason = DDS::REJECTED_BY_INSTANCES_LIMIT;
      ++sample_rejected_status_.total_count;
      ++sample_rejected_status_.total_count_change;
      sample_rejected_status_.last_instance_handle = handle;

      if (!CORBA::is_nil(listener.in()))
      {
        ACE_GUARD(typename DataReaderImpl::Reverse_Lock_t, unlock_guard, reverse_sample_lock_);

        listener->on_sample_rejected(this, sample_rejected_status_);
        sample_rejected_status_.total_count_change = 0;
      }  // do we want to do something if listener is nil???
      notify_status_condition_no_sample_lock();

      return;
    }

    {
      ACE_GUARD(ACE_Recursive_Thread_Mutex, instance_guard, instances_lock_);

#ifndef OPENDDS_NO_OWNERSHIP_KIND_EXCLUSIVE
      SharedInstanceMap_rch inst;
      OwnershipManagerScopedAccess ownership_scoped_access;
      OwnershipManagerPtr owner_manager = ownership_manager();

      bool new_handle = true;
      if (is_exclusive_ownership_) {
        OwnershipManagerScopedAccess temp(owner_manager);
        temp.swap(ownership_scoped_access);
        if (!owner_manager || ownership_scoped_access.lock_result_ != 0) {
          if (DCPS_debug_level > 0) {
            ACE_ERROR ((LM_ERROR,
                        ACE_TEXT("(%P|%t) ")
                        ACE_TEXT("%CDataReaderImpl::")
                        ACE_TEXT("store_instance_data, ")
                        ACE_TEXT("acquire instance_lock failed.\n"), TraitsType::type_name()));
          }
          return;
        }

        inst = dynamic_rchandle_cast<SharedInstanceMap>(
          owner_manager->get_instance_map(topic_servant_->type_name(), this));
        if (inst != 0) {
          typename InstanceMap::const_iterator const iter = inst->find(*instance_data);
          if (iter != inst->end ()) {
            handle = iter->second;
            new_handle = false;
          }
        }
      }
#endif

      just_registered = true;
      DDS::BuiltinTopicKey_t key = OpenDDS::DCPS::keyFromSample(static_cast<MessageType*>(instance_data.get()));
      bool owns_handle = false;
      if (handle == DDS::HANDLE_NIL) {
        handle = get_next_handle(key);
        owns_handle = true;
      }
      OpenDDS::DCPS::SubscriptionInstance_rch instance =
        OpenDDS::DCPS::make_rch<OpenDDS::DCPS::SubscriptionInstance>(
          this,
          qos_,
          ref(instances_lock_),
          handle, owns_handle);

      const std::pair<typename SubscriptionInstanceMapType::iterator, bool> bpair =
        instances_.insert(typename SubscriptionInstanceMapType::value_type(handle, instance));

      if (bpair.second == false) {
        if (DCPS_debug_level > 0) {
          ACE_ERROR((LM_ERROR,
                     ACE_TEXT("(%P|%t) ")
                     ACE_TEXT("%CDataReaderImpl::")
                     ACE_TEXT("store_instance_data, ")
                     ACE_TEXT("insert handle failed.\n"), TraitsType::type_name()));
        }
        return;
      }
      update_lookup_maps(bpair.first);

#ifndef OPENDDS_NO_OWNERSHIP_KIND_EXCLUSIVE
      if (owner_manager) {
        if (!inst) {
          inst = make_rch<SharedInstanceMap>();
          owner_manager->set_instance_map(
            topic_servant_->type_name(),
            inst,
            this);
        }

        if (new_handle) {
          const std::pair<typename InstanceMap::iterator, bool> bpair =
            inst->insert(typename InstanceMap::value_type(*instance_data,
              handle));
          if (bpair.second == false)
          {
            if (DCPS_debug_level > 0) {
              ACE_ERROR ((LM_ERROR,
                          ACE_TEXT("(%P|%t) ")
                          ACE_TEXT("%CDataReaderImpl::")
                          ACE_TEXT("store_instance_data, ")
                          ACE_TEXT("insert to participant scope %C failed.\n"), TraitsType::type_name(), TraitsType::type_name()));
            }
            return;
          }
        }

        OwnershipManagerScopedAccess temp;
        temp.swap(ownership_scoped_access);
        if (temp.release() != 0) {
          if (DCPS_debug_level > 0) {
            ACE_ERROR ((LM_ERROR,
                        ACE_TEXT("(%P|%t) ")
                        ACE_TEXT("%CDataReaderImpl::")
                        ACE_TEXT("store_instance_data, ")
                        ACE_TEXT("release instance_lock failed.\n"), TraitsType::type_name()));
          }
          return;
        }
      }
#endif
    } // scope for instances_lock_

    std::pair<typename InstanceMap::iterator, bool> bpair =
      instance_map_.insert(typename InstanceMap::value_type(*instance_data,
        handle));
    if (bpair.second == false)
    {
      if (DCPS_debug_level > 0) {
        ACE_ERROR ((LM_ERROR,
                    ACE_TEXT("(%P|%t) ")
                    ACE_TEXT("%CDataReaderImpl::")
                    ACE_TEXT("store_instance_data, ")
                    ACE_TEXT("insert %C failed.\n"), TraitsType::type_name(), TraitsType::type_name()));
      }
      return;
    }
    reverse_instance_map_[handle] = bpair.first;
  }
  else
  {
    just_registered = false;
    handle = it->second;
  }

  if (header.message_id_ != OpenDDS::DCPS::INSTANCE_REGISTRATION)
  {
    instance_ptr = get_handle_instance(handle);
    OPENDDS_ASSERT(instance_ptr);

    if (header.message_id_ == OpenDDS::DCPS::SAMPLE_DATA)
    {
      {
        ACE_GUARD(ACE_Recursive_Thread_Mutex, instance_guard, instances_lock_);
        filtered = ownership_filter_instance(instance_ptr, header.publication_id_);
      }

      MonotonicTimePoint now;
      MonotonicTimePoint deadline;
      if (!filtered && time_based_filter_instance(instance_ptr, now, deadline)) {
        filtered = true;
        if (qos_.reliability.kind == DDS::RELIABLE_RELIABILITY_QOS) {
          delay_sample(handle, move(instance_data), header, just_registered, now, deadline);
        }
      } else {
        // nothing time based filtered now
        clear_sample(handle);

      }

      if (filtered) {
        return;
      }
    }

    finish_store_instance_data(move(instance_data), header, instance_ptr, is_dispose_msg, is_unregister_msg);
  }
  else
  {
    instance_ptr = get_handle_instance(handle);
    OPENDDS_ASSERT(instance_ptr);
    instance_ptr->instance_state_->lively(header.publication_id_);
  }
}

void finish_store_instance_data(unique_ptr<MessageTypeWithAllocator> instance_data, const DataSampleHeader& header,
  SubscriptionInstance_rch instance_ptr, bool is_dispose_msg, bool is_unregister_msg )
{
  if ((qos_.resource_limits.max_samples_per_instance !=
        DDS::LENGTH_UNLIMITED) &&
      (instance_ptr->rcvd_samples_.size() >=
        static_cast<size_t>(qos_.resource_limits.max_samples_per_instance))) {

    // According to spec 1.2, Samples that contain no data do not
    // count towards the limits imposed by the RESOURCE_LIMITS QoS policy
    // so do not remove the oldest sample when unregister/dispose
    // message arrives.

    if (!is_dispose_msg && !is_unregister_msg
      && !instance_ptr->rcvd_samples_.matches(DDS::READ_SAMPLE_STATE))
    {
      DDS::DataReaderListener_var listener
        = listener_for(DDS::SAMPLE_REJECTED_STATUS);

      set_status_changed_flag(DDS::SAMPLE_REJECTED_STATUS, true);

      sample_rejected_status_.last_reason =
        DDS::REJECTED_BY_SAMPLES_PER_INSTANCE_LIMIT;
      ++sample_rejected_status_.total_count;
      ++sample_rejected_status_.total_count_change;
      sample_rejected_status_.last_instance_handle = instance_ptr->instance_handle_;

      if (!CORBA::is_nil(listener.in()))
      {
        ACE_GUARD(typename DataReaderImpl::Reverse_Lock_t, unlock_guard, reverse_sample_lock_);

        listener->on_sample_rejected(this, sample_rejected_status_);
        sample_rejected_status_.total_count_change = 0;
      }  // do we want to do something if listener is nil???
      notify_status_condition_no_sample_lock();
      return;
    }
    else if (!is_dispose_msg && !is_unregister_msg)
    {
      // Discard the oldest previously-read sample
      OpenDDS::DCPS::ReceivedDataElement* item =
        instance_ptr->rcvd_samples_.remove_head();
      item->dec_ref();
    }
  }
  else if (qos_.resource_limits.max_samples != DDS::LENGTH_UNLIMITED)
  {
    CORBA::Long total_samples = 0;
    {
      ACE_GUARD(ACE_Recursive_Thread_Mutex, instance_guard, instances_lock_);
      for (OpenDDS::DCPS::DataReaderImpl::SubscriptionInstanceMapType::iterator iter = instances_.begin();
        iter != instances_.end();
        ++iter) {
        OpenDDS::DCPS::SubscriptionInstance_rch ptr = iter->second;

        total_samples += (CORBA::Long) ptr->rcvd_samples_.size();
      }
    }

    if (total_samples >= qos_.resource_limits.max_samples)
    {
      // According to spec 1.2, Samples that contain no data do not
      // count towards the limits imposed by the RESOURCE_LIMITS QoS policy
      // so do not remove the oldest sample when unregister/dispose
      // message arrives.

      if (!is_dispose_msg && !is_unregister_msg
        && !instance_ptr->rcvd_samples_.matches(DDS::READ_SAMPLE_STATE))
      {
        DDS::DataReaderListener_var listener
          = listener_for(DDS::SAMPLE_REJECTED_STATUS);

        set_status_changed_flag(DDS::SAMPLE_REJECTED_STATUS, true);

        sample_rejected_status_.last_reason =
          DDS::REJECTED_BY_SAMPLES_LIMIT;
        ++sample_rejected_status_.total_count;
        ++sample_rejected_status_.total_count_change;
        sample_rejected_status_.last_instance_handle = instance_ptr->instance_handle_;
        if (!CORBA::is_nil(listener.in()))
        {
          ACE_GUARD(typename DataReaderImpl::Reverse_Lock_t, unlock_guard, reverse_sample_lock_);

          listener->on_sample_rejected(this, sample_rejected_status_);
          sample_rejected_status_.total_count_change = 0;
        }  // do we want to do something if listener is nil???
        notify_status_condition_no_sample_lock();

        return;
      }
      else if (!is_dispose_msg && !is_unregister_msg)
      {
        // Discard the oldest previously-read sample
        OpenDDS::DCPS::ReceivedDataElement *item =
          instance_ptr->rcvd_samples_.remove_head();
        item->dec_ref();
      }
    }
  }

  bool event_notify = false;

  if (is_dispose_msg) {
    event_notify = instance_ptr->instance_state_->dispose_was_received(header.publication_id_);
  }

  if (is_unregister_msg) {
    if (instance_ptr->instance_state_->unregister_was_received(header.publication_id_)) {
      event_notify = true;
    }
  }

  if (!is_dispose_msg && !is_unregister_msg) {
    event_notify = true;
    instance_ptr->instance_state_->data_was_received(header.publication_id_);
  }

  if (!event_notify) {
    return;
  }

  ReceivedDataElement* const ptr =
    new (*rd_allocator_.get()) ReceivedDataElementWithType<MessageTypeWithAllocator>(
      header, instance_data.release(), &sample_lock_);

  ptr->disposed_generation_count_ =
    instance_ptr->instance_state_->disposed_generation_count();
  ptr->no_writers_generation_count_ =
    instance_ptr->instance_state_->no_writers_generation_count();

  instance_ptr->last_sequence_ = header.sequence_;

  instance_ptr->rcvd_strategy_->add(ptr);

  if (! is_dispose_msg  && ! is_unregister_msg
      && instance_ptr->rcvd_samples_.size() > get_depth())
    {
      OpenDDS::DCPS::ReceivedDataElement* head_ptr =
        instance_ptr->rcvd_samples_.remove_head();

      if (head_ptr->sample_state_ == DDS::NOT_READ_SAMPLE_STATE)
        {
          DDS::DataReaderListener_var listener
            = listener_for (DDS::SAMPLE_LOST_STATUS);

          ++sample_lost_status_.total_count;
          ++sample_lost_status_.total_count_change;

          set_status_changed_flag(DDS::SAMPLE_LOST_STATUS, true);

          if (!CORBA::is_nil(listener.in()))
            {
              ACE_GUARD(typename DataReaderImpl::Reverse_Lock_t, unlock_guard, reverse_sample_lock_);

              listener->on_sample_lost(this, sample_lost_status_);

              sample_lost_status_.total_count_change = 0;
            }

          notify_status_condition_no_sample_lock();
        }

      head_ptr->dec_ref();
    }

#ifndef OPENDDS_NO_OBJECT_MODEL_PROFILE
  if (! ptr->coherent_change_) {
#endif
    RcHandle<OpenDDS::DCPS::SubscriberImpl> sub = get_subscriber_servant ();
    if (!sub || get_deleted())
      return;

    sub->set_status_changed_flag(DDS::DATA_ON_READERS_STATUS, true);

    set_status_changed_flag(DDS::DATA_AVAILABLE_STATUS, true);

    DDS::SubscriberListener_var sub_listener =
        sub->listener_for(DDS::DATA_ON_READERS_STATUS);
    if (!CORBA::is_nil(sub_listener.in()) && !coherent_) {
      if (!is_bit()) {
        sub->set_status_changed_flag(DDS::DATA_ON_READERS_STATUS, false);
        ACE_GUARD(typename DataReaderImpl::Reverse_Lock_t, unlock_guard, reverse_sample_lock_);
        sub_listener->on_data_on_readers(sub.in());
      } else {
        TheServiceParticipant->job_queue()->enqueue(make_rch<OnDataOnReaders>(sub, sub_listener, rchandle_from(static_cast<DataReaderImpl*>(this)), true, false));
      }
    } else {
      sub->notify_status_condition();

      DDS::DataReaderListener_var listener =
        listener_for (DDS::DATA_AVAILABLE_STATUS);

      if (!CORBA::is_nil(listener.in())) {
        if (!is_bit()) {
          set_status_changed_flag(DDS::DATA_AVAILABLE_STATUS, false);
          sub->set_status_changed_flag(DDS::DATA_ON_READERS_STATUS, false);
          ACE_GUARD(typename DataReaderImpl::Reverse_Lock_t, unlock_guard, reverse_sample_lock_);
          listener->on_data_available(this);
        } else {
          TheServiceParticipant->job_queue()->enqueue(make_rch<OnDataAvailable>(listener, rchandle_from(static_cast<DataReaderImpl*>(this)), true, true, true));
        }
      } else {
        notify_status_condition_no_sample_lock();
      }
    }
#ifndef OPENDDS_NO_OBJECT_MODEL_PROFILE
  }
#endif
}

/// Release sample_lock_ during status notifications in store_instance_data()
/// as the lock is not needed and could cause deadlock condition.
/// See comments in member function implementation for details.
void notify_status_condition_no_sample_lock()
{
  // This member function avoids a deadlock condition which otherwise
  // could occur as follows:
  // Thread 1: Call to WaitSet::wait() causes WaitSet::lock_ to lock and
  // eventually DataReaderImpl::sample_lock_ to lock in call to
  // DataReaderImpl::contains_samples().
  // Thread2: Call to DataReaderImpl::data_received()
  // causes DataReaderImpl::sample_lock_ to lock and eventually
  // during notify of status condition a call to WaitSet::signal()
  // causes WaitSet::lock_ to lock.
  // Because the DataReaderImpl::sample_lock_ is not needed during
  // status notification this member function is used in
  // store_instance_data() to release sample_lock_ before making
  // the notification.
  ACE_GUARD(typename DataReaderImpl::Reverse_Lock_t, unlock_guard, reverse_sample_lock_);
  notify_status_condition();
}


/// Common input read* & take* input processing and precondition checks
DDS::ReturnCode_t check_inputs(const char* method_name,
                               MessageSequenceType& received_data,
                               DDS::SampleInfoSeq& info_seq,
                               ::CORBA::Long max_samples)
{
  typename MessageSequenceType::PrivateMemberAccess received_data_p (received_data);

  // ---- start of preconditions common to read and take -----
  // SPEC ref v1.2 7.1.2.5.3.8 #1
  // NOTE: We can't check maximum() or release() here since those are
  //       implementation details of the sequences.  In general, the
  //       info_seq will have release() == true and maximum() == 0.
  //       If we're in zero-copy mode, the received_data will have
  //       release() == false and maximum() == 0.  If it's not
  //       zero-copy then received_data will have release == true()
  //       and maximum() == anything.
  if (received_data.length() != info_seq.length())
    {
      ACE_DEBUG((LM_DEBUG,
                 ACE_TEXT("(%P|%t) %CDataReaderImpl::%C ")
                 ACE_TEXT("PRECONDITION_NOT_MET sample and info input ")
                 ACE_TEXT("sequences do not match.\n"),
                 TraitsType::type_name(),
                 method_name ));
      return DDS::RETCODE_PRECONDITION_NOT_MET;
    }

  //SPEC ref v1.2 7.1.2.5.3.8 #4
  if ((received_data.maximum() > 0) && (received_data.release() == false))
    {
      ACE_DEBUG((LM_DEBUG,
                 ACE_TEXT("(%P|%t) %CDataReaderImpl::%C ")
                 ACE_TEXT("PRECONDITION_NOT_MET mismatch of ")
                 ACE_TEXT("maximum %d and owns %d\n"),
                 TraitsType::type_name(),
                 method_name,
                 received_data.maximum(),
                 received_data.release() ));

      return DDS::RETCODE_PRECONDITION_NOT_MET;
    }

  if (received_data.maximum() == 0)
    {
      // not in SPEC but needed.
      if (max_samples == DDS::LENGTH_UNLIMITED)
        {
          max_samples =
            static_cast< ::CORBA::Long> (received_data_p.max_slots());
        }
    }
  else
    {
      if (max_samples == DDS::LENGTH_UNLIMITED)
        {
          //SPEC ref v1.2 7.1.2.5.3.8 #5a
          max_samples = received_data.maximum();
        }
      else if (
               max_samples > static_cast< ::CORBA::Long> (received_data.maximum()))
        {
          //SPEC ref v1.2 7.1.2.5.3.8 #5c
          ACE_DEBUG((LM_DEBUG,
                     ACE_TEXT("(%P|%t) %CDataReaderImpl::%C ")
                     ACE_TEXT("PRECONDITION_NOT_MET max_samples %d > maximum %d\n"),
                     TraitsType::type_name(),
                     method_name,
                     max_samples,
                     received_data.maximum()));
          return DDS::RETCODE_PRECONDITION_NOT_MET;
        }
      //else
      //SPEC ref v1.2 7.1.2.5.3.8 #5b - is true by impl below.
    }

  // The spec does not say what to do in this case but it appears to be a good thing.
  // Note: max_slots is the greater of the sequence's maximum and init_size.
  if (static_cast< ::CORBA::Long> (received_data_p.max_slots()) < max_samples)
    {
      max_samples = static_cast< ::CORBA::Long> (received_data_p.max_slots());
    }
  //---- end of preconditions common to read and take -----

  return DDS::RETCODE_OK;
}

void delay_sample(DDS::InstanceHandle_t handle,
                  unique_ptr<MessageTypeWithAllocator> data,
                  const OpenDDS::DCPS::DataSampleHeader& header,
                  const bool just_registered,
                  const MonotonicTimePoint& now,
                  const MonotonicTimePoint& deadline)
{
  // sample_lock_ should already be held
  DataSampleHeader_ptr hdr(new OpenDDS::DCPS::DataSampleHeader(header));

  typename FilterDelayedSampleMap::iterator i = filter_delayed_sample_map_.find(handle);
  if (i == filter_delayed_sample_map_.end()) {

    // emplace()/insert() only if the sample is going to be
    // new (otherwise we call move(data) twice).
    std::pair<typename FilterDelayedSampleMap::iterator, bool> result =
#ifdef ACE_HAS_CPP11
      filter_delayed_sample_map_.emplace(std::piecewise_construct,
                                         std::forward_as_tuple(handle),
                                         std::forward_as_tuple(move(data), hdr, just_registered));
#else
      filter_delayed_sample_map_.insert(std::make_pair(handle, FilterDelayedSample(move(data), hdr, just_registered)));
#endif
    FilterDelayedSample& sample = result.first->second;
    sample.expiration_time = deadline;
    const bool schedule = filter_delayed_sample_queue_.empty();
    filter_delayed_sample_queue_.insert(std::make_pair(deadline, handle));
    if (schedule) {
      filter_delayed_sample_task_->schedule(now - deadline);
    } else if (filter_delayed_sample_queue_.begin()->second == handle) {
      filter_delayed_sample_task_->cancel();
      filter_delayed_sample_task_->schedule(now - deadline);
    }
  } else {
    FilterDelayedSample& sample = i->second;
    // we only care about the most recently filtered sample, so clean up the last one

    sample.message = move(data);
    sample.header = hdr;
    sample.new_instance = just_registered;
    // already scheduled for timeout at the desired time
  }
}

void clear_sample(DDS::InstanceHandle_t handle)
{
  // sample_lock_ should already be held

  typename FilterDelayedSampleMap::iterator sample = filter_delayed_sample_map_.find(handle);
  if (sample != filter_delayed_sample_map_.end()) {
    // leave the entry in the container, so that the key remains valid if the reactor is waiting on this lock while this is occurring
    sample->second.message.reset();
  }
}

void drop_sample(DDS::InstanceHandle_t handle)
{
  // sample_lock_ should already be held

  typename FilterDelayedSampleMap::iterator sample = filter_delayed_sample_map_.find(handle);
  if (sample != filter_delayed_sample_map_.end()) {
    for (FilterDelayedSampleQueue::iterator pos = filter_delayed_sample_queue_.lower_bound(sample->second.expiration_time), limit = filter_delayed_sample_queue_.upper_bound(sample->second.expiration_time); pos != limit; ++pos) {
      if (pos->second == handle) {
        filter_delayed_sample_queue_.erase(pos);
        break;
      }
    }

    // use the handle to erase, since the sample lock was released
    filter_delayed_sample_map_.erase(handle);
  }
}

void filter_delayed(const MonotonicTimePoint& now)
{
  ThreadStatusManager::Event ev(TheServiceParticipant->get_thread_status_manager());

  // Make a copy because finish_store_instance_data will release the sample lock.
  typedef OPENDDS_VECTOR(DDS::InstanceHandle_t) Handles;
  Handles handles;

  ACE_GUARD(ACE_Recursive_Thread_Mutex, guard, sample_lock_);

  for (FilterDelayedSampleQueue::iterator pos = filter_delayed_sample_queue_.begin(), limit = filter_delayed_sample_queue_.end(); pos != limit && pos->first <= now;) {
    handles.push_back(pos->second);
    filter_delayed_sample_queue_.erase(pos++);
  }

  const TimeDuration interval(qos_.time_based_filter.minimum_separation);

  for (Handles::const_iterator pos = handles.begin(), limit = handles.end(); pos != limit; ++pos) {
    const DDS::InstanceHandle_t handle = *pos;

    SubscriptionInstance_rch instance = get_handle_instance(handle);
    if (!instance) {
      continue;
    }

    typename FilterDelayedSampleMap::iterator data = filter_delayed_sample_map_.find(handle);
    if (data == filter_delayed_sample_map_.end()) {
      continue;
    }

    if (data->second.message) {
      const bool NOT_DISPOSE_MSG = false;
      const bool NOT_UNREGISTER_MSG = false;
      // clear the message, since ownership is being transferred to finish_store_instance_data.

      instance->last_accepted_.set_to_now();
      const DataSampleHeader_ptr header = data->second.header;
      const bool new_instance = data->second.new_instance;

      // should not use data iterator anymore, since finish_store_instance_data releases sample_lock_
      finish_store_instance_data(move(data->second.message),
                                 *header,
                                 instance,
                                 NOT_DISPOSE_MSG,
                                 NOT_UNREGISTER_MSG);

      accept_sample_processing(instance, *header, new_instance);

      // Refresh the iterator.
      data = filter_delayed_sample_map_.find(handle);
      if (data == filter_delayed_sample_map_.end()) {
        continue;
      }

      // Reschedule.
      data->second.expiration_time = now + interval;
      filter_delayed_sample_queue_.insert(std::make_pair(data->second.expiration_time, handle));

    } else {
      // this check is performed to handle the corner case where
      // store_instance_data received and delivered a sample, while this
      // method was waiting for the lock
      if (MonotonicTimePoint::now() - instance->last_sample_tv_ >= interval) {
        // no new data to process, so remove from container
        filter_delayed_sample_map_.erase(data);
      }
    }
  }

  if (!filter_delayed_sample_queue_.empty()) {
    filter_delayed_sample_task_->schedule(filter_delayed_sample_queue_.begin()->first - now);
  }
}

unique_ptr<DataAllocator>& data_allocator() { return data_allocator_; }

unique_ptr<DataAllocator> data_allocator_;

InstanceMap instance_map_;
ReverseInstanceMap reverse_instance_map_;

typedef DCPS::PmfSporadicTask<DataReaderImpl_T> DRISporadicTask;

RcHandle<DRISporadicTask> filter_delayed_sample_task_;
#ifdef OPENDDS_HAS_STD_SHARED_PTR
typedef std::shared_ptr<const OpenDDS::DCPS::DataSampleHeader> DataSampleHeader_ptr;
#else
typedef ACE_Strong_Bound_Ptr<const OpenDDS::DCPS::DataSampleHeader, ACE_Null_Mutex> DataSampleHeader_ptr;
#endif
struct FilterDelayedSample {
  FilterDelayedSample(unique_ptr<MessageTypeWithAllocator> msg, DataSampleHeader_ptr hdr, bool new_inst)
    : message(move(msg))
    , header(hdr)
    , new_instance(new_inst)
  {}
  container_supported_unique_ptr<MessageTypeWithAllocator> message;
  DataSampleHeader_ptr header;
  bool new_instance;
  MonotonicTimePoint expiration_time;
};
typedef OPENDDS_MAP(DDS::InstanceHandle_t, FilterDelayedSample) FilterDelayedSampleMap;
FilterDelayedSampleMap filter_delayed_sample_map_;
typedef OPENDDS_MULTIMAP(MonotonicTimePoint, DDS::InstanceHandle_t) FilterDelayedSampleQueue;
FilterDelayedSampleQueue filter_delayed_sample_queue_;

bool marshal_skip_serialize_;

};

template <typename MessageType>
void* DataReaderImpl_T<MessageType>::MessageTypeWithAllocator::operator new(size_t , ACE_New_Allocator& pool)
{
  typedef typename DataReaderImpl_T<MessageType>::MessageTypeMemoryBlock MessageTypeMemoryBlock;
  MessageTypeMemoryBlock* block =
    static_cast<MessageTypeMemoryBlock*>(pool.malloc(sizeof(MessageTypeMemoryBlock)));
  block->allocator_ = &pool;
  return block;
}

template <typename MessageType>
void DataReaderImpl_T<MessageType>::MessageTypeWithAllocator::operator delete(void* memory)
{
  if (memory) {
    MessageTypeMemoryBlock* block = static_cast<MessageTypeMemoryBlock*>(memory);
    block->allocator_->free(block);
  }
}

template <typename MessageType>
void DataReaderImpl_T<MessageType>::MessageTypeWithAllocator::operator delete(void* memory, ACE_New_Allocator&)
{
  operator delete(memory);
}

}
}

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif /* OPENDDS_DDS_DCPS_DATAREADERIMPL_T_H */
