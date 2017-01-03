#ifndef dds_DCPS_DataWriterImpl_T_h
#define dds_DCPS_DataWriterImpl_T_h

#include "dds/DCPS/PublicationInstance.h"
#include "dds/DCPS/DataWriterImpl.h"
#include "dds/DCPS/DataReaderImpl.h"
#include "dds/DCPS/Util.h"
#include "dds/DCPS/TypeSupportImpl.h"
#include "dcps_export.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
  namespace DCPS {

/** Servant for DataWriter interface of the Traits::MessageType data type.
 *
 * See the DDS specification, OMG formal/04-12-02, for a description of
 * this interface.
 */
  template <typename MessageType>
  class
#if ( __GNUC__ == 4 && __GNUC_MINOR__ == 1)
    OpenDDS_Dcps_Export
#endif
    DataWriterImpl_T
    : public virtual OpenDDS::DCPS::LocalObject<typename DDSTraits<MessageType>::DataWriterType>,
      public virtual OpenDDS::DCPS::DataWriterImpl
  {
  public:
    typedef DDSTraits<MessageType> TraitsType;
    typedef MarshalTraits<MessageType> MarshalTraitsType;

    typedef OPENDDS_MAP_CMP(MessageType, DDS::InstanceHandle_t,
                            typename TraitsType::LessThanType) InstanceMap;
    typedef ::OpenDDS::DCPS::Dynamic_Cached_Allocator_With_Overflow<ACE_Thread_Mutex>  DataAllocator;

    enum {
      cdr_header_size = 4
    };

    DataWriterImpl_T (void)
      : marshaled_size_ (0)
      , key_marshaled_size_ (0)
      , data_allocator_ (0)
      , mb_allocator_ (0)
      , db_allocator_ (0)
    {
    }

    virtual ~DataWriterImpl_T (void)
    {
      delete data_allocator_;
      delete mb_allocator_;
      delete db_allocator_;
    }

  virtual DDS::InstanceHandle_t register_instance (
      const MessageType & instance)
    {
      DDS::Time_t const timestamp =
        ::OpenDDS::DCPS::time_value_to_time (ACE_OS::gettimeofday ());
      return register_instance_w_timestamp (instance, timestamp);
    }

  virtual DDS::InstanceHandle_t register_instance_w_timestamp (
      const MessageType & instance,
      const DDS::Time_t & timestamp)
    {
      DDS::InstanceHandle_t registered_handle = DDS::HANDLE_NIL;

      DDS::ReturnCode_t const ret
          = this->get_or_create_instance_handle(registered_handle,
                                                instance,
                                                timestamp);
      if (ret != DDS::RETCODE_OK)
        {
          ACE_ERROR ((LM_ERROR,
                      ACE_TEXT("(%P|%t) ")
                      ACE_TEXT("%CDataWriterImpl::")
                      ACE_TEXT("register_instance_w_timestamp, ")
                      ACE_TEXT("register failed error=%d.\n"),
                      TraitsType::type_name(),
                      ret));
        }

      return registered_handle;
    }

  virtual DDS::ReturnCode_t unregister_instance (
      const MessageType & instance,
      DDS::InstanceHandle_t handle)
    {
      DDS::Time_t const timestamp =
        ::OpenDDS::DCPS::time_value_to_time (ACE_OS::gettimeofday ());

      return unregister_instance_w_timestamp (instance,
                                              handle,
                                              timestamp);
    }

  virtual DDS::ReturnCode_t unregister_instance_w_timestamp (
      const MessageType & instance,
      DDS::InstanceHandle_t handle,
      const DDS::Time_t & timestamp)
    {
      DDS::InstanceHandle_t const registered_handle =
        this->lookup_instance(instance);

      if (registered_handle == DDS::HANDLE_NIL)
        {
          // This case could be the instance is not registered yet or
          // already unregistered.
          ACE_ERROR_RETURN ((LM_ERROR,
                             ACE_TEXT("(%P|%t) ")
                             ACE_TEXT("%CDataWriterImpl::")
                             ACE_TEXT("unregister_instance_w_timestamp, ")
                             ACE_TEXT("The instance is not registered.\n"),
                             TraitsType::type_name()),
                            DDS::RETCODE_ERROR);
        }
      else if (handle != DDS::HANDLE_NIL && handle != registered_handle)
        {
          ACE_ERROR_RETURN ((LM_ERROR,
                             ACE_TEXT("(%P|%t) ")
                             ACE_TEXT("%CDataWriterImpl::")
                             ACE_TEXT("unregister_w_timestamp, ")
                             ACE_TEXT("The given handle=%X is different from ")
                             ACE_TEXT("registered handle=%X.\n"),
                             TraitsType::type_name(),
                             handle, registered_handle),
                            DDS::RETCODE_ERROR);
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
  virtual DDS::ReturnCode_t write (
      const MessageType & instance_data,
      DDS::InstanceHandle_t handle)
    {
      DDS::Time_t const source_timestamp =
        ::OpenDDS::DCPS::time_value_to_time (ACE_OS::gettimeofday ());
      return write_w_timestamp (instance_data,
                                handle,
                                source_timestamp);
    }


  //WARNING: If the handle is non-nil and the instance is not registered
  //         then this operation may cause an access violation.
  //         This lack of safety helps performance.
  virtual DDS::ReturnCode_t write_w_timestamp (
      const MessageType & instance_data,
      DDS::InstanceHandle_t handle,
      const DDS::Time_t & source_timestamp)
    {
      //  This operation assumes the provided handle is valid. The handle
      //  provided will not be verified.

      if (handle == DDS::HANDLE_NIL) {
        DDS::InstanceHandle_t registered_handle = DDS::HANDLE_NIL;
        DDS::ReturnCode_t ret
            = this->get_or_create_instance_handle(registered_handle,
                                                  instance_data,
                                                  source_timestamp);
        if (ret != DDS::RETCODE_OK) {
          ACE_ERROR_RETURN((LM_ERROR,
                            ACE_TEXT("(%P|%t) ")
                            ACE_TEXT("%CDataWriterImpl::write, ")
                            ACE_TEXT("register failed err=%d.\n"),
                            TraitsType::type_name(),
                            ret),
                           ret);
        }

        handle = registered_handle;
      }

      // list of reader RepoIds that should not get data
      OpenDDS::DCPS::GUIDSeq_var filter_out;
#ifndef OPENDDS_NO_CONTENT_FILTERED_TOPIC
      if (TheServiceParticipant->publisher_content_filter()) {
        ACE_GUARD_RETURN(ACE_Thread_Mutex, reader_info_guard, this->reader_info_lock_, DDS::RETCODE_ERROR);
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

  virtual DDS::ReturnCode_t dispose (
      const MessageType & instance_data,
      DDS::InstanceHandle_t instance_handle)
    {
      DDS::Time_t const source_timestamp =
        ::OpenDDS::DCPS::time_value_to_time (ACE_OS::gettimeofday ());
      return dispose_w_timestamp (instance_data,
                                  instance_handle,
                                  source_timestamp);
    }

  virtual DDS::ReturnCode_t dispose_w_timestamp (
      const MessageType & instance_data,
      DDS::InstanceHandle_t instance_handle,
      const DDS::Time_t & source_timestamp)
    {
      if(instance_handle == DDS::HANDLE_NIL)
        {
          instance_handle = this->lookup_instance(instance_data);
          if (instance_handle == DDS::HANDLE_NIL)
            {
              ACE_ERROR_RETURN ((LM_ERROR,
                                 ACE_TEXT("(%P|%t) ")
                                 ACE_TEXT("%CDataWriterImpl::dispose, ")
                                 ACE_TEXT("The instance sample is not registered.\n"),
                                 TraitsType::type_name()),
                                DDS::RETCODE_ERROR);
            }
        }

      return OpenDDS::DCPS::DataWriterImpl::dispose(instance_handle,
                                                    source_timestamp);
    }

  virtual DDS::ReturnCode_t get_key_value (
      MessageType & key_holder,
      DDS::InstanceHandle_t handle)
    {
      ACE_GUARD_RETURN (ACE_Recursive_Thread_Mutex,
                        guard,
                        get_lock (),
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

      return DDS::RETCODE_ERROR;
    }

  virtual DDS::InstanceHandle_t lookup_instance (
      const MessageType & instance_data)
    {
      ACE_GUARD_RETURN (ACE_Recursive_Thread_Mutex,
                        guard,
                        get_lock (),
                        DDS::RETCODE_ERROR);

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


  /**
   * Initialize the DataWriter object.
   * Called as part of create_datawriter.
   */
    virtual void init (DDS::Topic_ptr                       topic,
                       OpenDDS::DCPS::TopicImpl*              topic_servant,
                       const DDS::DataWriterQos &           qos,
                       DDS::DataWriterListener_ptr          a_listener,
                       const DDS::StatusMask &              mask,
                       OpenDDS::DCPS::DomainParticipantImpl*  participant_servant,
                       OpenDDS::DCPS::PublisherImpl*          publisher_servant,
                       DDS::DataWriter_ptr                  dw_objref)
    {
      OpenDDS::DCPS::DataWriterImpl::init (topic,
                                           topic_servant,
                                           qos,
                                           a_listener,
                                           mask,
                                           participant_servant,
                                           publisher_servant,
                                           dw_objref);

      MessageType data;
      if (MarshalTraitsType::gen_is_bounded_size()) {
        marshaled_size_ = 8 + TraitsType::gen_max_marshaled_size(data, true);
        // worst case: CDR encapsulation (4 bytes) + Padding for alignment (4 bytes)
      } else {
        marshaled_size_ = 0; // should use gen_find_size when marshaling
      }
      if (MarshalTraitsType::gen_is_bounded_key_size()) {
        OpenDDS::DCPS::KeyOnly<const MessageType > ko(data);
        key_marshaled_size_ = 8 + TraitsType::gen_max_marshaled_size(ko, true);
        // worst case: CDR Encapsulation (4 bytes) + Padding for alignment (4 bytes)
      } else {
        key_marshaled_size_ = 0; // should use gen_find_size when marshaling
      }
    }

  /**
   * Do parts of enable specific to the datatype.
   * Called by DataWriterImpl::enable().
   */
    virtual DDS::ReturnCode_t enable_specific ()
    {
      if (MarshalTraitsType::gen_is_bounded_size ())
        {
          data_allocator_ = new DataAllocator (n_chunks_, marshaled_size_);
          if (::OpenDDS::DCPS::DCPS_debug_level >= 2)
            ACE_DEBUG((LM_DEBUG,
                       ACE_TEXT("(%P|%t) %CDataWriterImpl::")
                       ACE_TEXT("enable_specific-data")
                       ACE_TEXT(" Dynamic_Cached_Allocator_With_Overflow %x ")
                       ACE_TEXT("with %d chunks\n"),
                       TraitsType::type_name(),
                       data_allocator_,
                       n_chunks_));
        }
      else
        {
          if (::OpenDDS::DCPS::DCPS_debug_level >= 2)
            ACE_DEBUG((LM_DEBUG,
                       ACE_TEXT("(%P|%t) %CDataWriterImpl::enable_specific")
                       ACE_TEXT(" is unbounded data - allocate from heap\n"), TraitsType::type_name()));
        }

      mb_allocator_ =
        new ::OpenDDS::DCPS::MessageBlockAllocator (
                                                    n_chunks_ * association_chunk_multiplier_);
      db_allocator_ = new ::OpenDDS::DCPS::DataBlockAllocator (n_chunks_);

      if (::OpenDDS::DCPS::DCPS_debug_level >= 2)
        {
          ACE_DEBUG((LM_DEBUG,
                     ACE_TEXT("(%P|%t) %CDataWriterImpl::")
                     ACE_TEXT("enable_specific-mb ")
                     ACE_TEXT("Cached_Allocator_With_Overflow ")
                     ACE_TEXT("%x with %d chunks\n"),
                     TraitsType::type_name(),
                     mb_allocator_,
                     n_chunks_ * association_chunk_multiplier_));
          ACE_DEBUG((LM_DEBUG,
                     ACE_TEXT("(%P|%t) %CDataWriterImpl::")
                     ACE_TEXT("enable_specific-db ")
                     ACE_TEXT("Cached_Allocator_With_Overflow ")
                     ACE_TEXT("%x with %d chunks\n"),
                     TraitsType::type_name(),
                     db_allocator_,
                     n_chunks_));
        }

      return DDS::RETCODE_OK;
    }

  /**
   * The framework has completed its part of unregistering the
   * given instance.
   */
  virtual void unregistered(DDS::InstanceHandle_t instance_handle)
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
                                   OpenDDS::DCPS::MarshalingType marshaling_type)
    {
      const bool cdr = this->cdr_encapsulation(), swap = this->swap_bytes();

      ACE_Message_Block* mb;
      if (marshaling_type == OpenDDS::DCPS::KEY_ONLY_MARSHALING) {
        // Don't use the cached allocator for the registered sample message
        // block.

        OpenDDS::DCPS::KeyOnly<const MessageType > ko_instance_data(instance_data);
        size_t effective_size = 0, padding = 0;
        if (key_marshaled_size_) {
          effective_size = key_marshaled_size_;
        } else {
          if (cdr && !Serializer::use_rti_serialization()) {
            effective_size = cdr_header_size; // CDR encapsulation
          }
          TraitsType::gen_find_size(ko_instance_data, effective_size, padding);
          if (cdr && Serializer::use_rti_serialization()) {
            effective_size += (cdr_header_size);
          }
        }
        if (cdr) {
          effective_size += padding;
        }
        ACE_NEW_RETURN(mb, ACE_Message_Block(effective_size,
                                             ACE_Message_Block::MB_DATA,
                                             0, //cont
                                             0, //data
                                             0, //alloc_strategy
                                             get_db_lock()), 0);
        OpenDDS::DCPS::Serializer serializer(mb, swap, cdr
                                             ? OpenDDS::DCPS::Serializer::ALIGN_CDR
                                             : OpenDDS::DCPS::Serializer::ALIGN_NONE);
        if (cdr) {
          serializer << ACE_OutputCDR::from_octet(0);
          serializer << ACE_OutputCDR::from_octet(swap ? !ACE_CDR_BYTE_ORDER : ACE_CDR_BYTE_ORDER);
          serializer << ACE_CDR::UShort(0);
        }
        // If this is RTI serialization, start counting byte offset AFTER
        // the header
        if (cdr && Serializer::use_rti_serialization()) {
          // Start counting byte-offset AFTER header
          serializer.reset_alignment();
        }
        serializer << ko_instance_data;
      } else { // OpenDDS::DCPS::FULL_MARSHALING
        size_t effective_size = 0, padding = 0;
        if (marshaled_size_) {
          effective_size = marshaled_size_;
        } else {
          if (cdr && !Serializer::use_rti_serialization()) {
            effective_size = cdr_header_size; // CDR encapsulation
          }
          TraitsType::gen_find_size(instance_data, effective_size, padding);
          if (cdr && Serializer::use_rti_serialization()) {
            effective_size += (cdr_header_size);
          }
        }
        if (cdr) {
          effective_size += padding;
        }
        ACE_NEW_MALLOC_RETURN(mb,
                              static_cast<ACE_Message_Block*>(
                                                              mb_allocator_->malloc(
                                                                                    sizeof(ACE_Message_Block))),
                              ACE_Message_Block(
                                                effective_size,
                                                ACE_Message_Block::MB_DATA,
                                                0, //cont
                                                0, //data
                                                data_allocator_, //allocator_strategy
                                                get_db_lock(), //data block locking_strategy
                                                ACE_DEFAULT_MESSAGE_BLOCK_PRIORITY,
                                                ACE_Time_Value::zero,
                                                ACE_Time_Value::max_time,
                                                db_allocator_,
                                                mb_allocator_),
                              0);
        OpenDDS::DCPS::Serializer serializer(mb, swap, cdr
                                             ? OpenDDS::DCPS::Serializer::ALIGN_CDR
                                             : OpenDDS::DCPS::Serializer::ALIGN_NONE);
        if (cdr) {
          serializer << ACE_OutputCDR::from_octet(0);
          serializer << ACE_OutputCDR::from_octet(swap ? !ACE_CDR_BYTE_ORDER : ACE_CDR_BYTE_ORDER);
          serializer << ACE_CDR::UShort(0);
        }
        // If this is RTI serialization, start counting byte offset AFTER
        // the header
        if (cdr && Serializer::use_rti_serialization()) {
          // Start counting byte-offset AFTER header
          serializer.reset_alignment();
        }
        serializer << instance_data;
      }

      return mb;
    }

  /**
   * Find the instance handle for the given instance_data using
   * the data type's key(s).  If the instance does not already exist
   * create a new instance handle for it.
   */
  DDS::ReturnCode_t get_or_create_instance_handle(
    DDS::InstanceHandle_t& handle,
    const MessageType& instance_data,
    const DDS::Time_t & source_timestamp)
    {
      ACE_GUARD_RETURN(ACE_Recursive_Thread_Mutex,
                       guard,
                       get_lock(),
                       DDS::RETCODE_ERROR);

      handle = DDS::HANDLE_NIL;
      typename InstanceMap::const_iterator it = instance_map_.find(instance_data);

      bool needs_creation = true;
      bool needs_registration = true;

      if (it != instance_map_.end())
        {
          needs_creation = false;

          handle = it->second;
          OpenDDS::DCPS::PublicationInstance_rch instance = get_handle_instance(handle);

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
          DDS::ReturnCode_t ret = register_instance_i(handle, marshalled, source_timestamp);
          // note: the WriteDataContainer/PublicationInstance maintains ownership
          // of the marshalled sample.

          if (ret != DDS::RETCODE_OK)
            {
              marshalled->release ();
              handle = DDS::HANDLE_NIL;
              return ret;
            }

          if (needs_creation)
            {
              std::pair<typename InstanceMap::iterator, bool> pair =
                instance_map_.insert(typename InstanceMap::value_type(instance_data, handle));

              if (pair.second == false)
                {
                  handle = DDS::HANDLE_NIL;
                  ACE_ERROR_RETURN ((LM_ERROR,
                                     ACE_TEXT("(%P|%t) ")
                                     ACE_TEXT("%CDataWriterImpl::")
                                     ACE_TEXT("get_or_create_instance_handle, ")
                                     ACE_TEXT("insert %s failed. \n"),
                                     TraitsType::type_name(), TraitsType::type_name()),
                                    DDS::RETCODE_ERROR);
                }
            } // end of if (needs_creation)

          send_all_to_flush_control(guard);

        } // end of if (needs_registration)

      return DDS::RETCODE_OK;
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
}

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif /* dds_DCPS_DataWriterImpl_T_h */
