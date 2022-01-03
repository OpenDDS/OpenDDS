// -*- C++ -*-
//

#ifndef DCPSDATAWRITERI_H_
#define DCPSDATAWRITERI_H_

#include "dds/DCPS/DataWriterCallbacks.h"
#include "dds/DdsDcpsInfrastructureC.h"

#include <vector>

#if !defined (ACE_LACKS_PRAGMA_ONCE)
#pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

#include "DiscReceivedCalls.h"
#include "dds/DCPS/RcObject.h"

class TAO_DDS_DCPSDataWriter_i
  : public OpenDDS::DCPS::DataWriterCallbacks
{
public:
  TAO_DDS_DCPSDataWriter_i (void);

  virtual ~TAO_DDS_DCPSDataWriter_i (void);

  virtual ::DDS::ReturnCode_t enable_specific ()
      { received_.received(DiscReceivedCalls::ENABLE_SPECIFIC); return ::DDS::RETCODE_OK;};

  virtual void add_association (
      const ::OpenDDS::DCPS::RepoId& yourId,
      const OpenDDS::DCPS::ReaderAssociation& reader,
      bool active);

  virtual void remove_associations (
      const OpenDDS::DCPS::ReaderIdSeq & readers,
      ::CORBA::Boolean notify_lost);

  virtual void update_incompatible_qos (
      const OpenDDS::DCPS::IncompatibleQosStatus & status);

  virtual void update_subscription_params(
    const OpenDDS::DCPS::RepoId&, const DDS::StringSeq &);

  DiscReceivedCalls& received()
    {
      return received_;
    }

  OpenDDS::DCPS::WeakRcHandle<OpenDDS::ICE::Endpoint> get_ice_endpoint() { return OpenDDS::DCPS::WeakRcHandle<OpenDDS::ICE::Endpoint>(); }

private:
  DiscReceivedCalls received_;
};


#endif /* DCPSDATAWRITERI_H_  */
