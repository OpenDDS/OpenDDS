// -*- C++ -*-
//
// $Id$
#ifndef DATAREADER_LISTENER_IMPL
#define DATAREADER_LISTENER_IMPL

#include <dds/DdsDcpsSubscriptionS.h>
#include <dds/DdsDcpsPublicationC.h>

#if !defined (ACE_LACKS_PRAGMA_ONCE)
#pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

class PubMessageDataReaderImpl;
class AckMessageDataWriterImpl;

//Class PubDataReaderListenerImpl
class PubDataReaderListenerImpl
  : public virtual POA_DDS::DataReaderListener,
    public virtual PortableServer::RefCountServantBase
{
public:
  //Constructor
  PubDataReaderListenerImpl ();
  void init (DDS::DataReader_ptr dr, DDS::DataWriter_ptr dw);


  //Destructor
  virtual ~PubDataReaderListenerImpl (void);

  virtual void on_requested_deadline_missed (
    DDS::DataReader_ptr reader,
    const DDS::RequestedDeadlineMissedStatus & status)
    throw (CORBA::SystemException);

  virtual void on_requested_incompatible_qos (
    DDS::DataReader_ptr reader,
    const DDS::RequestedIncompatibleQosStatus & status)
  throw (CORBA::SystemException);

  virtual void on_liveliness_changed (
    DDS::DataReader_ptr reader,
    const DDS::LivelinessChangedStatus & status)
  throw (CORBA::SystemException);

  virtual void on_subscription_match (
    DDS::DataReader_ptr reader,
    const DDS::SubscriptionMatchStatus & status
  )
  throw (CORBA::SystemException);

  virtual void on_sample_rejected(
    DDS::DataReader_ptr reader,
    const DDS::SampleRejectedStatus& status
  )
  throw (CORBA::SystemException);

  virtual void on_data_available(
    DDS::DataReader_ptr reader
  )
  throw (CORBA::SystemException);

  virtual void on_sample_lost(
    DDS::DataReader_ptr reader,
    const DDS::SampleLostStatus& status
  )
  throw (CORBA::SystemException);

  int done() const {
    return this->done_;
  }

private:

  DDS::DataWriter_var writer_;
  DDS::DataReader_var reader_;
  PubMessageDataReaderImpl* dr_servant_;
  AckMessageDataWriterImpl* dw_servant_;
  DDS::InstanceHandle_t handle_;
  //  DDS::DataReader_var reader_;
  CORBA::Long  sample_num_;
  int   done_;
};

#endif /* DATAREADER_LISTENER_IMPL  */
