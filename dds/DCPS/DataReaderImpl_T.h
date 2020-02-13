#ifndef dds_DCPS_DataReaderImpl_T_h
#define dds_DCPS_DataReaderImpl_T_h
#include "dds/DCPS/MultiTopicImpl.h"
#include "dds/DCPS/RakeResults_T.h"
#include "dds/DCPS/SubscriberImpl.h"
#include "dds/DCPS/BuiltInTopicUtils.h"
#include "dds/DCPS/Util.h"
#include "dds/DCPS/TypeSupportImpl.h"
#include "dds/DCPS/Watchdog.h"
#include "dcps_export.h"
#include "dds/DCPS/GuidConverter.h"

#include "ace/Bound_Ptr.h"
#include "ace/Time_Value.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
  namespace DCPS {

  /** Servant for DataReader interface of Traits::MessageType data type.
   *
   * See the DDS specification, OMG formal/04-12-02, for a description of
   * this interface.
   *
   */
  template <typename MessageType>
    class
#if ( __GNUC__ == 4 && __GNUC_MINOR__ == 1)
    OpenDDS_Dcps_Export
#endif
    DataReaderImpl_T
    : public virtual OpenDDS::DCPS::LocalObject<typename DDSTraits<MessageType>::DataReaderType>,
      public virtual OpenDDS::DCPS::DataReaderImpl
  {
  public:
    typedef DDSTraits<MessageType> TraitsType;
    typedef typename TraitsType::MessageSequenceType MessageSequenceType;

    typedef OPENDDS_MAP_CMP_T(MessageType, DDS::InstanceHandle_t,
                              typename TraitsType::LessThanType) InstanceMap;

    class SharedInstanceMap
      : public RcObject
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
    };

    struct MessageTypeMemoryBlock {
      MessageTypeWithAllocator element_;
      ACE_New_Allocator* allocator_;
    };

    typedef OpenDDS::DCPS::Cached_Allocator_With_Overflow<MessageTypeMemoryBlock, ACE_Null_Mutex>  DataAllocator;

    typedef typename TraitsType::DataReaderType Interface;

    DataReaderImpl_T (void)
    : filter_delayed_handler_(make_rch<FilterDelayedHandler>(ref(*this)))
    {
    }

    virtual ~DataReaderImpl_T (void)
    {
      for (typename InstanceMap::iterator it = instance_map_.begin();
           it != instance_map_.end(); ++it)
        {
          OpenDDS::DCPS::SubscriptionInstance_rch ptr = get_handle_instance(it->second);
          this->purge_data(ptr);
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
                   this->get_n_chunks ()));

      return DDS::RETCODE_OK;
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
                        this->sample_lock_,
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
                        this->sample_lock_,
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

      ACE_GUARD_RETURN (ACE_Recursive_Thread_Mutex, guard, this->sample_lock_,
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

      ACE_GUARD_RETURN (ACE_Recursive_Thread_Mutex, guard, this->sample_lock_,
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
                                             DDS::SampleInfo& sample_info)
  {
    bool found_data = false;

    ACE_GUARD_RETURN(ACE_Recursive_Thread_Mutex, guard, sample_lock_, DDS::RETCODE_ERROR);

    const typename InstanceMap::iterator the_end = instance_map_.end();
    for (typename InstanceMap::iterator it = instance_map_.begin(); it != the_end; ++it) {
      const SubscriptionInstance_rch ptr = get_handle_instance(it->second);

      bool most_recent_generation = false;

      for (ReceivedDataElement* item = ptr->rcvd_samples_.head_; item; item = item->next_data_sample_) {
#ifndef OPENDDS_NO_OBJECT_MODEL_PROFILE
        if (item->coherent_change_) continue;
#endif

        if (item->sample_state_ & DDS::NOT_READ_SAMPLE_STATE) {
          if (item->registered_data_) {
            received_data = *static_cast<MessageType*>(item->registered_data_);
          }
          ptr->instance_state_->sample_info(sample_info, item);
          item->sample_state_ = DDS::READ_SAMPLE_STATE;

          if (!most_recent_generation) {
            most_recent_generation = ptr->instance_state_->most_recent_generation(item);
          }
          found_data = true;
        }
        if (found_data) {
          break;
        }
      }

      if (found_data) {
        if (most_recent_generation) {
          ptr->instance_state_->accessed();
        }
        // Get the sample_ranks, generation_ranks, and
        // absolute_generation_ranks for this info_seq
        this->sample_info(sample_info, ptr->rcvd_samples_.tail_);

        break;
      }
    }

    post_read_or_take();
    return found_data ? DDS::RETCODE_OK : DDS::RETCODE_NO_DATA;
  }

  virtual DDS::ReturnCode_t take_next_sample(MessageType& received_data,
                                             DDS::SampleInfo& sample_info)
  {
    bool found_data = false;
    ACE_GUARD_RETURN(ACE_Recursive_Thread_Mutex, guard, sample_lock_, DDS::RETCODE_ERROR);

    const typename InstanceMap::iterator the_end = instance_map_.end();
    for (typename InstanceMap::iterator it = instance_map_.begin(); it != the_end; ++it) {
      DDS::InstanceHandle_t handle = it->second;
      OpenDDS::DCPS::SubscriptionInstance_rch ptr = get_handle_instance(handle);

      bool most_recent_generation = false;

      OpenDDS::DCPS::ReceivedDataElement* tail = 0;
      OpenDDS::DCPS::ReceivedDataElement* next;
      OpenDDS::DCPS::ReceivedDataElement* item = ptr->rcvd_samples_.head_;
      while (item) {
#ifndef OPENDDS_NO_OBJECT_MODEL_PROFILE
        if (item->coherent_change_) {
          item = item->next_data_sample_;
          continue;
        }
#endif
        if (item->sample_state_ & DDS::NOT_READ_SAMPLE_STATE) {
          if (item->registered_data_) {
            received_data = *static_cast<MessageType*>(item->registered_data_);
          }
          ptr->instance_state_->sample_info(sample_info, item);

          item->sample_state_ = DDS::READ_SAMPLE_STATE;

          if (!most_recent_generation) {
            most_recent_generation = ptr->instance_state_->most_recent_generation(item);
          }

          if (item == ptr->rcvd_samples_.tail_) {
            tail = ptr->rcvd_samples_.tail_;
            item = item->next_data_sample_;

          } else {
            next = item->next_data_sample_;

            ptr->rcvd_samples_.remove(item);
            item->dec_ref();

            item = next;
          }

          found_data = true;
        }

        if (found_data) {
          break;
        }
      }

      if (found_data) {
        if (most_recent_generation) {
          ptr->instance_state_->accessed();
        }

        //
        // Get the sample_ranks, generation_ranks, and
        // absolute_generation_ranks for this info_seq
        //
        if (tail) {
          this->sample_info(sample_info, tail);

          ptr->rcvd_samples_.remove(tail);
          tail->dec_ref();

        } else {
          this->sample_info(sample_info, ptr->rcvd_samples_.tail_);
        }

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
                      this->sample_lock_,
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
                      this->sample_lock_,
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

    ACE_GUARD_RETURN (ACE_Recursive_Thread_Mutex, guard, this->sample_lock_,
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

    ACE_GUARD_RETURN (ACE_Recursive_Thread_Mutex, guard, this->sample_lock_,
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

    ACE_GUARD_RETURN (ACE_Recursive_Thread_Mutex, guard, this->sample_lock_,
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

    ACE_GUARD_RETURN (ACE_Recursive_Thread_Mutex, guard, this->sample_lock_,
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

  virtual DDS::ReturnCode_t get_key_value (
                                             MessageType & key_holder,
                                             DDS::InstanceHandle_t handle)
  {
    ACE_GUARD_RETURN (ACE_Recursive_Thread_Mutex,
                      guard,
                      this->sample_lock_,
                      DDS::RETCODE_ERROR);

    typename InstanceMap::iterator const the_end = instance_map_.end ();
    for (typename InstanceMap::iterator it = instance_map_.begin ();
         it != the_end;
         ++it)
      {
        if (it->second == handle)
          {
            key_holder = it->first;
            return DDS::RETCODE_OK;
          }
      }

    return DDS::RETCODE_BAD_PARAMETER;
  }

  virtual DDS::InstanceHandle_t lookup_instance (const MessageType & instance_data)
  {
    typename InstanceMap::const_iterator const it = instance_map_.find(instance_data);

    if (it == instance_map_.end())
      {
        return DDS::HANDLE_NIL;
      }
    else
      {
        return it->second;
      }
  }

  virtual DDS::ReturnCode_t auto_return_loan(void* seq)
  {
    MessageSequenceType& received_data =
      *static_cast< MessageSequenceType*> (seq);

    if (!received_data.release())
      {
        // this->release_loan(received_data);
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

    for (SubscriptionInstanceMapType::iterator iter = instances_.begin(), end = instances_.end(); iter != end; ++iter) {
      SubscriptionInstance& inst = *iter->second;

      if (inst.instance_state_->match(view_states, instance_states)) {
        for (ReceivedDataElement* item = inst.rcvd_samples_.head_; item != 0; item = item->next_data_sample_) {
          if ((item->sample_state_ & sample_states)
#ifndef OPENDDS_NO_OBJECT_MODEL_PROFILE
              && !item->coherent_change_
#endif
              && item->registered_data_) {
            if (!item->valid_data_ && filter_has_non_key_fields) {
              continue;
            }
            if (evaluator.eval(*static_cast<MessageType*>(item->registered_data_), params)) {
              return true;
            }
          }
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
                      this->sample_lock_,
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
                      this->sample_lock_,
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
                                             DDS::ViewStateKind view)
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

    // Call store_instance_data() once or twice, depending on if we need to
    // process the INSTANCE_REGISTRATION.  In either case, store_instance_data()
    // owns the memory for the sample and it must come from the correct allocator.
    for (int i = 0; i < 2; ++i) {
      if (i == 0 && inst != DDS::HANDLE_NIL) continue;

      DataSampleHeader header;
      const int msg = i ? SAMPLE_DATA : INSTANCE_REGISTRATION;
      header.message_id_ = static_cast<char>(msg);
      bool just_registered;
      unique_ptr<MessageTypeWithAllocator> data(new (*data_allocator()) MessageTypeWithAllocator(sample));
      store_instance_data(move(data), header, instance, just_registered, filtered);
      if (instance) inst = instance->instance_handle_;
    }

    if (!filtered) {
      if (view == DDS::NOT_NEW_VIEW_STATE) {
        if (instance) instance->instance_state_->accessed();
      }
      notify_read_conditions();
    }
    return inst;
  }

  void set_instance_state(DDS::InstanceHandle_t instance,
                          DDS::InstanceStateKind state)
  {
    using namespace OpenDDS::DCPS;
    ACE_GUARD(ACE_Recursive_Thread_Mutex, guard, sample_lock_);

    SubscriptionInstance_rch si = get_handle_instance(instance);
    if (si && state != DDS::ALIVE_INSTANCE_STATE) {
      DataSampleHeader header;
      const int msg = (state == DDS::NOT_ALIVE_DISPOSED_INSTANCE_STATE)
        ? DISPOSE_INSTANCE : UNREGISTER_INSTANCE;
      header.message_id_ = static_cast<char>(msg);
      bool just_registered, filtered;
      unique_ptr<MessageTypeWithAllocator> data(new (*data_allocator()) MessageTypeWithAllocator);
      get_key_value(*data, instance);
      store_instance_data(move(data), header, si, just_registered, filtered);
      if (!filtered)
      {
        notify_read_conditions();
      }
    }
  }

  virtual void lookup_instance(const OpenDDS::DCPS::ReceivedDataSample& sample,
                               OpenDDS::DCPS::SubscriptionInstance_rch& instance)
  {
    //!!! caller should already have the sample_lock_

    MessageType data;

    const bool cdr = sample.header_.cdr_encapsulation_;

    OpenDDS::DCPS::Serializer ser(
      sample.sample_.get(),
      sample.header_.byte_order_ != ACE_CDR_BYTE_ORDER,
      cdr ? OpenDDS::DCPS::Serializer::ALIGN_CDR
          : OpenDDS::DCPS::Serializer::ALIGN_NONE);

    if (cdr) {
      ACE_CDR::ULong header;
      if (!(ser >> header)) {
        ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) %CDataReaderImpl::lookup_instance ")
                  ACE_TEXT("deserialization header failed.\n"),
                  TraitsType::type_name()));
        return;
      }

    // Start counting byte-offset AFTER header
    ser.reset_alignment();
    }

    if (sample.header_.key_fields_only_) {
      ser >> OpenDDS::DCPS::KeyOnly< MessageType>(data);
    } else {
      ser >> data;
    }

    if (!ser.good_bit()) {
      ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) %CDataReaderImpl::lookup_instance ")
                 ACE_TEXT("deserialization failed.\n"),
                 TraitsType::type_name()));
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
            const TimeDuration new_interval(qos.time_based_filter.minimum_separation);
            filter_delayed_handler_->reset_interval(new_interval);
          } else {
            filter_delayed_handler_->cancel();
          }
        }
        // else no existing timers to change/cancel
      }
      // else no qos change so nothing to change
    }

    DataReaderImpl::qos_change(qos);
  }

protected:

  virtual void dds_demarshal(const OpenDDS::DCPS::ReceivedDataSample& sample,
                             OpenDDS::DCPS::SubscriptionInstance_rch& instance,
                             bool& just_registered,
                             bool& filtered,
                             OpenDDS::DCPS::MarshalingType marshaling_type)
  {
    unique_ptr<MessageTypeWithAllocator> data(new (*data_allocator()) MessageTypeWithAllocator);
    const bool cdr = sample.header_.cdr_encapsulation_;

    OpenDDS::DCPS::Serializer ser(
                                  sample.sample_.get(),
                                  sample.header_.byte_order_ != ACE_CDR_BYTE_ORDER,
                                  cdr ? OpenDDS::DCPS::Serializer::ALIGN_CDR : OpenDDS::DCPS::Serializer::ALIGN_NONE);

    if (cdr) {
      ACE_CDR::ULong header;
      if (!(ser >> header)) {
        ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) %CDataReaderImpl::dds_demarshal ")
                  ACE_TEXT("deserialization header failed, dropping sample.\n"),
                  TraitsType::type_name()));
        return;
      }

    // Start counting byte-offset AFTER header
    ser.reset_alignment();
    }

    if (marshaling_type == OpenDDS::DCPS::KEY_ONLY_MARSHALING) {
      ser >> OpenDDS::DCPS::KeyOnly< MessageType>(*data);
    } else {
      ser >> *data;
    }

    if (!ser.good_bit()) {
      ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) %CDataReaderImpl::dds_demarshal ")
                 ACE_TEXT("deserialization failed, dropping sample.\n"),
                 TraitsType::type_name()));
      return;
    }

#ifndef OPENDDS_NO_CONTENT_FILTERED_TOPIC
    if (!sample.header_.content_filter_) { // if this is true, the writer has already filtered
      using OpenDDS::DCPS::ContentFilteredTopicImpl;
      if (content_filtered_topic_) {
        const bool sample_only_has_key_fields = !sample.header_.valid_data();
        const MessageType& type = static_cast<MessageType&>(*data);
        if (!content_filtered_topic_->filter(type, sample_only_has_key_fields)) {
          filtered = true;
          return;
        }
      }
    }
#endif

    store_instance_data(move(data), sample.header_, instance, just_registered, filtered);
  }

  virtual void dispose_unregister(const OpenDDS::DCPS::ReceivedDataSample& sample,
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
    this->dds_demarshal(sample, instance, just_registered, filtered, marshaling);
  }

  virtual void purge_data(OpenDDS::DCPS::SubscriptionInstance_rch instance)
  {
    filter_delayed_handler_->drop_sample(instance->instance_handle_);


    instance->instance_state_->cancel_release();

    while (instance->rcvd_samples_.size_ > 0)
      {
        OpenDDS::DCPS::ReceivedDataElement* head =
          instance->rcvd_samples_.remove_head();
        head->dec_ref();
      }
  }

  virtual void release_instance_i (DDS::InstanceHandle_t handle)
  {
    typename InstanceMap::iterator const the_end = instance_map_.end ();
    typename InstanceMap::iterator it = instance_map_.begin ();
    while (it != the_end)
      {
        if (it->second == handle)
          {
            typename InstanceMap::iterator curIt = it;
            ++ it;
            instance_map_.erase (curIt);
          }
        else
          ++ it;
      }
  }

private:

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

#ifndef OPENDDS_NO_OBJECT_MODEL_PROFILE
  if (!group_coherent_ordered) {
#endif
    for (typename InstanceMap::iterator it = instance_map_.begin(),
         the_end = instance_map_.end(); it != the_end; ++it) {

      const DDS::InstanceHandle_t handle = it->second;
      const SubscriptionInstance_rch inst = get_handle_instance(handle);

      if (inst->instance_state_->match(view_states, instance_states)) {
        size_t i(0);
        for (ReceivedDataElement* item = inst->rcvd_samples_.head_; item; item = item->next_data_sample_) {
          if ((item->sample_state_ & sample_states)
#ifndef OPENDDS_NO_OBJECT_MODEL_PROFILE
              && !item->coherent_change_
#endif
              ) {
            results.insert_sample(item, inst, ++i);
          }
        }
      }
    }
#ifndef OPENDDS_NO_OBJECT_MODEL_PROFILE
  } else {
    const RakeData item = group_coherent_ordered_data_.get_data();
    results.insert_sample(item.rde_, item.si_, item.index_in_instance_);
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

#ifndef OPENDDS_NO_OBJECT_MODEL_PROFILE
  if (!group_coherent_ordered) {
#endif

    for (typename InstanceMap::iterator it = instance_map_.begin(), the_end = instance_map_.end(); it != the_end; ++it) {

      const DDS::InstanceHandle_t handle = it->second;
      const SubscriptionInstance_rch inst = get_handle_instance(handle);

      if (inst->instance_state_->match(view_states, instance_states)) {
        size_t i(0);
        for (ReceivedDataElement* item = inst->rcvd_samples_.head_; item; item = item->next_data_sample_) {
          if ((item->sample_state_ & sample_states)
#ifndef OPENDDS_NO_OBJECT_MODEL_PROFILE
              && !item->coherent_change_
#endif
              ) {
            results.insert_sample(item, inst, ++i);
          }
        }
      }
    }
#ifndef OPENDDS_NO_OBJECT_MODEL_PROFILE
  } else {
    const RakeData item = group_coherent_ordered_data_.get_data();
    results.insert_sample(item.rde_, item.si_, item.index_in_instance_);
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
    size_t i(0);
    for (ReceivedDataElement* item = inst->rcvd_samples_.head_; item; item = item->next_data_sample_) {
      if ((item->sample_state_ & sample_states)
#ifndef OPENDDS_NO_OBJECT_MODEL_PROFILE
          && !item->coherent_change_
#endif
          ) {
        results.insert_sample(item, inst, ++i);
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
    const GuidConverter conv(get_subscription_id());
    ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) DataReaderImpl_T::read_instance_i: ")
               ACE_TEXT("will return no data reading sub %C because:\n  %C\n"),
               OPENDDS_STRING(conv).c_str(), msg.c_str()));
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

  if (inst->instance_state_->match(view_states, instance_states)) {
    size_t i(0);
    for (ReceivedDataElement* item = inst->rcvd_samples_.head_; item; item = item->next_data_sample_) {
      if ((item->sample_state_ & sample_states)
#ifndef OPENDDS_NO_OBJECT_MODEL_PROFILE
          && !item->coherent_change_
#endif
          ) {
        results.insert_sample(item, inst, ++i);
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
  DDS::InstanceHandle_t handle(DDS::HANDLE_NIL);

  ACE_GUARD_RETURN(ACE_Recursive_Thread_Mutex, guard, sample_lock_, DDS::RETCODE_ERROR);

  typename InstanceMap::iterator it;
  const typename InstanceMap::iterator the_end = instance_map_.end ();

  if (a_handle == DDS::HANDLE_NIL) {
      it = instance_map_.begin();

  } else {
    for (it = instance_map_.begin(); it != the_end; ++it) {
      if (a_handle == it->second) {
        ++it;
        break;
      }
    }
  }

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
  ACE_GUARD_RETURN(ACE_Recursive_Thread_Mutex, guard, this->sample_lock_, DDS::RETCODE_ERROR);

  typename InstanceMap::iterator it;
  const typename InstanceMap::iterator the_end = instance_map_.end ();

  if (a_handle == DDS::HANDLE_NIL) {
    it = instance_map_.begin();

  } else {
    for (it = instance_map_.begin(); it != the_end; ++it) {
      if (a_handle == it->second) {
        ++it;
        break;
      }
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

void store_instance_data(
                         unique_ptr<MessageTypeWithAllocator> instance_data,
                         const OpenDDS::DCPS::DataSampleHeader& header,
                         OpenDDS::DCPS::SubscriptionInstance_rch& instance_ptr,
                         bool& just_registered,
                         bool& filtered)
{
  const bool is_dispose_msg =
    header.message_id_ == OpenDDS::DCPS::DISPOSE_INSTANCE ||
    header.message_id_ == OpenDDS::DCPS::DISPOSE_UNREGISTER_INSTANCE;
  const bool is_unregister_msg =
    header.message_id_ == OpenDDS::DCPS::UNREGISTER_INSTANCE ||
    header.message_id_ == OpenDDS::DCPS::DISPOSE_UNREGISTER_INSTANCE;

  // not filtering any data, except what is specifically identified as filtered below
  filtered = false;

  DDS::InstanceHandle_t handle(DDS::HANDLE_NIL);

  //!!! caller should already have the sample_lock_
  //We will unlock it before calling into listeners

  typename InstanceMap::const_iterator const it = instance_map_.find(*instance_data);

  if ((is_dispose_msg || is_unregister_msg) && it == instance_map_.end())
  {
    return;
  }

  if (it == instance_map_.end())
  {
    std::size_t instances_size = 0;
    {
      ACE_GUARD(ACE_Recursive_Thread_Mutex, instance_guard, instances_lock_);
      instances_size = instances_.size();
    }
    if ((this->qos_.resource_limits.max_instances != DDS::LENGTH_UNLIMITED) &&
      ((::CORBA::Long) instances_size >= this->qos_.resource_limits.max_instances))
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

#ifndef OPENDDS_NO_OWNERSHIP_KIND_EXCLUSIVE
    SharedInstanceMap_rch inst;
    bool new_handle = true;
    if (this->is_exclusive_ownership_) {
      OwnershipManagerPtr owner_manager = this->ownership_manager();

      if (!owner_manager || owner_manager->instance_lock_acquire () != 0) {
        ACE_ERROR ((LM_ERROR,
                    ACE_TEXT("(%P|%t) ")
                    ACE_TEXT("%CDataReaderImpl::")
                    ACE_TEXT("store_instance_data, ")
                    ACE_TEXT("acquire instance_lock failed. \n"), TraitsType::type_name()));
        return;
      }

      inst = dynamic_rchandle_cast<SharedInstanceMap>(
        owner_manager->get_instance_map(this->topic_servant_->type_name(), this));
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
    handle = handle == DDS::HANDLE_NIL ? this->get_next_handle( key) : handle;
    OpenDDS::DCPS::SubscriptionInstance_rch instance =
      OpenDDS::DCPS::make_rch<OpenDDS::DCPS::SubscriptionInstance>(
        this,
        this->qos_,
        ref(this->instances_lock_),
        handle);

    instance->instance_handle_ = handle;

    {
      ACE_GUARD(ACE_Recursive_Thread_Mutex, instance_guard, instances_lock_);
      int ret = OpenDDS::DCPS::bind(instances_, handle, instance);

      if (ret != 0)
      {
        ACE_ERROR ((LM_ERROR,
                    ACE_TEXT("(%P|%t) ")
                    ACE_TEXT("%CDataReaderImpl::")
                    ACE_TEXT("store_instance_data, ")
                    ACE_TEXT("insert handle failed. \n"), TraitsType::type_name()));
        return;
      }
    }

#ifndef OPENDDS_NO_OWNERSHIP_KIND_EXCLUSIVE
    OwnershipManagerPtr owner_manager = this->ownership_manager();

    if (owner_manager) {
      if (!inst) {
        inst = make_rch<SharedInstanceMap>();
        owner_manager->set_instance_map(
          this->topic_servant_->type_name(),
          inst,
          this);
      }

      if (new_handle) {
        std::pair<typename InstanceMap::iterator, bool> bpair =
          inst->insert(typename InstanceMap::value_type(*instance_data,
            handle));
        if (bpair.second == false)
        {
          ACE_ERROR ((LM_ERROR,
                      ACE_TEXT("(%P|%t) ")
                      ACE_TEXT("%CDataReaderImpl::")
                      ACE_TEXT("store_instance_data, ")
                      ACE_TEXT("insert to participant scope %C failed. \n"), TraitsType::type_name(), TraitsType::type_name()));
          return;
        }
      }

      if (owner_manager->instance_lock_release () != 0) {
        ACE_ERROR ((LM_ERROR,
                    ACE_TEXT("(%P|%t) ")
                    ACE_TEXT("%CDataReaderImpl::")
                    ACE_TEXT("store_instance_data, ")
                    ACE_TEXT("release instance_lock failed. \n"), TraitsType::type_name()));
        return;
      }
    }
#endif

    std::pair<typename InstanceMap::iterator, bool> bpair =
      instance_map_.insert(typename InstanceMap::value_type(*instance_data,
        handle));
    if (bpair.second == false)
    {
      ACE_ERROR ((LM_ERROR,
                  ACE_TEXT("(%P|%t) ")
                  ACE_TEXT("%CDataReaderImpl::")
                  ACE_TEXT("store_instance_data, ")
                  ACE_TEXT("insert %C failed. \n"), TraitsType::type_name(), TraitsType::type_name()));
      return;
    }
  }
  else
  {
    just_registered = false;
    handle = it->second;
  }

  if (header.message_id_ != OpenDDS::DCPS::INSTANCE_REGISTRATION)
  {
    instance_ptr = get_handle_instance(handle);

    if (header.message_id_ == OpenDDS::DCPS::SAMPLE_DATA)
    {
      filtered = ownership_filter_instance(instance_ptr, header.publication_id_);

      TimeDuration filter_time_expired;
      if (!filtered && time_based_filter_instance(instance_ptr, filter_time_expired)) {
        filtered = true;
        if (this->qos_.reliability.kind == DDS::RELIABLE_RELIABILITY_QOS) {
          filter_delayed_handler_->delay_sample(
            handle, move(instance_data), header, just_registered, filter_time_expired);
        }
      } else {
        // nothing time based filtered now
        filter_delayed_handler_->clear_sample(handle);

      }

      if (filtered) {
        return;
      }
    }

    finish_store_instance_data(move(instance_data), header, instance_ptr, is_dispose_msg, is_unregister_msg);
  }
  else
  {
    instance_ptr = this->get_handle_instance(handle);
    instance_ptr->instance_state_->lively(header.publication_id_);
  }
}

void finish_store_instance_data(unique_ptr<MessageTypeWithAllocator> instance_data, const DataSampleHeader& header,
  SubscriptionInstance_rch instance_ptr, bool is_dispose_msg, bool is_unregister_msg )
{
  if ((this->qos_.resource_limits.max_samples_per_instance !=
        DDS::LENGTH_UNLIMITED) &&
      (instance_ptr->rcvd_samples_.size_ >=
        this->qos_.resource_limits.max_samples_per_instance)) {

    // According to spec 1.2, Samples that contain no data do not
    // count towards the limits imposed by the RESOURCE_LIMITS QoS policy
    // so do not remove the oldest sample when unregister/dispose
    // message arrives.

    if (!is_dispose_msg && !is_unregister_msg
      && instance_ptr->rcvd_samples_.head_->sample_state_
      == DDS::NOT_READ_SAMPLE_STATE)
    {
      // for now the implemented QoS means that if the head sample
      // is NOT_READ then none are read.
      // TBD - in future we will reads may not read in order so
      //       just looking at the head will not be enough.
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
      OpenDDS::DCPS::ReceivedDataElement *item =
        instance_ptr->rcvd_samples_.head_;
      instance_ptr->rcvd_samples_.remove(item);
      item->dec_ref();
    }
  }
  else if (this->qos_.resource_limits.max_samples != DDS::LENGTH_UNLIMITED)
  {
    CORBA::Long total_samples = 0;
    {
      ACE_GUARD(ACE_Recursive_Thread_Mutex, instance_guard, this->instances_lock_);
      for (OpenDDS::DCPS::DataReaderImpl::SubscriptionInstanceMapType::iterator iter = instances_.begin();
        iter != instances_.end();
        ++iter) {
        OpenDDS::DCPS::SubscriptionInstance_rch ptr = iter->second;

        total_samples += (CORBA::Long) ptr->rcvd_samples_.size_;
      }
    }

    if (total_samples >= this->qos_.resource_limits.max_samples)
    {
      // According to spec 1.2, Samples that contain no data do not
      // count towards the limits imposed by the RESOURCE_LIMITS QoS policy
      // so do not remove the oldest sample when unregister/dispose
      // message arrives.

      if (!is_dispose_msg && !is_unregister_msg
        && instance_ptr->rcvd_samples_.head_->sample_state_
        == DDS::NOT_READ_SAMPLE_STATE)
      {
        // for now the implemented QoS means that if the head sample
        // is NOT_READ then none are read.
        // TBD - in future we will reads may not read in order so
        //       just looking at the head will not be enough.
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
          instance_ptr->rcvd_samples_.head_;
        instance_ptr->rcvd_samples_.remove(item);
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

  OpenDDS::DCPS::ReceivedDataElement *ptr =
    new (*rd_allocator_.get()) OpenDDS::DCPS::ReceivedDataElementWithType<MessageTypeWithAllocator>(header,instance_data.release(), &this->sample_lock_);

  ptr->disposed_generation_count_ =
    instance_ptr->instance_state_->disposed_generation_count();
  ptr->no_writers_generation_count_ =
    instance_ptr->instance_state_->no_writers_generation_count();

  instance_ptr->last_sequence_ = header.sequence_;

  instance_ptr->rcvd_strategy_->add(ptr);

  if (! is_dispose_msg  && ! is_unregister_msg
      && instance_ptr->rcvd_samples_.size_ > get_depth())
    {
      OpenDDS::DCPS::ReceivedDataElement* head_ptr =
        instance_ptr->rcvd_samples_.head_;

      instance_ptr->rcvd_samples_.remove(head_ptr);

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
    if (!sub)
      return;

    sub->set_status_changed_flag(DDS::DATA_ON_READERS_STATUS, true);

    set_status_changed_flag(DDS::DATA_AVAILABLE_STATUS, true);

    DDS::SubscriberListener_var sub_listener =
        sub->listener_for(DDS::DATA_ON_READERS_STATUS);
    if (!CORBA::is_nil(sub_listener.in()) && !this->coherent_)
      {
        ACE_GUARD(typename DataReaderImpl::Reverse_Lock_t, unlock_guard, reverse_sample_lock_);

        sub_listener->on_data_on_readers(sub.in());
        sub->set_status_changed_flag(DDS::DATA_ON_READERS_STATUS, false);
      }
    else
      {
        sub->notify_status_condition();

        DDS::DataReaderListener_var listener =
            listener_for (DDS::DATA_AVAILABLE_STATUS);

        if (!CORBA::is_nil(listener.in()))
          {
            ACE_GUARD(typename DataReaderImpl::Reverse_Lock_t, unlock_guard, reverse_sample_lock_);

            listener->on_data_available(this);
            set_status_changed_flag(DDS::DATA_AVAILABLE_STATUS, false);
            sub->set_status_changed_flag(DDS::DATA_ON_READERS_STATUS, false);
          }
        else
          {
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
DDS::ReturnCode_t check_inputs (
                                const char* method_name,
                                MessageSequenceType & received_data,
                                DDS::SampleInfoSeq & info_seq,
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

class FilterDelayedHandler : public Watchdog {
public:
  FilterDelayedHandler(DataReaderImpl_T<MessageType>& data_reader_impl)
  // Watchdog's interval_ only used for resetting current intervals
  : Watchdog(TimeDuration::zero_value)
  , data_reader_impl_(data_reader_impl)
  {
  }

  virtual ~FilterDelayedHandler()
  {
  }

  void cancel()
  {
    cancel_all();
    cleanup();
  }

  void delay_sample(DDS::InstanceHandle_t handle,
                    unique_ptr<MessageTypeWithAllocator> data,
                    const OpenDDS::DCPS::DataSampleHeader& header,
                    const bool just_registered,
                    const TimeDuration& filter_time_expired)
  {
    // sample_lock_ should already be held
    RcHandle<DataReaderImpl_T<MessageType> > data_reader_impl(data_reader_impl_.lock());

    if (!data_reader_impl) {
      return;
    }

    MessageTypeWithAllocator* instance_data = data.get();

    DataSampleHeader_ptr hdr(new OpenDDS::DCPS::DataSampleHeader(header));

    typename FilterDelayedSampleMap::iterator i = map_.find(handle);
    if (i == map_.end()) {

      // emplace()/insert() only if the sample is going to be
      // new (otherwise we call move(data) twice).
      std::pair<typename FilterDelayedSampleMap::iterator, bool> result =
#ifdef ACE_HAS_CPP11
      map_.emplace(std::piecewise_construct,
                   std::forward_as_tuple(handle),
                   std::forward_as_tuple(move(data), hdr, just_registered));
#else
      map_.insert(std::make_pair(handle, FilterDelayedSample(move(data), hdr, just_registered)));
#endif
      FilterDelayedSample& sample = result.first->second;

      const TimeDuration interval(
        data_reader_impl->qos_.time_based_filter.minimum_separation);

      const TimeDuration filter_time_remaining =
        TimeDuration(data_reader_impl->qos_.time_based_filter.minimum_separation) -
        filter_time_expired;

      long timer_id = -1;

      {
        ACE_GUARD(Reverse_Lock_t, unlock_guard, data_reader_impl->reverse_sample_lock_);
        timer_id = schedule_timer(reinterpret_cast<const void*>(intptr_t(handle)),
          filter_time_remaining, interval);
      }

      // ensure that another sample has not replaced this while the lock was released
      if (instance_data == sample.message.get()) {
        sample.timer_id = timer_id;
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

    typename FilterDelayedSampleMap::iterator sample = map_.find(handle);
    if (sample != map_.end()) {
      // leave the entry in the container, so that the key remains valid if the reactor is waiting on this lock while this is occurring
      sample->second.message.reset();
    }
  }

  void drop_sample(DDS::InstanceHandle_t handle)
  {
    // sample_lock_ should already be held

    typename FilterDelayedSampleMap::iterator sample = map_.find(handle);
    if (sample != map_.end()) {
      {
        RcHandle<DataReaderImpl_T<MessageType> > data_reader_impl(data_reader_impl_.lock());
        if (data_reader_impl) {
          ACE_GUARD(Reverse_Lock_t, unlock_guard, data_reader_impl->reverse_sample_lock_);
          cancel_timer(sample->second.timer_id);
        }
      }

      // use the handle to erase, since the sample lock was released
      map_.erase(handle);
    }
  }

private:

  int handle_timeout(const ACE_Time_Value&, const void* act)
  {
    DDS::InstanceHandle_t handle = static_cast<DDS::InstanceHandle_t>(reinterpret_cast<intptr_t>(act));

    RcHandle<DataReaderImpl_T<MessageType> > data_reader_impl(data_reader_impl_.lock());
    if (!data_reader_impl) {
      return -1;
    }

    SubscriptionInstance_rch instance = data_reader_impl->get_handle_instance(handle);
    if (!instance) {
      return 0;
    }

    long cancel_timer_id = -1;

    {
      ACE_GUARD_RETURN(ACE_Recursive_Thread_Mutex, guard, data_reader_impl->sample_lock_, -1);

      typename FilterDelayedSampleMap::iterator data = map_.find(handle);
      if (data == map_.end()) {
        return 0;
      }

      if (data->second.message) {
        const bool NOT_DISPOSE_MSG = false;
        const bool NOT_UNREGISTER_MSG = false;
        // clear the message, since ownership is being transfered to finish_store_instance_data.

        instance->last_accepted_.set_to_now();
        const DataSampleHeader_ptr header = data->second.header;
        const bool new_instance = data->second.new_instance;

        // should not use data iterator anymore, since finish_store_instance_data releases sample_lock_
        data_reader_impl->finish_store_instance_data(
          move(data->second.message),
          *header,
          instance,
          NOT_DISPOSE_MSG,
          NOT_UNREGISTER_MSG);

        data_reader_impl->accept_sample_processing(instance, *header, new_instance);
      } else {
        // this check is performed to handle the corner case where
        // store_instance_data received and delivered a sample, while this
        // method was waiting for the lock
        const TimeDuration interval(data_reader_impl->qos_.time_based_filter.minimum_separation);
        if (MonotonicTimePoint::now() - instance->last_sample_tv_ >= interval) {
          // nothing to process, so unregister this handle for timeout
          cancel_timer_id = data->second.timer_id;
          // no new data to process, so remove from container
          map_.erase(data);
        }
      }
    }

    if (cancel_timer_id != -1) {
      cancel_timer(cancel_timer_id);
    }
    return 0;
  }

  virtual void reschedule_deadline()
  {
    RcHandle<DataReaderImpl_T<MessageType> > data_reader_impl(data_reader_impl_.lock());

    if (data_reader_impl) {
      ACE_GUARD(ACE_Recursive_Thread_Mutex, guard, data_reader_impl->sample_lock_);

      for (typename FilterDelayedSampleMap::iterator sample = map_.begin(); sample != map_.end(); ++sample) {
        reset_timer_interval(sample->second.timer_id);
      }
    }
  }

  void cleanup()
  {
    RcHandle<DataReaderImpl_T<MessageType> > data_reader_impl(data_reader_impl_.lock());
    if (data_reader_impl) {
      ACE_GUARD(ACE_Recursive_Thread_Mutex, guard, data_reader_impl->sample_lock_);
      // insure instance_ptrs get freed
      map_.clear();
    }
  }

  WeakRcHandle<DataReaderImpl_T<MessageType> > data_reader_impl_;

  typedef ACE_Strong_Bound_Ptr<const OpenDDS::DCPS::DataSampleHeader, ACE_Null_Mutex> DataSampleHeader_ptr;

  struct FilterDelayedSample {

    FilterDelayedSample(unique_ptr<MessageTypeWithAllocator> msg, DataSampleHeader_ptr hdr, bool new_inst)
    : message(move(msg))
    , header(hdr)
    , new_instance(new_inst)
    , timer_id(-1) {
    }

    container_supported_unique_ptr<MessageTypeWithAllocator> message;
    DataSampleHeader_ptr header;
    bool new_instance;
    long timer_id;
  };


  typedef OPENDDS_MAP(DDS::InstanceHandle_t, FilterDelayedSample) FilterDelayedSampleMap;

  FilterDelayedSampleMap map_;
public:
  typedef typename DataReaderImpl_T<MessageType>::DataAllocator DataAllocator;
  //We put the data_allocator_ inside FilterDelayedHandler because the reactor thread in FilterDelayedHandler may be still alive
  // after the containing DataReaderImpl is destroyed. This avoids access violation during cleanup.
  unique_ptr<DataAllocator> data_allocator_;
};

unique_ptr<DataAllocator>& data_allocator() { return filter_delayed_handler_->data_allocator_; }

RcHandle<FilterDelayedHandler> filter_delayed_handler_;

InstanceMap  instance_map_;
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

#endif /* dds_DCPS_DataReaderImpl_T_h */
