// -*- C++ -*-
//
// $Id$
#ifndef NULLLISTENER_H
#define NULLLISTENER_H

#include <ace/config-macros.h>

#if !defined (ACE_LACKS_PRAGMA_ONCE)
#pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

#include "model_export.h"
#include "dds/DdsDcpsSubscriptionS.h"
#include "dds/DCPS/Definitions.h"

namespace OpenDDS { namespace Model {

class OpenDDS_Model_Export NullListener
  : public virtual OpenDDS::DCPS::LocalObject<DDS::DataReaderListener>
{
public:
  //Constructor
  NullListener();

  //Destructor
  virtual ~NullListener();

  virtual void on_requested_deadline_missed (
    DDS::DataReader_ptr reader,
    const DDS::RequestedDeadlineMissedStatus& status
  );

 virtual void on_requested_incompatible_qos (
    DDS::DataReader_ptr reader,
    const DDS::RequestedIncompatibleQosStatus& status
  );

  virtual void on_liveliness_changed (
    DDS::DataReader_ptr reader,
    const DDS::LivelinessChangedStatus& status
  );

  virtual void on_subscription_match (
    DDS::DataReader_ptr reader,
    const DDS::SubscriptionMatchedStatus& status
  );

  virtual void on_sample_rejected(
    DDS::DataReader_ptr reader,
    const DDS::SampleRejectedStatus& status
  );

  virtual void on_data_available(
    DDS::DataReader_ptr reader
  );

  virtual void on_sample_lost(
    DDS::DataReader_ptr reader,
    const DDS::SampleLostStatus& status
  );

};

} } // End of namespace OpenDDS::Model

#endif /* NULLLISTENER_H  */

