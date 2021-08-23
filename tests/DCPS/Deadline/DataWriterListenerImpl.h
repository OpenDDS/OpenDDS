// -*- C++ -*-
//

#ifndef DATAWRITER_LISTENER_IMPL
#define DATAWRITER_LISTENER_IMPL

#include <dds/DCPS/Definitions.h>
#include <dds/DCPS/LocalObject.h>
#include <dds/DCPS/ConditionVariable.h>
#include <dds/DCPS/TimeTypes.h>

#include <dds/DdsDcpsPublicationC.h>

#include <ace/Thread_Mutex.h>

#if !defined (ACE_LACKS_PRAGMA_ONCE)
#pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

class DataWriterListenerImpl
  : public virtual OpenDDS::DCPS::LocalObject<OpenDDS::DCPS::DataWriterListener>
{
public:
  DataWriterListenerImpl();

  bool wait_matched(long count, const OpenDDS::DCPS::TimeDuration& max_wait) const;
  CORBA::Long offered_deadline_total_count() const;

  virtual void on_offered_deadline_missed(
      DDS::DataWriter_ptr writer,
      const DDS::OfferedDeadlineMissedStatus& status);

  virtual void on_publication_matched(
      DDS::DataWriter_ptr writer,
      const DDS::PublicationMatchedStatus& status);

  virtual void on_offered_incompatible_qos(
      DDS::DataWriter_ptr writer,
      const DDS::OfferedIncompatibleQosStatus& status);

  virtual void on_liveliness_lost(
      DDS::DataWriter_ptr writer,
      const DDS::LivelinessLostStatus& status);

  virtual void on_publication_disconnected(
      DDS::DataWriter_ptr reader,
      const OpenDDS::DCPS::PublicationDisconnectedStatus& status);

  virtual void on_publication_reconnected(
      DDS::DataWriter_ptr reader,
      const OpenDDS::DCPS::PublicationReconnectedStatus& status);

  virtual void on_publication_lost(
      DDS::DataWriter_ptr writer,
      const OpenDDS::DCPS::PublicationLostStatus& status);

protected:
  virtual ~DataWriterListenerImpl();

private:
  typedef ACE_Thread_Mutex Mutex;
  typedef ACE_Guard<Mutex> Lock;
  mutable Mutex mutex_;
  mutable OpenDDS::DCPS::ConditionVariable<Mutex> matched_condition_;
  long matched_;
  CORBA::Long offered_deadline_total_count_;
};

#endif /* DATAWRITER_LISTENER_IMPL  */
