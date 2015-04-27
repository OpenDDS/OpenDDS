#ifndef dds_DCPS_DataWriterImpl_T_h
#define dds_DCPS_DataWriterImpl_T_h

#include "dds/DCPS/PublicationInstance.h"

namespace OpenDDS {

/** Servant for DataWriter interface of the Traits::MessageType data type.
 *
 * See the DDS specification, OMG formal/04-12-02, for a description of
 * this interface.
 */
  template <typename Traits>
  class DataWriterImpl
    : public virtual OpenDDS::DCPS::LocalObject<typename Traits::DataWriterType>,
      public virtual OpenDDS::DCPS::DataWriterImpl
  {
  public:
    typedef Traits TraitsType;
    typedef typename Traits::MessageType MessageType;

    typedef OPENDDS_MAP_CMP(MessageType, ::DDS::InstanceHandle_t,
                            typename Traits::LessThanType) InstanceMap;
    typedef ::OpenDDS::DCPS::Dynamic_Cached_Allocator_With_Overflow<ACE_Thread_Mutex>  DataAllocator;

    /// Constructor
    DataWriterImpl (void)
      : marshaled_size_ (0)
      , key_marshaled_size_ (0)
      , data_allocator_ (0)
      , mb_allocator_ (0)
      , db_allocator_ (0)
    {
    }

    /// Destructor
    virtual ~DataWriterImpl (void)
    {
      delete data_allocator_;
      delete mb_allocator_;
      delete db_allocator_;
    }

  virtual ::DDS::InstanceHandle_t register_instance (
      const MessageType & instance)
    {
      ::DDS::Time_t const timestamp =
        ::OpenDDS::DCPS::time_value_to_time (ACE_OS::gettimeofday ());
      return register_instance_w_timestamp (instance, timestamp);
    }

  virtual ::DDS::InstanceHandle_t register_instance_w_timestamp (
      const MessageType & instance,
      const ::DDS::Time_t & timestamp)
    {
      ::DDS::InstanceHandle_t registered_handle = ::DDS::HANDLE_NIL;

      ::DDS::ReturnCode_t const ret
          = this->get_or_create_instance_handle(registered_handle,
                                                instance,
                                                timestamp);
      if (ret != ::DDS::RETCODE_OK)
        {
          ACE_ERROR ((LM_ERROR,
                      ACE_TEXT("(%P|%t) ")
                      ACE_TEXT("%sDataWriterImpl::")
                      ACE_TEXT("register_instance_w_timestamp, ")
                      ACE_TEXT("register failed error=%d.\n"),
                      Traits::type_name(),
                      ret));
        }

      return registered_handle;
    }

  virtual ::DDS::ReturnCode_t unregister_instance (
      const MessageType & instance,
      ::DDS::InstanceHandle_t handle)
    {
      ::DDS::Time_t const timestamp =
        ::OpenDDS::DCPS::time_value_to_time (ACE_OS::gettimeofday ());

      return unregister_instance_w_timestamp (instance,
                                              handle,
                                              timestamp);
    }

  virtual ::DDS::ReturnCode_t unregister_instance_w_timestamp (
      const MessageType & instance,
      ::DDS::InstanceHandle_t handle,
      const ::DDS::Time_t & timestamp)
    {
      ::DDS::InstanceHandle_t const registered_handle =
        this->lookup_instance(instance);

      if (registered_handle == ::DDS::HANDLE_NIL)
        {
          // This case could be the instance is not registered yet or
          // already unregistered.
          ACE_ERROR_RETURN ((LM_ERROR,
                             ACE_TEXT("(%P|%t) ")
                             ACE_TEXT("%sDataWriterImpl::")
                             ACE_TEXT("unregister_instance_w_timestamp, ")
                             ACE_TEXT("The instance is not registered.\n"),
                             Traits::type_name()),
                            ::DDS::RETCODE_ERROR);
        }
      else if (handle != ::DDS::HANDLE_NIL && handle != registered_handle)
        {
          ACE_ERROR_RETURN ((LM_ERROR,
                             ACE_TEXT("(%P|%t) ")
                             ACE_TEXT("%sDataWriterImpl::")
                             ACE_TEXT("unregister_w_timestamp, ")
                             ACE_TEXT("The given handle=%X is different from ")
                             ACE_TEXT("registered handle=%X.\n"),
                             Traits::type_name(),
                             handle, registered_handle),
                            ::DDS::RETCODE_ERROR);
        }

      // DataWriterImpl::unregister_instance_i will call back to inform the
      // DataWriter.
      // That the instance handle is removed from there and hence
      // DataWriter can remove the instance here.
      return OpenDDS::DCPS::DataWriterImpl::unregister_instance_i(handle, timestamp);
    }

  //WARNING: If the handle is non-nil and the instance is not registered
  //         then this operation may cause an access violation.
  //         This lack of safety helps performance.
  virtual ::DDS::ReturnCode_t write (
      const MessageType & instance_data,
      ::DDS::InstanceHandle_t handle)
    {
      ::DDS::Time_t const source_timestamp =
        ::OpenDDS::DCPS::time_value_to_time (ACE_OS::gettimeofday ());
      return write_w_timestamp (instance_data,
                                handle,
                                source_timestamp);
    }


  //WARNING: If the handle is non-nil and the instance is not registered
  //         then this operation may cause an access violation.
  //         This lack of safety helps performance.
  virtual ::DDS::ReturnCode_t write_w_timestamp (
      const MessageType & instance_data,
      ::DDS::InstanceHandle_t handle,
      const ::DDS::Time_t & source_timestamp)
    {
      //  This operation assumes the provided handle is valid. The handle
      //  provided will not be verified.

      if (handle == ::DDS::HANDLE_NIL) {
        ::DDS::InstanceHandle_t registered_handle = ::DDS::HANDLE_NIL;
        ::DDS::ReturnCode_t ret
            = this->get_or_create_instance_handle(registered_handle,
                                                  instance_data,
                                                  source_timestamp);
        if (ret != ::DDS::RETCODE_OK) {
          ACE_ERROR_RETURN((LM_ERROR,
                            ACE_TEXT("(%P|%t) ")
                            ACE_TEXT("%sDataWriterImpl::write, ")
                            ACE_TEXT("register failed err=%d.\n"),
                            Traits::type_name(),
                            ret),
                           ret);
        }

        handle = registered_handle;
      }

      // list of reader RepoIds that should not get data
      OpenDDS::DCPS::GUIDSeq_var filter_out;
#ifndef OPENDDS_NO_CONTENT_FILTERED_TOPIC
      if (TheServiceParticipant->publisher_content_filter()) {
        ACE_GUARD_RETURN(ACE_Thread_Mutex, reader_info_guard, this->reader_info_lock_, ::DDS::RETCODE_ERROR);
        for (RepoIdToReaderInfoMap::iterator iter = reader_info_.begin(),
               end = reader_info_.end(); iter != end; ++iter) {
          const ReaderInfo& ri = iter->second;
          if (!ri.eval_.is_nil()) {
            if (!filter_out.ptr()) {
              filter_out = new OpenDDS::DCPS::GUIDSeq;
            }
            if (!ri.eval_->eval(instance_data, ri.expression_params_)) {
              push_back(filter_out.inout(), iter->first);
            }
          }
        }
      }
#endif

      ACE_Message_Block* const marshalled =
        dds_marshal (instance_data, OpenDDS::DCPS::FULL_MARSHALING);

      return OpenDDS::DCPS::DataWriterImpl::write(marshalled, handle,
                                                  source_timestamp,
                                                  filter_out._retn());
    }

  virtual ::DDS::ReturnCode_t dispose (
      const MessageType & instance_data,
      ::DDS::InstanceHandle_t instance_handle)
    {
      ::DDS::Time_t const source_timestamp =
        ::OpenDDS::DCPS::time_value_to_time (ACE_OS::gettimeofday ());
      return dispose_w_timestamp (instance_data,
                                  instance_handle,
                                  source_timestamp);
    }

  virtual ::DDS::ReturnCode_t dispose_w_timestamp (
      const MessageType & instance_data,
      ::DDS::InstanceHandle_t instance_handle,
      const ::DDS::Time_t & source_timestamp)
    {
      if(instance_handle == ::DDS::HANDLE_NIL)
        {
          instance_handle = this->lookup_instance(instance_data);
          if (instance_handle == ::DDS::HANDLE_NIL)
            {
              ACE_ERROR_RETURN ((LM_ERROR,
                                 ACE_TEXT("(%P|%t) ")
                                 ACE_TEXT("%sDataWriterImpl::dispose, ")
                                 ACE_TEXT("The instance sample is not registered.\n"),
                                 Traits::type_name()),
                                ::DDS::RETCODE_ERROR);
            }
        }

      return OpenDDS::DCPS::DataWriterImpl::dispose(instance_handle,
                                                    source_timestamp);
    }

  virtual ::DDS::ReturnCode_t get_key_value (
      MessageType & key_holder,
      ::DDS::InstanceHandle_t handle)
    {
      ACE_GUARD_RETURN (ACE_Recursive_Thread_Mutex,
                        guard,
                        get_lock (),
                        ::DDS::RETCODE_ERROR);

      typename InstanceMap::iterator const the_end = instance_map_.end ();
      for (typename InstanceMap::iterator it = instance_map_.begin ();
           it != the_end;
           ++it)
        {
          if (it->second == handle)
            {
              key_holder = it->first;
              return ::DDS::RETCODE_OK;
            }
        }

      return ::DDS::RETCODE_ERROR;
    }

  virtual ::DDS::InstanceHandle_t lookup_instance (
      const MessageType & instance_data)
    {
      ACE_GUARD_RETURN (ACE_Recursive_Thread_Mutex,
                        guard,
                        get_lock (),
                        ::DDS::RETCODE_ERROR);

      typename InstanceMap::const_iterator const it = instance_map_.find(instance_data);

      if (it == instance_map_.end())
        {
          return ::DDS::HANDLE_NIL;
        }
      else
        {
          return it->second;
        }
    }


  /**
   * Initialize the DataWriter object.
   * Called as part of create_datawriter.
   */
  virtual void init (
        ::DDS::Topic_ptr                       topic,
        OpenDDS::DCPS::TopicImpl*              topic_servant,
        const ::DDS::DataWriterQos &           qos,
        ::DDS::DataWriterListener_ptr          a_listener,
        const ::DDS::StatusMask &              mask,
        OpenDDS::DCPS::DomainParticipantImpl*  participant_servant,
        OpenDDS::DCPS::PublisherImpl*          publisher_servant,
        ::DDS::DataWriter_ptr                  dw_objref);

  /**
   * Do parts of enable specific to the datatype.
   * Called by DataWriterImpl::enable().
   */
    virtual ::DDS::ReturnCode_t enable_specific ();

  /**
   * The framework has completed its part of unregistering the
   * given instance.
   */
  virtual void unregistered(::DDS::InstanceHandle_t instance_handle)
  {
    ACE_UNUSED_ARG(instance_handle);
    // Previously this method removed the instance from the instance_map_.
    // The instance handle will not be removed from the
    // map so the instance for re-registration after unregistered
    // will use the old handle.
  }

  /**
   * Accessor to the marshalled data sample allocator.
   */
  ACE_INLINE
  DataAllocator* data_allocator () const  {
    return data_allocator_;
  };

private:

  /**
   * Serialize the instance data.
   *
   * @param instance_data The data to serialize.
   * @param marshaling_type Enumerated type specifying whether to marshal
   *        just the keys or the entire message.
   * @return returns the serialized data.
   */
  ACE_Message_Block* dds_marshal(
    const MessageType& instance_data,
    OpenDDS::DCPS::MarshalingType marshaling_type);

  /**
   * Find the instance handle for the given instance_data using
   * the data type's key(s).  If the instance does not already exist
   * create a new instance handle for it.
   */
  ::DDS::ReturnCode_t get_or_create_instance_handle(
    ::DDS::InstanceHandle_t& handle,
    const MessageType& instance_data,
    const ::DDS::Time_t & source_timestamp)
    {
      ACE_GUARD_RETURN(ACE_Recursive_Thread_Mutex,
                       guard,
                       get_lock(),
                       ::DDS::RETCODE_ERROR);

      handle = ::DDS::HANDLE_NIL;
      typename InstanceMap::const_iterator it = instance_map_.find(instance_data);

      bool needs_creation = true;
      bool needs_registration = true;

      if (it != instance_map_.end())
        {
          needs_creation = false;

          handle = it->second;
          OpenDDS::DCPS::PublicationInstance* instance = get_handle_instance(handle);

          if (instance->unregistered_ == false)
            {
              needs_registration = false;
            }
          // else: The instance is unregistered and now register again.
        }

      if (needs_registration)
        {
          // don't use fast allocator for registration.
          ACE_Message_Block* const marshalled =
            this->dds_marshal(instance_data,
                              OpenDDS::DCPS::KEY_ONLY_MARSHALING);

          // tell DataWriterLocal and Publisher about the instance.
          ::DDS::ReturnCode_t ret = register_instance_i(handle, marshalled, source_timestamp);
          // note: the WriteDataContainer/PublicationInstance maintains ownership
          // of the marshalled sample.

          if (ret != ::DDS::RETCODE_OK)
            {
              marshalled->release ();
              handle = ::DDS::HANDLE_NIL;
              return ret;
            }

          if (needs_creation)
            {
              std::pair<typename InstanceMap::iterator, bool> pair =
                instance_map_.insert(typename InstanceMap::value_type(instance_data, handle));

              if (pair.second == false)
                {
                  handle = ::DDS::HANDLE_NIL;
                  ACE_ERROR_RETURN ((LM_ERROR,
                                     ACE_TEXT("(%P|%t) ")
                                     ACE_TEXT("%sDataWriterImpl::")
                                     ACE_TEXT("get_or_create_instance_handle, ")
                                     ACE_TEXT("insert %s failed. \n"),
                                     Traits::type_name(), Traits::type_name()),
                                    ::DDS::RETCODE_ERROR);
                }
            } // end of if (needs_creation)

          send_all_to_flush_control(guard);

        } // end of if (needs_registration)

      return ::DDS::RETCODE_OK;
    }

    InstanceMap  instance_map_;
    size_t       marshaled_size_;
    size_t       key_marshaled_size_;
    DataAllocator* data_allocator_;
    ::OpenDDS::DCPS::MessageBlockAllocator* mb_allocator_;
    ::OpenDDS::DCPS::DataBlockAllocator*    db_allocator_;

    // A class, normally provided by an unit test, that needs access to
    // private methods/members.
    friend class ::DDS_TEST;
  };

}

#endif /* dds_DCPS_DataWriterImpl_T_h */
