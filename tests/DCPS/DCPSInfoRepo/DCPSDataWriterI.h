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
#include "dds/DCPS/RcObject_T.h"

class TAO_DDS_DCPSDataWriter_i
  : public OpenDDS::DCPS::DataWriterCallbacks
  , public OpenDDS::DCPS::RcObject<ACE_SYNCH_MUTEX>
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

  virtual void association_complete(const OpenDDS::DCPS::RepoId& /*remote_id*/) { received_.received(DiscReceivedCalls::ASSOC_COMPLETE); }

  virtual void remove_associations (
      const OpenDDS::DCPS::ReaderIdSeq & readers,
      ::CORBA::Boolean notify_lost);

  virtual void update_incompatible_qos (
      const OpenDDS::DCPS::IncompatibleQosStatus & status);

  virtual void update_subscription_params(
    const OpenDDS::DCPS::RepoId&, const DDS::StringSeq &);

  void inconsistent_topic() {}

  DiscReceivedCalls& received()
    {
      return received_;
    }

  void _add_ref();
  void _remove_ref();
private:
  DiscReceivedCalls received_;
};


#endif /* DCPSDATAWRITERI_H_  */
