// -*- C++ -*-
//


#include "FooDataWriterImpl.h"
#include "dds/DCPS/Registered_Data_Types.h"
#include "dds/DdsDcpsDomainC.h"
#include "dds/DCPS/Service_Participant.h"
#include "dds/DCPS/Qos_Helper.h"

FooDataWriterImpl::FooDataWriterImpl (void)
: foo_allocator_ (0),
  mb_allocator_ (0),
  db_allocator_ (0)
{
}

FooDataWriterImpl::~FooDataWriterImpl (void)
{
  delete foo_allocator_;
  delete mb_allocator_;
  delete db_allocator_;
}


::DDS::InstanceHandle_t FooDataWriterImpl::register_instance(
    const Foo & instance_data
    )
{
  ::DDS::Time_t source_timestamp
    = ::OpenDDS::DCPS::time_value_to_time (ACE_OS::gettimeofday ());
  return register_w_timestamp (instance_data,
                               ::OpenDDS::DCPS::HANDLE_NIL,
                               source_timestamp);
}

::DDS::InstanceHandle_t FooDataWriterImpl::register_w_timestamp (
    const Foo & instance_data,
    ::DDS::InstanceHandle_t ,
    const ::DDS::Time_t & source_timestamp
  )
{
  ACE_GUARD_RETURN (ACE_Recursive_Thread_Mutex,
                    guard,
                    get_lock (),
                    ::DDS::RETCODE_ERROR);
  ACE_Message_Block* marshalled;
  int is_new = 0;
  ::DDS::InstanceHandle_t handle;
  ::DDS::ReturnCode_t ret
    = this->get_or_create_instance_handle(handle,
                                          instance_data,
                                          is_new,
                                          marshalled,
                                          source_timestamp);
  if (ret != ::DDS::RETCODE_OK)
  {
      ACE_ERROR ((LM_ERROR,
                  ACE_TEXT("(%P|%t) ")
                  ACE_TEXT("FooDataWriterImpl::register_w_timestamp, ")
                  ACE_TEXT("register failed error=%d.\n"),
                  ret));
  }

  return handle;
}

::DDS::ReturnCode_t FooDataWriterImpl::unregister (
    const Foo & instance_data,
    ::DDS::InstanceHandle_t handle
  )
{
  ::DDS::Time_t source_timestamp
    = ::OpenDDS::DCPS::time_value_to_time (ACE_OS::gettimeofday ());
  return unregister_w_timestamp (instance_data,
                                 handle,
                                 source_timestamp);
}

::DDS::ReturnCode_t FooDataWriterImpl::unregister_w_timestamp (
    const Foo & instance_data,
    ::DDS::InstanceHandle_t handle,
    const ::DDS::Time_t & source_timestamp
  )
{
  ACE_GUARD_RETURN (ACE_Recursive_Thread_Mutex,
                    guard,
                    get_lock (),
                    ::DDS::RETCODE_ERROR);
  ::DDS::InstanceHandle_t registered_handle
      = this->get_instance_handle(instance_data);

  if(registered_handle == ::OpenDDS::DCPS::HANDLE_NIL)
  {
    // This case could be the instance is not registered yet or
    // already unregistered.
    ACE_ERROR_RETURN ((LM_ERROR,
                        ACE_TEXT("(%P|%t) ")
                        ACE_TEXT("FooDataWriterImpl::unregister, ")
                        ACE_TEXT("The instance is not registered.\n")),
                        ::DDS::RETCODE_ERROR);
  }
  else if (handle != ::OpenDDS::DCPS::HANDLE_NIL && handle != registered_handle)
  {
    ACE_ERROR_RETURN ((LM_ERROR,
                        ACE_TEXT("(%P|%t) ")
                        ACE_TEXT("FooDataWriterImpl::unregister, ")
                        ACE_TEXT("The given handle=%X is different from "
                        "registered handle=%X.\n"),
                        handle, registered_handle),
                        ::DDS::RETCODE_ERROR);
  }
  else
  {
    // DataWriterImpl::unregister will call back to inform the FooDataWriter
    // that the instance handle is removed from there and hence FooDataWriter
    // can remove the instance here.
    return DataWriterImpl::unregister(handle, source_timestamp);
  }
}

::DDS::ReturnCode_t FooDataWriterImpl::write (
    const Foo & instance_data,
    ::DDS::InstanceHandle_t handle
  )
{
  ::DDS::Time_t source_timestamp
    = ::OpenDDS::DCPS::time_value_to_time (ACE_OS::gettimeofday ());
  return write_w_timestamp (instance_data,
                            handle,
                            source_timestamp);
}

::DDS::ReturnCode_t FooDataWriterImpl::write_w_timestamp (
    const Foo & instance_data,
    ::DDS::InstanceHandle_t handle,
    const ::DDS::Time_t & source_timestamp
  )
{
  //  A lock is obtained on entering this method to serialize access to
  //  the contained data storage and interfaces.  This lock protects the
  //  marshaled data buffers as well as the instance data containers.

  ACE_GUARD_RETURN (ACE_Recursive_Thread_Mutex,
                    guard,
                    get_lock (),
                    ::DDS::RETCODE_ERROR);

  ACE_Message_Block* marshalled;
  int is_new = 0;
  ::DDS::InstanceHandle_t registered_handle;
  ::DDS::ReturnCode_t ret
    = this->get_or_create_instance_handle(registered_handle,
                                          instance_data,
                                          is_new,
                                          marshalled,
                                          source_timestamp);
  if (ret != ::DDS::RETCODE_OK)
  {
    ACE_ERROR ((LM_ERROR,
                ACE_TEXT("(%P|%t) ")
                ACE_TEXT("FooDataWriterImpl::write, ")
                ACE_TEXT("register failed err=%d.\n"),
                ret));
  }

  if (handle == ::OpenDDS::DCPS::HANDLE_NIL)
  {
    // note: do not tell subscriber if there is an implicit registration.
    //    Subscriber must be able to handle a new instance without being
    //    told about it.

    handle = registered_handle;
  }

  if (handle == ::OpenDDS::DCPS::HANDLE_NIL)
  {
    ACE_ERROR_RETURN ((LM_ERROR,
                       ACE_TEXT("(%P|%t) ")
                       ACE_TEXT("FooDataWriterImpl::write, ")
                       ACE_TEXT("The instance has not registered yet.")),
                       ::DDS::RETCODE_ERROR);
  }

  if (handle != registered_handle)
  {
    ACE_ERROR_RETURN ((LM_ERROR,
                       ACE_TEXT("(%P|%t) ")
                       ACE_TEXT("FooDataWriterImpl::write, ")
                       ACE_TEXT("The given handle=%X is different from "
                       "registered handle=%X.\n"),
                       handle, registered_handle),
                       ::DDS::RETCODE_ERROR);
  }

  marshalled = marshal (instance_data); // FOR_WRITE
  return DataWriterImpl::write(marshalled, handle, source_timestamp);
}

::DDS::ReturnCode_t FooDataWriterImpl::dispose (
    const Foo & instance_data,
    ::DDS::InstanceHandle_t handle
  )
{
  ::DDS::Time_t source_timestamp
    = ::OpenDDS::DCPS::time_value_to_time (ACE_OS::gettimeofday ());
  return dispose_w_timestamp (instance_data,
                              handle,
                              source_timestamp);
}

::DDS::ReturnCode_t FooDataWriterImpl::dispose_w_timestamp (
    const Foo & instance_data,
    ::DDS::InstanceHandle_t handle,
    const ::DDS::Time_t & source_timestamp
  )
{
  ACE_GUARD_RETURN (ACE_Recursive_Thread_Mutex,
                    guard,
                    get_lock (),
                    ::DDS::RETCODE_ERROR);

  ACE_Message_Block* marshalled;
  ::DDS::InstanceHandle_t registered_handle
      = this->get_instance_handle(instance_data);
  if(handle == ::OpenDDS::DCPS::HANDLE_NIL)
  {
    if (registered_handle == ::OpenDDS::DCPS::HANDLE_NIL)
    {
      ACE_ERROR_RETURN ((LM_ERROR,
                          ACE_TEXT("(%P|%t) ")
                          ACE_TEXT("FooDataWriterImpl::dispose, ")
                          ACE_TEXT("The handle is not registered yet.\n")),
                          ::DDS::RETCODE_ERROR);
    }
    else
    {
      handle = registered_handle;
    }
  }
  else if(handle != registered_handle)
  {
    ACE_ERROR_RETURN ((LM_ERROR,
                        ACE_TEXT("(%P|%t) ")
                        ACE_TEXT("FooDataWriterImpl::dispose, ")
                        ACE_TEXT("The given handle=%X is different from "
                        "registered handle=%X.\n"),
                        handle, registered_handle),
                        ::DDS::RETCODE_ERROR);
  }

  marshalled = this->marshal (instance_data, 0);  // NOT_FOR_WRITE

  return DataWriterImpl::dispose(handle, source_timestamp);
}

::DDS::ReturnCode_t FooDataWriterImpl::get_key_value (
    Foo & key_holder,
    ::DDS::InstanceHandle_t handle
  )
  {
    ACE_GUARD_RETURN (ACE_Recursive_Thread_Mutex,
                      guard,
                      get_lock (),
                      ::DDS::RETCODE_ERROR);
    FooMap::iterator it;
    for (it = instance_map_.begin ();
         it != instance_map_.end ();
         it ++)
    {
      if (it->second == handle)
      {
        key_holder = it->first;
        return ::DDS::RETCODE_OK;
      }
    }

    return ::DDS::RETCODE_ERROR;
  }


// Note: The FooDataWriter gives ownership of the marshalled data
//       to the WriteDataContainer.
ACE_Message_Block*
 FooDataWriterImpl::marshal(
                const Foo& instance_data,
                int  for_write)
{
  ACE_Message_Block* mb;
  if (for_write)
  {
    ACE_NEW_MALLOC_RETURN (mb,
                           static_cast<ACE_Message_Block*> (
                             mb_allocator_->malloc (
                             sizeof (ACE_Message_Block))),
                           ACE_Message_Block( sizeof (Foo),
                                              ACE_Message_Block::MB_DATA,
                                              0, //cont
                                              0, //data
                                              foo_allocator_, //allocator_strategy
                                              0, //locking_strategy
                                              ACE_DEFAULT_MESSAGE_BLOCK_PRIORITY,
                                              ACE_Time_Value::zero,
                                              ACE_Time_Value::max_time,
                                              db_allocator_,
                                              mb_allocator_),
                            0);
    mb->copy ((const char *)&instance_data, sizeof (Foo));
  }
  else
  { // Don't use the cached allocator for the registered sample message
    // block.
    Foo* register_sample = new Foo();
    *register_sample = instance_data;

    ACE_NEW_RETURN (mb,
                    ACE_Message_Block ((const char*)register_sample,
                                        sizeof (Foo)),
                    0);
    // Let the PublicationInstance destructor release the Message Block
    // and delete this register_sample.
    mb->clr_flags(ACE_Message_Block::DONT_DELETE);
  }

  return mb;
}

::DDS::ReturnCode_t
 FooDataWriterImpl::get_or_create_instance_handle(
                DDS::InstanceHandle_t& handle,
                Foo instance_data,
                int& is_new,
                ACE_Message_Block*& marshalled, // only if is_new==1
                const ::DDS::Time_t & source_timestamp)
{
  handle = ::OpenDDS::DCPS::HANDLE_NIL;
  FooMap::const_iterator it = instance_map_.find(instance_data);

  if (it == instance_map_.end())
  {
    is_new = 1;
    // don't use fast allocator for registration.
    marshalled = this->marshal(instance_data, 0); //NOT_FOR_WRITE

    // tell DataWriterLocal and Publisher about the instance.
    ::DDS::ReturnCode_t ret
      = register_instance(handle, marshalled, source_timestamp);
    if (ret == ::DDS::RETCODE_OK)
    {
      std::pair<FooMap::iterator, bool> pair
          = instance_map_.insert(FooMap::value_type(instance_data, handle));
      if (pair.second == false)
        {
          ACE_ERROR_RETURN ((LM_ERROR,
                              ACE_TEXT("(%P|%t) "
                              "FooDataWriterImpl::get_or_create_instance_handle, ")
                              ACE_TEXT("insert Foo(key=%d) failed. \n"),
                              instance_data.key),
                              ::DDS::RETCODE_ERROR);
        }
    }
    else
    {
      marshalled->release ();
      return ret;
    }
  }
  else {
    is_new = 0;
    handle = it->second;
  }
  return ::DDS::RETCODE_OK;
}

::DDS::InstanceHandle_t
 FooDataWriterImpl::get_instance_handle(
                Foo instance_data)
{
  FooMap::const_iterator it = instance_map_.find(instance_data);

  if (it == instance_map_.end())
  {
    return ::OpenDDS::DCPS::HANDLE_NIL;
  }
  else
  {
    return it->second;
  }
}

void FooDataWriterImpl::init (
      ::DDS::Topic_ptr                       a_topic,
      const ::DDS::DataWriterQos &           qos,
      ::DDS::DataWriterListener_ptr          a_listener,
      OpenDDS::DCPS::DomainParticipantImpl*      participant,
      OpenDDS::DCPS::PublisherImpl*              publisher,
      ::DDS::Publisher_ptr                   publisher_objref,
      OpenDDS::DCPS::DataWriterRemote_ptr        dw_remote_objref
    )
{
  DataWriterImpl::init (a_topic,
                        qos,
                        a_listener,
                        participant,
                        publisher,
                        publisher_objref,
                        dw_remote_objref);

  if (qos.resource_limits.max_samples == ::DDS::LENGTH_UNLIMITED)
  {
    foo_allocator_ = new FooAllocator (n_chunks_);
    mb_allocator_ = new ::OpenDDS::DCPS::MessageBlockAllocator (n_chunks_);
    db_allocator_ = new ::OpenDDS::DCPS::DataBlockAllocator (n_chunks_);
  }
}


void FooDataWriterImpl::unregistered(::DDS::InstanceHandle_t   instance_handle)
{
  for (FooMap::iterator it = instance_map_.begin ();
    it != instance_map_.end ();
    it ++)
  {
    if (it->second == instance_handle)
    {
      instance_map_.erase (it);
      break;
    }
  }
}
