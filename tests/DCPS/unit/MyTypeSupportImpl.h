// -*- C++ -*-
// ============================================================================
/**
 *  @file   MyTypeSupportImpl.h
 *
 *
 *
 */
// ============================================================================


#ifndef MYTYPESUPPORTIMPL_H_
#define MYTYPESUPPORTIMPL_H_

#include "dds/DCPS/DataReaderImpl.h"
#include "dds/DCPS/DataWriterImpl.h"
#include "dds/DCPS/Definitions.h"
#include "MyTypeSupportC.h"

#if !defined (ACE_LACKS_PRAGMA_ONCE)
#pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

/**
 * @class MyTypeSupportImpl
 *
 * @brief An implementation of a TypeSupport
 *
 */
class MyTypeSupportImpl : public virtual OpenDDS::DCPS::LocalObject<MyTypeSupport>
{

public:
  MyTypeSupportImpl (void);

  virtual ~MyTypeSupportImpl (void);


  virtual ::DDS::ReturnCode_t register_type (
      ::DDS::DomainParticipant_ptr participant,
      const char * type_name
    );

  virtual ::DDS::ReturnCode_t unregister_type(
      ::DDS::DomainParticipant_ptr participant,
      const char * type_name
    );

  virtual
  char * get_type_name (
    );

  virtual ::DDS::DataWriter_ptr create_datawriter (
    );

  virtual ::DDS::DataReader_ptr create_datareader (
    );

#ifndef OPENDDS_NO_MULTI_TOPIC
  virtual ::DDS::DataReader_ptr create_multitopic_datareader();
#endif

#ifndef OPENDDS_NO_CONTENT_SUBSCRIPTION_PROFILE
  virtual const OpenDDS::DCPS::MetaStruct& getMetaStructForType();
#endif

  bool has_dcps_key() { return false; }
};

class MyDataReaderImpl :  public virtual OpenDDS::DCPS::DataReaderImpl
{
public:
  virtual ::DDS::ReturnCode_t enable_specific (
      ) { return ::DDS::RETCODE_OK;};

  virtual ::DDS::ReturnCode_t auto_return_loan (void *)
  {
    return ::DDS::RETCODE_ERROR;
  }

  virtual void purge_data(OpenDDS::DCPS::SubscriptionInstance_rch) {}
  virtual void release_instance_i(DDS::InstanceHandle_t) {}
  virtual void dds_demarshal(const OpenDDS::DCPS::ReceivedDataSample&,
                             OpenDDS::DCPS::SubscriptionInstance_rch &,
                             bool &,
                             bool &,
                             OpenDDS::DCPS::MarshalingType) {}
  virtual void dec_ref_data_element(OpenDDS::DCPS::ReceivedDataElement *) {}
  virtual void delete_instance_map (void *) {}
  bool contains_sample_filtered(DDS::SampleStateMask, DDS::ViewStateMask,
    DDS::InstanceStateMask, const OpenDDS::DCPS::FilterEvaluator&,
    const DDS::StringSeq&) { return true; }
  virtual void lookup_instance(const OpenDDS::DCPS::ReceivedDataSample&,
                               OpenDDS::DCPS::SubscriptionInstance_rch&) {}

#ifndef OPENDDS_NO_CONTENT_SUBSCRIPTION_PROFILE
  DDS::ReturnCode_t read_generic(
    OpenDDS::DCPS::DataReaderImpl::GenericBundle&, DDS::SampleStateMask,
    DDS::ViewStateMask, DDS::InstanceStateMask, bool);

  DDS::InstanceHandle_t lookup_instance_generic(const void* data);

  DDS::ReturnCode_t take(
    OpenDDS::DCPS::AbstractSamples&,
    DDS::SampleStateMask, DDS::ViewStateMask,
    DDS::InstanceStateMask ) { return 0; }

  DDS::ReturnCode_t read_instance_generic(void*& data,
    DDS::SampleInfo& info, DDS::InstanceHandle_t instance,
    DDS::SampleStateMask sample_states, DDS::ViewStateMask view_states,
    DDS::InstanceStateMask instance_states);

  DDS::ReturnCode_t read_next_instance_generic(void*&,
    DDS::SampleInfo&, DDS::InstanceHandle_t, DDS::SampleStateMask,
    DDS::ViewStateMask, DDS::InstanceStateMask);

  void set_instance_state(DDS::InstanceHandle_t, DDS::InstanceStateKind)
  {}
#endif
};

class MyDataWriterImpl :  public virtual OpenDDS::DCPS::DataWriterImpl
{
public:
  virtual ::DDS::ReturnCode_t enable_specific (
      ) {return ::DDS::RETCODE_OK;};

  virtual void unregistered(DDS::InstanceHandle_t /* instance_handle */) {};
};

#endif /* MYTYPESUPPORTIMPL_H_  */
