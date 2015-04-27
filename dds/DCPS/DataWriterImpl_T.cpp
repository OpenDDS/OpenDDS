#ifndef dds_DCPS_DataWriterImpl_T_cpp
#define dds_DCPS_DataWriterImpl_T_cpp

namespace OpenDDS {

  template <typename Traits>
  void DataWriterImpl<Traits>::init (
                                     ::DDS::Topic_ptr                       topic,
                                     OpenDDS::DCPS::TopicImpl*              topic_servant,
                                     const ::DDS::DataWriterQos &           qos,
                                     ::DDS::DataWriterListener_ptr          a_listener,
                                     const ::DDS::StatusMask &              mask,
                                     OpenDDS::DCPS::DomainParticipantImpl*  participant_servant,
                                     OpenDDS::DCPS::PublisherImpl*          publisher_servant,
                                     ::DDS::DataWriter_ptr                  dw_objref)
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
    if (OpenDDS::DCPS::gen_is_bounded_size(data)) {
      marshaled_size_ = 8 + OpenDDS::DCPS::gen_max_marshaled_size(data, true);
      // worst case: CDR encapsulation (4 bytes) + Padding for alignment (4 bytes)
    } else {
      marshaled_size_ = 0; // should use gen_find_size when marshaling
    }
    OpenDDS::DCPS::KeyOnly<const MessageType > ko(data);
    if (OpenDDS::DCPS::gen_is_bounded_size(ko)) {
      key_marshaled_size_ = 8 + OpenDDS::DCPS::gen_max_marshaled_size(ko, true);
      // worst case: CDR Encapsulation (4 bytes) + Padding for alignment (4 bytes)
    } else {
      key_marshaled_size_ = 0; // should use gen_find_size when marshaling
    }
  }

  template <typename Traits>
  ::DDS::ReturnCode_t DataWriterImpl<Traits>::enable_specific ()
  {
    MessageType data;
    if (OpenDDS::DCPS::gen_is_bounded_size (data))
      {
        data_allocator_ = new DataAllocator (n_chunks_, marshaled_size_);
        if (::OpenDDS::DCPS::DCPS_debug_level >= 2)
          ACE_DEBUG((LM_DEBUG,
                     ACE_TEXT("(%P|%t) %sDataWriterImpl::")
                     ACE_TEXT("enable_specific-data")
                     ACE_TEXT(" Dynamic_Cached_Allocator_With_Overflow %x ")
                     ACE_TEXT("with %d chunks\n"),
                     Traits::type_name(),
                     data_allocator_,
                     n_chunks_));
      }
    else
      {
        if (::OpenDDS::DCPS::DCPS_debug_level >= 2)
          ACE_DEBUG((LM_DEBUG,
                     ACE_TEXT("(%P|%t) %sDataWriterImpl::enable_specific")
                     ACE_TEXT(" is unbounded data - allocate from heap\n"), Traits::type_name()));
      }

    mb_allocator_ =
      new ::OpenDDS::DCPS::MessageBlockAllocator (
                                                  n_chunks_ * association_chunk_multiplier_);
    db_allocator_ = new ::OpenDDS::DCPS::DataBlockAllocator (n_chunks_);

    if (::OpenDDS::DCPS::DCPS_debug_level >= 2)
      {
        ACE_DEBUG((LM_DEBUG,
                   ACE_TEXT("(%P|%t) %sDataWriterImpl::")
                   ACE_TEXT("enable_specific-mb ")
                   ACE_TEXT("Cached_Allocator_With_Overflow ")
                   ACE_TEXT("%x with %d chunks\n"),
                   Traits::type_name(),
                   mb_allocator_,
                   n_chunks_ * association_chunk_multiplier_));
        ACE_DEBUG((LM_DEBUG,
                   ACE_TEXT("(%P|%t) %sDataWriterImpl::")
                   ACE_TEXT("enable_specific-db ")
                   ACE_TEXT("Cached_Allocator_With_Overflow ")
                   ACE_TEXT("%x with %d chunks\n"),
                   Traits::type_name(),
                   db_allocator_,
                   n_chunks_));
      }

    return ::DDS::RETCODE_OK;
  }

  template <typename Traits>
  ACE_Message_Block* DataWriterImpl<Traits>::dds_marshal(
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
        if (cdr) {
          effective_size = 4; // CDR encapsulation
        }
        OpenDDS::DCPS::gen_find_size(ko_instance_data, effective_size, padding);
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
      serializer << ko_instance_data;

    } else { // OpenDDS::DCPS::FULL_MARSHALING
      size_t effective_size = 0, padding = 0;
      if (marshaled_size_) {
        effective_size = marshaled_size_;
      } else {
        if (cdr) {
          effective_size = 4; // CDR encapsulation
        }
        OpenDDS::DCPS::gen_find_size(instance_data, effective_size, padding);
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
      serializer << instance_data;
    }

    return mb;
  }

}

#endif /* dds_DCPS_DataWriterImpl_T_cpp */
