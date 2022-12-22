// -*- C++ -*-
//

#ifndef DCPSDATAREADERI_H_
#define DCPSDATAREADERI_H_

#include "dds/DCPS/DataReaderCallbacks.h"
#include "dds/DdsDcpsInfrastructureC.h"

#include "DiscReceivedCalls.h"

#include "dds/DCPS/Discovery.h"

#include <vector>

#if !defined (ACE_LACKS_PRAGMA_ONCE)
#pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

class TAO_DDS_DCPSDataReader_i
  : public OpenDDS::DCPS::DataReaderCallbacks
{
public:
  enum Called { ENABLE_SPECIFIC, ADD_ASSOC, REM_ASSOC, UPDATE_INCOMP_QOS };

  TAO_DDS_DCPSDataReader_i (void);

  virtual ~TAO_DDS_DCPSDataReader_i (void);


  virtual ::DDS::ReturnCode_t enable_specific ()
    { received_.received(DiscReceivedCalls::ENABLE_SPECIFIC); return ::DDS::RETCODE_OK;};

  virtual void add_association (
      const ::OpenDDS::DCPS::RepoId& yourId,
      const OpenDDS::DCPS::WriterAssociation& writer,
      bool active);

  virtual void remove_associations (
      const OpenDDS::DCPS::WriterIdSeq & writers,
      ::CORBA::Boolean notify_lost);

  virtual void update_incompatible_qos (
      const OpenDDS::DCPS::IncompatibleQosStatus & status);

  virtual void signal_liveliness(const OpenDDS::DCPS::RepoId& /*remote_participant*/) { }

  DiscReceivedCalls& received()
    {
      return received_;
    }
  OpenDDS::DCPS::Discovery* disco_;
  DDS::DomainId_t domainId_;
  ::OpenDDS::DCPS::RepoId participantId_;

  OpenDDS::DCPS::WeakRcHandle<OpenDDS::ICE::Endpoint> get_ice_endpoint() { return OpenDDS::DCPS::WeakRcHandle<OpenDDS::ICE::Endpoint>(); }

private:
  DiscReceivedCalls received_;
};


#endif /* DCPSDATAREADERI_H_  */
