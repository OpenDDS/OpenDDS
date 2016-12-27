// -*- C++ -*-
// ============================================================================
/**
 *  @file   FooDataWriterImpl.h
 *
 *
 *
 */
// ============================================================================



#ifndef FOODATAWRITERIMPL_H_
#define FOODATAWRITERIMPL_H_

#include "FooTypeS.h"
#include "footype_export.h"
#include "dds/DCPS/DataWriterImpl.h"
#include "dds/DCPS/Cached_Allocator_With_Overflow_T.h"
#include "ace/Message_Block.h"
#include "ace/Malloc_T.h"
#include <map>
#ifdef WIN32
#include <hash_map>
#else
#include <ext/hash_map>
#endif

#if !defined (ACE_LACKS_PRAGMA_ONCE)
#pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */


struct FooLess
{
  bool operator()(const Foo& left, const Foo& right) const
  {
    return (left.key < right.key);
  }
};

/**
 * @class FooDataWriterImpl
 *
 * @brief Implementation of the FooDataWriter
 *
 */
class FooType_Export FooDataWriterImpl : public virtual POA_FooDataWriter,
                                         public virtual OpenDDS::DCPS::DataWriterImpl
{
 public:
  typedef std::map<Foo, DDS::InstanceHandle_t, FooLess> FooMap;
  typedef ::OpenDDS::DCPS::Cached_Allocator_With_Overflow<Foo, ACE_Null_Mutex>  FooAllocator;

  FooDataWriterImpl (void);

  virtual ~FooDataWriterImpl (void);

  virtual ::DDS::InstanceHandle_t register_instance (
      const Foo & instance_data
    );

  virtual ::DDS::InstanceHandle_t register_w_timestamp (
      const Foo & instance_data,
      ::DDS::InstanceHandle_t instance_handle,
      const ::DDS::Time_t & source_timestamp
    );

  virtual ::DDS::ReturnCode_t unregister (
      const Foo & instance_data,
      ::DDS::InstanceHandle_t handle
    );

  virtual ::DDS::ReturnCode_t unregister_w_timestamp (
      const Foo & instance_data,
      ::DDS::InstanceHandle_t handle,
      const ::DDS::Time_t & source_timestamp
    );

  virtual ::DDS::ReturnCode_t write (
      const Foo & instance_data,
      ::DDS::InstanceHandle_t handle
    );

  virtual ::DDS::ReturnCode_t write_w_timestamp (
      const Foo & instance_data,
      ::DDS::InstanceHandle_t handle,
      const ::DDS::Time_t & source_timestamp
    );

  virtual ::DDS::ReturnCode_t dispose (
      const Foo & instance_data,
      ::DDS::InstanceHandle_t instance_handle
    );

  virtual ::DDS::ReturnCode_t dispose_w_timestamp (
      const Foo & instance_data,
      ::DDS::InstanceHandle_t instance_handle,
      const ::DDS::Time_t & source_timestamp
    );

  virtual ::DDS::ReturnCode_t get_key_value (
      Foo & key_holder,
      ::DDS::InstanceHandle_t handle
    );

  ACE_Message_Block* marshal(
                const Foo& instance_data,
                int  for_write = 1);

  ::DDS::ReturnCode_t get_or_create_instance_handle(
      DDS::InstanceHandle_t& handle,
      Foo instance_data,
      int& is_new,
      ACE_Message_Block*& marshalled,
      const ::DDS::Time_t & source_timestamp);

  ::DDS::InstanceHandle_t get_instance_handle(
                Foo instance_data);

  virtual void init (
        ::DDS::Topic_ptr                       a_topic,
        const ::DDS::DataWriterQos &           qos,
        ::DDS::DataWriterListener_ptr          a_listener,
        OpenDDS::DCPS::DomainParticipantImpl*      participant,
        OpenDDS::DCPS::PublisherImpl*              publisher,
        ::DDS::Publisher_ptr                   publisher_objref,
        OpenDDS::DCPS::DataWriterRemote_ptr        dw_remote_objref
      );

  virtual void unregistered(::DDS::InstanceHandle_t   instance_handle);

private:
   FooMap  instance_map_;
   FooAllocator* foo_allocator_;
   ::OpenDDS::DCPS::MessageBlockAllocator* mb_allocator_;
   ::OpenDDS::DCPS::DataBlockAllocator*    db_allocator_;
};



#endif /* FOODATAWRITERIMPL_H_  */
