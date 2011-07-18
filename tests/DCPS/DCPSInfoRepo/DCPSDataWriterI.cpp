// -*- C++ -*-
//
// $Id$

#include "DCPSDataWriterI.h"
#include <dds/DCPS/RepoIdConverter.h>

// Implementation skeleton constructor
TAO_DDS_DCPSDataWriter_i::TAO_DDS_DCPSDataWriter_i (void)
  {
  }

// Implementation skeleton destructor
TAO_DDS_DCPSDataWriter_i::~TAO_DDS_DCPSDataWriter_i (void)
  {
  }

void TAO_DDS_DCPSDataWriter_i::add_association (
    const ::OpenDDS::DCPS::RepoId& yourId,
    const OpenDDS::DCPS::ReaderAssociation& reader
  )
  ACE_THROW_SPEC ((
    CORBA::SystemException
  ))
  {

    ACE_DEBUG((LM_DEBUG,
               ACE_TEXT("\nTAO_DDS_DCPSDataWriter_i::add_associations () :\n")
               ACE_TEXT("\tWriter %d Adding association to a reader:\n"),
               yourId));

    ACE_DEBUG((LM_DEBUG,
               ACE_TEXT("\t reader id - %d\n")
               ACE_TEXT("\t transport_id - %C\n"),
               reader.readerId,
               reader.readerTransInfo[0].transport_type.in()
           ));
  }


void TAO_DDS_DCPSDataWriter_i::remove_associations (
    const OpenDDS::DCPS::ReaderIdSeq & readers,
    ::CORBA::Boolean notify_lost
  )
  ACE_THROW_SPEC ((
    CORBA::SystemException
  ))
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
        ACE_DEBUG((LM_DEBUG,
                   ACE_TEXT("\tAssociation - %d\n")
                   ACE_TEXT("\t InstanceHandle_t - %d\n"),
                   cnt,
                   readers[cnt]
               ));
      }
  }


void TAO_DDS_DCPSDataWriter_i::update_incompatible_qos (
    const OpenDDS::DCPS::IncompatibleQosStatus & status
  )
  ACE_THROW_SPEC ((
    CORBA::SystemException
  ))
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
  }


void
TAO_DDS_DCPSDataWriter_i::update_subscription_params(
  const OpenDDS::DCPS::RepoId& id, const DDS::StringSeq& params)
ACE_THROW_SPEC((CORBA::SystemException))
{
  OpenDDS::DCPS::RepoIdConverter readerConv(id);
  ACE_DEBUG((LM_INFO,
             ACE_TEXT("\nTAO_DDS_DCPSDataWriter_i::update_subscription_params() :\n")
             ACE_TEXT("\treader = %C\n\tparams.length = %d\n"),
             std::string(readerConv).c_str(), params.length()));

#if TAO_MAJOR_VERSION > 1 || (TAO_MAJOR_VERSION == 1 && TAO_MINOR_VERSION > 5)
#define DOT_IN .in()
#else
#define DOT_IN
#endif

  const CORBA::ULong length = params.length();
  for (CORBA::ULong i = 0; i < length; ++i) {
    ACE_DEBUG((LM_INFO, ACE_TEXT("\tparams[%d] = %C\n"), i, params[i] DOT_IN));
  }
}
