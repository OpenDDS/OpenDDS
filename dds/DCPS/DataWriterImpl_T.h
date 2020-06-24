#ifndef OPENDDS_DDS_DCPS_DATAWRITERIMPL_T_H
#define OPENDDS_DDS_DCPS_DATAWRITERIMPL_T_H

#include "PublicationInstance.h"
#include "DataWriterImpl.h"
#include "DataReaderImpl.h"
#include "Util.h"
#include "TypeSupportImpl.h"
#include "dcps_export.h"
#include "SafetyProfileStreams.h"
#include "DCPS_Utils.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

/**
 * Servant for DataWriter interface of the Traits::MessageType data type.
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
  typedef KeyOnly<const MessageType> KeyOnlyType;

  typedef OPENDDS_MAP_CMP_T(MessageType, DDS::InstanceHandle_t,
                            typename TraitsType::LessThanType) InstanceMap;
  typedef ::OpenDDS::DCPS::Dynamic_Cached_Allocator_With_Overflow<ACE_Thread_Mutex> DataAllocator;

  /**
   * Used to hold the encoding and get the buffer sizes needed to store the
   * results of the encoding. This is used to isolate the RTPS CDR encapsulated
   * mode from the classical OpenDDS mode.
   *
   * TODO(iguessthislldo): Remove this class if multiple DataWriter encoding
   * modes aren't necessary?
   */
  class EncodingMode {
  public:
    EncodingMode()
    : valid_(false)
    , bounded_(false)
    , buffer_size_(0)
    , key_only_bounded_(false)
    , key_only_buffer_size_(0)
    , header_size_(0)
    {
    }

    EncodingMode(Encoding::Kind kind, bool swap_the_bytes)
    : encoding_(kind, swap_the_bytes)
    , valid_(true)
    , bounded_(MarshalTraitsType::gen_is_bounded_size())
    , buffer_size_(0)
    , key_only_bounded_(MarshalTraitsType::gen_is_bounded_key_size())
    , key_only_buffer_size_(0)
    , header_size_(0)
    {
      if (encoding_.is_encapsulated()) {
        header_size_ = EncapsulationHeader::serialized_size;
      }
      MessageType x;
      if (bounded_) {
        TraitsType::max_serialized_size(encoding_, buffer_size_, x);
        buffer_size_ += header_size_;
      }
      if (key_only_bounded_) {
        const KeyOnlyType key_only_x(x);
        TraitsType::max_serialized_size(
          encoding_, key_only_buffer_size_, key_only_x);
        key_only_buffer_size_ += header_size_;
      }
    }

    bool valid() const
    {
      return valid_;
    }

    const Encoding& encoding() const
    {
      return encoding_;
    }

    /// Size of a buffer needed to store a sample if it's bounded
    size_t buffer_size() const
    {
      return buffer_size_;
    }

    /// Size of a buffer needed to store a specific sample
    size_t buffer_size(const MessageType& x) const
    {
      if (bounded_) {
        return buffer_size_;
      }
      return header_size_ + serialized_size(encoding_, x);
    }

    /// Size of a buffer needed to store a specific key only sample
    size_t buffer_size(const KeyOnlyType& x) const
    {
      if (key_only_bounded_) {
        return key_only_buffer_size_;
      }
      return header_size_ + serialized_size(encoding_, x);
    }

  private:
    Encoding encoding_;
    bool valid_;
    bool bounded_;
    size_t buffer_size_;
    bool key_only_bounded_;
    size_t key_only_buffer_size_;
    size_t header_size_;
  };

  DataWriterImpl_T()
  {
  }

  virtual ~DataWriterImpl_T()
  {
  }

  virtual DDS::InstanceHandle_t register_instance(const MessageType& instance)
  {
    return register_instance_w_timestamp(instance, SystemTimePoint::now().to_dds_time());
  }

  virtual DDS::InstanceHandle_t register_instance_w_timestamp(
    const MessageType& instance, const DDS::Time_t& timestamp)
  {
    DDS::InstanceHandle_t registered_handle = DDS::HANDLE_NIL;

    const DDS::ReturnCode_t ret = get_or_create_instance_handle(registered_handle, instance, timestamp);
    if (ret != DDS::RETCODE_OK) {
      ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: %CDataWriterImpl::register_instance_w_timestamp: ")
        ACE_TEXT("register failed: %C\n"),
        TraitsType::type_name(),
        retcode_to_string(ret)));
    }

    return registered_handle;
  }

  virtual DDS::ReturnCode_t unregister_instance(const MessageType& instance, DDS::InstanceHandle_t handle)
  {
    return unregister_instance_w_timestamp(instance, handle, SystemTimePoint::now().to_dds_time());
  }

  virtual DDS::ReturnCode_t unregister_instance_w_timestamp(
    const MessageType& instance_data,
    DDS::InstanceHandle_t instance_handle,
    const DDS::Time_t& timestamp)
  {
    if (instance_handle == DDS::HANDLE_NIL) {
      instance_handle = this->lookup_instance(instance_data);
      if (instance_handle == DDS::HANDLE_NIL) {
        ACE_ERROR_RETURN((
            LM_ERROR, ACE_TEXT("(%P|%t) ERROR: %CDataWriterImpl::unregister_instance_w_timestamp: ")
            ACE_TEXT("The instance sample is not registered.\n"),
            TraitsType::type_name()),
          DDS::RETCODE_ERROR);
      }
    }

    // DataWriterImpl::unregister_instance_i will call back to inform the
    // DataWriter.
    // That the instance handle is removed from there and hence
    // DataWriter can remove the instance here.
    return OpenDDS::DCPS::DataWriterImpl::unregister_instance_i(instance_handle, timestamp);
  }

  //WARNING: If the handle is non-nil and the instance is not registered
  //         then this operation may cause an access violation.
  //         This lack of safety helps performance.
  virtual DDS::ReturnCode_t
  write(const MessageType& instance_data, DDS::InstanceHandle_t handle)
  {
    return write_w_timestamp(instance_data, handle, SystemTimePoint::now().to_dds_time());
  }

  //WARNING: If the handle is non-nil and the instance is not registered
  //         then this operation may cause an access violation.
  //         This lack of safety helps performance.
  virtual DDS::ReturnCode_t write_w_timestamp(
    const MessageType& instance_data,
    DDS::InstanceHandle_t handle,
    const DDS::Time_t& source_timestamp)
  {
    //  This operation assumes the provided handle is valid. The handle
    //  provided will not be verified.

    if (handle == DDS::HANDLE_NIL) {
      DDS::InstanceHandle_t registered_handle = DDS::HANDLE_NIL;
      const DDS::ReturnCode_t ret =
        this->get_or_create_instance_handle(registered_handle, instance_data, source_timestamp);
      if (ret != DDS::RETCODE_OK) {
        ACE_ERROR_RETURN((
            LM_ERROR, ACE_TEXT("(%P|%t) ERROR: %CDataWriterImpl::write_w_timestamp: ")
            ACE_TEXT("register failed: %C.\n"),
            TraitsType::type_name(),
            retcode_to_string(ret)),
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

    Message_Block_Ptr marshalled(
      dds_marshal(instance_data, OpenDDS::DCPS::FULL_MARSHALING));
    return OpenDDS::DCPS::DataWriterImpl::write(
      move(marshalled), handle, source_timestamp, filter_out._retn());
  }

  virtual DDS::ReturnCode_t
  dispose(const MessageType& instance_data, DDS::InstanceHandle_t instance_handle)
  {
    return dispose_w_timestamp(instance_data, instance_handle, SystemTimePoint::now().to_dds_time());
  }

  virtual DDS::ReturnCode_t dispose_w_timestamp(
    const MessageType& instance_data,
    DDS::InstanceHandle_t instance_handle,
    const DDS::Time_t& source_timestamp)
  {
    if (instance_handle == DDS::HANDLE_NIL) {
      instance_handle = this->lookup_instance(instance_data);
      if (instance_handle == DDS::HANDLE_NIL) {
        ACE_ERROR_RETURN((
            LM_ERROR,
            ACE_TEXT("(%P|%t) ERROR: %CDataWriterImpl::dispose_w_timestamp, ")
            ACE_TEXT("The instance sample is not registered.\n"),
            TraitsType::type_name()),
          DDS::RETCODE_ERROR);
      }
    }

    return OpenDDS::DCPS::DataWriterImpl::dispose(instance_handle, source_timestamp);
  }

  virtual DDS::ReturnCode_t get_key_value(
    MessageType& key_holder,
    DDS::InstanceHandle_t handle)
  {
    ACE_GUARD_RETURN(ACE_Recursive_Thread_Mutex, guard, get_lock(), DDS::RETCODE_ERROR);

    typename InstanceMap::iterator const the_end = instance_map_.end();
    for (typename InstanceMap::iterator it = instance_map_.begin(); it != the_end; ++it) {
      if (it->second == handle) {
        key_holder = it->first;
        return DDS::RETCODE_OK;
      }
    }

    return DDS::RETCODE_BAD_PARAMETER;
  }

  virtual DDS::InstanceHandle_t lookup_instance(
    const MessageType& instance_data)
  {
    ACE_GUARD_RETURN(ACE_Recursive_Thread_Mutex, guard, get_lock(), DDS::RETCODE_ERROR);

    typename InstanceMap::const_iterator const it = instance_map_.find(instance_data);
    if (it == instance_map_.end()) {
      return DDS::HANDLE_NIL;
    } else {
      return it->second;
    }
  }

  /**
   * Do parts of enable specific to the datatype.
   * Called by DataWriterImpl::enable().
   */
  virtual DDS::ReturnCode_t enable_specific()
  {
    mb_allocator_.reset(new ::OpenDDS::DCPS::MessageBlockAllocator(n_chunks_ * association_chunk_multiplier_));
    db_allocator_.reset(new ::OpenDDS::DCPS::DataBlockAllocator(n_chunks_));

    if (::OpenDDS::DCPS::DCPS_debug_level >= 2) {
      ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) %CDataWriterImpl::enable_specific-mb ")
                 ACE_TEXT("Cached_Allocator_With_Overflow ")
                 ACE_TEXT("%x with %d chunks\n"),
                 TraitsType::type_name(),
                 mb_allocator_.get(),
                 n_chunks_ * association_chunk_multiplier_));
      ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) %CDataWriterImpl::enable_specific-db ")
                 ACE_TEXT("Cached_Allocator_With_Overflow ")
                 ACE_TEXT("%x with %d chunks\n"),
                 TraitsType::type_name(),
                 db_allocator_.get(),
                 n_chunks_));
    }

    return DDS::RETCODE_OK;
  }

  /**
   * Accessor to the marshalled data sample allocator.
   */
  ACE_INLINE
  DataAllocator* data_allocator() const
  {
    return data_allocator_.get();
  };

  DDS::ReturnCode_t setup_serialization()
  {
    /**
     * TODO(iguessthislldo): Update when we can determine what encoding we need
     * to use from from a data representation QoS.
     */
    encoding_mode_ = EncodingMode(
      cdr_encapsulation() ?
        Encoding::KIND_XCDR1 : Encoding::KIND_UNALIGNED_CDR,
      swap_bytes());
#if 0
    /*
     * We need to get the encoding kinds for encapsulated and non-encapsulated
     * encoding modes. We do this by picking the first supported representation
     * that is supported by the mode, which is sorta what the XTypes spec says
     * to do, except it only assumes one mode. One mode (but not both) may be
     * left set to KIND_UNKNOWN, disabling it and preventing it from matching
     * with readers just available through that mode.
     */
    Encoding::Kind encap_kind = Encoding::KIND_UNKNOWN;
    Encoding::Kind nonencap_kind = Encoding::KIND_UNKNOWN;
    DDS::DataRepresentationIdSeq repIds =
      get_effective_data_rep_qos(qos_.representation.value);
    for (CORBA::ULong i = 0; i < repIds.length(); ++i) {
      const DDS::DataRepresentationId_t repr = repIds[i];
      const Encoding::Kind kind =
        repr_ext_to_encoding_kind(repr, MarshalTraitsType::extensibility());
      switch (kind) {
      case Encoding::KIND_CDR_UNALIGNED:
        if (nonencap_kind == Encoding::KIND_UNKNOWN) {
          nonencap_kind = kind;
        }
        break;

      case Encoding::KIND_CDR_PLAIN:
      case Encoding::KIND_CDR_PARAMLIST:
      case Encoding::KIND_XCDR2_PLAIN:
      case Encoding::KIND_XCDR2_PARAMLIST:
      case Encoding::KIND_XCDR2_DELIMITED:
        if (encap_kind == Encoding::KIND_UNKNOWN) {
          encap_kind = kind;
        }
        break;

      case Encoding::KIND_XML:
        // Not Supported, Ignore
        break;

      case Encoding::KIND_UNKNOWN:
        if (::OpenDDS::DCPS::DCPS_debug_level) {
          ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: ")
            ACE_TEXT("%CDataWriterImpl::setup_serialization: ")
            ACE_TEXT("Encountered unknown DataRepresentationId_t value: %d\n"),
            TraitsType::type_name(), repr));
        }
        return DDS::RETCODE_ERROR;
      }
      if (encap_kind != Encoding::KIND_UNKNOWN &&
          nonencap_kind != Encoding::KIND_UNKNOWN) {
        break;
      }
    }

    // Try to Setup Encoding Modes
    const bool swap_the_bytes = swap_bytes();
    encapsulated_encoding_mode_ = EncodingMode(encap_kind, swap_the_bytes);
    non_encapsulated_encoding_mode_ = EncodingMode(nonencap_kind, swap_the_bytes);
    if (!encapsulated_encoding_mode_.valid() &&
        !non_encapsulated_encoding_mode_.valid()) {
      if (::OpenDDS::DCPS::DCPS_debug_level) {
        ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: ")
          ACE_TEXT("%CDataWriterImpl::setup_serialization: ")
          ACE_TEXT("None of allowed data representations are supported!\n"),
          TraitsType::type_name()));
      }
      return DDS::RETCODE_ERROR;
    }
#endif

    // Set up allocator with reserved space for data if it is bounded
    if (MarshalTraitsType::gen_is_bounded_size()) {
      const size_t chunk_size = encoding_mode_.buffer_size();
      data_allocator_.reset(new DataAllocator(n_chunks_, chunk_size));
      if (::OpenDDS::DCPS::DCPS_debug_level >= 2) {
        ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) ")
          ACE_TEXT("%CDataWriterImpl::setup_serialization: ")
          ACE_TEXT("using data allocator at %x with %u %u byte chunks\n"),
          TraitsType::type_name(),
          data_allocator_.get(),
          n_chunks_,
          chunk_size));
      }
    } else if (::OpenDDS::DCPS::DCPS_debug_level >= 2) {
      ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) ")
        ACE_TEXT("%CDataWriterImpl::setup_serialization: ")
        ACE_TEXT("sample size is unbounded, not using data allocator, ")
        ACE_TEXT("always allocating from heap\n"),
        TraitsType::type_name()));
    }

    return DDS::RETCODE_OK;
  }

private:

  /**
   * Serialize the instance data.
   *
   * @param instance_data The data to serialize.
   * @param marshaling_type Enumerated type specifying whether to marshal
   *        just the keys or the entire message.
   * @return returns the serialized data.
   */
  ACE_Message_Block* dds_marshal(const MessageType& instance_data,
                                 OpenDDS::DCPS::MarshalingType marshaling_type)
  {
    const bool encapsulated = cdr_encapsulation();
    const Encoding& encoding = encoding_mode_.encoding();
    Message_Block_Ptr mb;
    ACE_Message_Block* tmp_mb;

    if (marshaling_type == OpenDDS::DCPS::KEY_ONLY_MARSHALING) {
      // Don't use the cached allocator for the registered sample message
      // block.
      KeyOnlyType ko_instance_data(instance_data);
      ACE_NEW_RETURN(tmp_mb,
        ACE_Message_Block(
          encoding_mode_.buffer_size(ko_instance_data),
          ACE_Message_Block::MB_DATA,
          0, // cont
          0, // data
          0, // alloc_strategy
          get_db_lock()),
        0);
      mb.reset(tmp_mb);

      OpenDDS::DCPS::Serializer serializer(mb.get(), encoding);
      if (encapsulated) {
        EncapsulationHeader encap;
        if (!encap.from_encoding(encoding, MarshalTraitsType::extensibility())) {
          return 0;
        }
        if (!(serializer << encap)) {
          ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: ")
            ACE_TEXT("%CDataWriterImpl::dds_marshal(): ")
            ACE_TEXT("key only data encapsulation header serialization error.\n"),
            TraitsType::type_name()));
          return 0;
        }
      }
      if (!(serializer << ko_instance_data)) {
        ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: ")
          ACE_TEXT("%CDataWriterImpl::dds_marshal(): ")
          ACE_TEXT("key only data serialization error.\n"),
          TraitsType::type_name()));
        return 0;
      }

    } else { // OpenDDS::DCPS::FULL_MARSHALING
      ACE_NEW_MALLOC_RETURN(tmp_mb,
        static_cast<ACE_Message_Block*>(
          mb_allocator_->malloc(sizeof(ACE_Message_Block))),
        ACE_Message_Block(
          encoding_mode_.buffer_size(instance_data),
          ACE_Message_Block::MB_DATA,
          0, // cont
          0, // data
          data_allocator_.get(), // allocator_strategy
          get_db_lock(), // data block locking_strategy
          ACE_DEFAULT_MESSAGE_BLOCK_PRIORITY,
          ACE_Time_Value::zero,
          ACE_Time_Value::max_time,
          db_allocator_.get(),
          mb_allocator_.get()),
        0);
      mb.reset(tmp_mb);

      OpenDDS::DCPS::Serializer serializer(mb.get(), encoding);
      if (encapsulated) {
        EncapsulationHeader encap;
        if (!encap.from_encoding(encoding, MarshalTraitsType::extensibility())) {
          return 0;
        }
        if (!(serializer << encap)) {
          ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: ")
            ACE_TEXT("%CDataWriterImpl::dds_marshal(): ")
            ACE_TEXT("data encapsulation header serialization error.\n"),
            TraitsType::type_name()));
          return 0;
        }
      }
      if (!(serializer << instance_data)) {
        ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: ")
          ACE_TEXT("%CDataWriterImpl::dds_marshal(): ")
          ACE_TEXT("data serialization error.\n"),
          TraitsType::type_name()));
        return 0;
      }
    }

    return mb.release();
  }

  /**
   * Find the instance handle for the given instance_data using the data type's
   * key(s). If the instance does not already exist create a new instance
   * handle for it.
   */
  DDS::ReturnCode_t get_or_create_instance_handle(
    DDS::InstanceHandle_t& handle,
    const MessageType& instance_data,
    const DDS::Time_t& source_timestamp)
  {
    ACE_GUARD_RETURN(ACE_Recursive_Thread_Mutex, guard, get_lock(), DDS::RETCODE_ERROR);

    handle = DDS::HANDLE_NIL;
    typename InstanceMap::const_iterator it = instance_map_.find(instance_data);

    bool needs_creation = true;
    bool needs_registration = true;

    if (it != instance_map_.end()) {
      needs_creation = false;

      handle = it->second;
      OpenDDS::DCPS::PublicationInstance_rch instance = get_handle_instance(handle);

      if (instance && instance->unregistered_ == false) {
        needs_registration = false;
      }
      // else: The instance is unregistered and now register again.
    }

    if (needs_registration) {
      // don't use fast allocator for registration.
      Message_Block_Ptr marshalled(
        this->dds_marshal(instance_data,
                          OpenDDS::DCPS::KEY_ONLY_MARSHALING));

      // tell DataWriterLocal and Publisher about the instance.
      DDS::ReturnCode_t ret = register_instance_i(handle, move(marshalled), source_timestamp);
      // note: the WriteDataContainer/PublicationInstance maintains ownership
      // of the marshalled sample.

      if (ret != DDS::RETCODE_OK) {
        handle = DDS::HANDLE_NIL;
        return ret;
      }

      if (needs_creation) {
        std::pair<typename InstanceMap::iterator, bool> pair =
          instance_map_.insert(typename InstanceMap::value_type(instance_data, handle));

        if (pair.second == false) {
          handle = DDS::HANDLE_NIL;
          ACE_ERROR_RETURN((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: %CDataWriterImpl::get_or_create_instance_handle ")
                            ACE_TEXT("insert %C failed.\n"),
                            TraitsType::type_name(), TraitsType::type_name()),
                           DDS::RETCODE_ERROR);
        }
      } // end of if (needs_creation)

      send_all_to_flush_control(guard);

    } // end of if (needs_registration)

    return DDS::RETCODE_OK;
  }

  InstanceMap instance_map_;
  unique_ptr<DataAllocator> data_allocator_;
  unique_ptr<MessageBlockAllocator> mb_allocator_;
  unique_ptr<DataBlockAllocator> db_allocator_;
  EncodingMode encoding_mode_;

  // A class, normally provided by an unit test, that needs access to
  // private methods/members.
  friend class ::DDS_TEST;
};

} // namespace DCPS
} // namepsace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif /* OPENDDS_DDS_DCPS_DATAWRITERIMPL_T_H */
