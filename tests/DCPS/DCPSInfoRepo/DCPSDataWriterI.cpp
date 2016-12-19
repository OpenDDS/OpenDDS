// -*- C++ -*-
//

#include "DCPSDataWriterI.h"
#include <dds/DCPS/RepoIdConverter.h>

TAO_DDS_DCPSDataWriter_i::TAO_DDS_DCPSDataWriter_i (void)
  {
  }

TAO_DDS_DCPSDataWriter_i::~TAO_DDS_DCPSDataWriter_i (void)
  {
  }

void TAO_DDS_DCPSDataWriter_i::add_association (
    const ::OpenDDS::DCPS::RepoId& yourId,
    const OpenDDS::DCPS::ReaderAssociation& reader,
    bool /*active*/
  )
  {

    OpenDDS::DCPS::RepoIdConverter converterY(yourId);
    ACE_DEBUG((LM_DEBUG,
               ACE_TEXT("\nTAO_DDS_DCPSDataWriter_i::add_associations () :\n")
               ACE_TEXT("\tWriter %C Adding association to a reader:\n"),
               std::string(converterY).c_str()));

    OpenDDS::DCPS::RepoIdConverter converterR(reader.readerId);
    ACE_DEBUG((LM_DEBUG,
               ACE_TEXT("\t reader id - %C\n")
               ACE_TEXT("\t transport_id - %C\n"),
               std::string(converterR).c_str(),
               reader.readerTransInfo[0].transport_type.in()
           ));
    received_.received(DiscReceivedCalls::ADD_ASSOC);
  }


void TAO_DDS_DCPSDataWriter_i::remove_associations (
    const OpenDDS::DCPS::ReaderIdSeq & readers,
    ::CORBA::Boolean notify_lost
  )
  {
    ACE_UNUSED_ARG (notify_lost);
    CORBA::ULong length = readers.length();

    ACE_DEBUG((LM_DEBUG,
               ACE_TEXT("\nTAO_DDS_DCPSDataWriter_i::remove_associations () :\n")
               ACE_TEXT("\tRemoving association to %d readers:\n"),
               length
               ));

    for (CORBA::ULong cnt = 0; cnt < length; ++cnt)
      {
        OpenDDS::DCPS::RepoIdConverter converter(readers[cnt]);
        ACE_DEBUG((LM_DEBUG,
                   ACE_TEXT("\tAssociation - %d\n")
                   ACE_TEXT("\t RepoId - %C\n"),
                   cnt,
                   std::string(converter).c_str()
               ));
        received_.received(DiscReceivedCalls::REM_ASSOC);
      }
  }


void TAO_DDS_DCPSDataWriter_i::update_incompatible_qos (
    const OpenDDS::DCPS::IncompatibleQosStatus & status
  )
  {
    ACE_DEBUG((LM_INFO,
               ACE_TEXT("\n!!! TAO_DDS_DCPSDataWriter_i::update_incompatible_qos () :\n")
               ACE_TEXT("\t%d new incompatible DataReaders %d  total\n"),
               status.count_since_last_send, status.total_count
               ));
    ACE_DEBUG((LM_INFO,
               ACE_TEXT("\tLast incompatible QOS policy was %d\n"),
               status.last_policy_id
               ));

    CORBA::ULong length = status.policies.length();
    for (CORBA::ULong cnt = 0; cnt < length; ++cnt)
      {
        ACE_DEBUG((LM_INFO,
                   ACE_TEXT("\tPolicy - %d")
                   ACE_TEXT("\tcount - %d\n"),
                   status.policies[cnt].policy_id,
                   status.policies[cnt].count
               ));
      }
    received_.received(DiscReceivedCalls::UPDATE_INCOMP_QOS);
  }


void
TAO_DDS_DCPSDataWriter_i::update_subscription_params(
  const OpenDDS::DCPS::RepoId& id, const DDS::StringSeq& params)
{
  OpenDDS::DCPS::RepoIdConverter readerConv(id);
  ACE_DEBUG((LM_INFO,
             ACE_TEXT("\nTAO_DDS_DCPSDataWriter_i::update_subscription_params() :\n")
             ACE_TEXT("\treader = %C\n\tparams.length = %d\n"),
             std::string(readerConv).c_str(), params.length()));
  const CORBA::ULong length = params.length();
  for (CORBA::ULong i = 0; i < length; ++i) {
    ACE_DEBUG((LM_INFO, ACE_TEXT("\tparams[%d] = %C\n"), i, params[i].in()));
  }
  received_.received(DiscReceivedCalls::UPDATE_SUB_PARAMS);
}


void
TAO_DDS_DCPSDataWriter_i::_add_ref()
{
  OpenDDS::DCPS::RcObject<ACE_SYNCH_MUTEX>::_add_ref();
}

void
TAO_DDS_DCPSDataWriter_i::_remove_ref()
{
  OpenDDS::DCPS::RcObject<ACE_SYNCH_MUTEX>::_remove_ref();
}
