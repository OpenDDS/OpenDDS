// -*- C++ -*-
//
// $Id$

#include "DCPSDataWriterI.h"

// Implementation skeleton constructor
TAO_DDS_DCPSDataWriter_i::TAO_DDS_DCPSDataWriter_i (void)
  {
  }
  
// Implementation skeleton destructor
TAO_DDS_DCPSDataWriter_i::~TAO_DDS_DCPSDataWriter_i (void)
  {
  }
  
void TAO_DDS_DCPSDataWriter_i::add_associations (
    ::TAO::DCPS::RepoId yourId,
    const TAO::DCPS::ReaderAssociationSeq & readers
    ACE_ENV_ARG_DECL
  )
  ACE_THROW_SPEC ((
    CORBA::SystemException
  ))
  {

    CORBA::ULong length = readers.length();

    ACE_DEBUG((LM_DEBUG,
               ACE_TEXT("\nTAO_DDS_DCPSDataWriter_i::add_associations () :\n")
               ACE_TEXT("\tWriter %d Adding association to %d readers:\n"),
               yourId,
               length
               ));

    for (CORBA::ULong cnt = 0; cnt < length; ++cnt)
      {
        ACE_DEBUG((LM_DEBUG,
                   ACE_TEXT("\tAssociation - %d\n")
                   ACE_TEXT("\t reader id - %d\n")
                   ACE_TEXT("\t transport_id - %d\n"),
                   cnt,
                   readers[cnt].readerId,
                   readers[cnt].readerTransInfo.transport_id
               ));
      }
  }


void TAO_DDS_DCPSDataWriter_i::remove_associations (
    const TAO::DCPS::ReaderIdSeq & readers
    ACE_ENV_ARG_DECL
  )
  ACE_THROW_SPEC ((
    CORBA::SystemException
  ))
  {
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
    const TAO::DCPS::IncompatibleQosStatus & status
    ACE_ENV_ARG_DECL
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


