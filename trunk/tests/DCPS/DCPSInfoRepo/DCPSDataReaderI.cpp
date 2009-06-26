// -*- C++ -*-
//
// $Id$

#include "dds/DCPS/RepoIdConverter.h"

#include "DCPSDataReaderI.h"

// Implementation skeleton constructor
TAO_DDS_DCPSDataReader_i::TAO_DDS_DCPSDataReader_i (void)
  {
  }

// Implementation skeleton destructor
TAO_DDS_DCPSDataReader_i::~TAO_DDS_DCPSDataReader_i (void)
  {
  }

void TAO_DDS_DCPSDataReader_i::add_associations (
    const ::OpenDDS::DCPS::RepoId& yourId,
    const OpenDDS::DCPS::WriterAssociationSeq & writers
  )
  ACE_THROW_SPEC ((
    CORBA::SystemException
  ))
  {
    CORBA::ULong length = writers.length();

    OpenDDS::DCPS::RepoIdConverter converter(yourId);
    ACE_DEBUG((LM_DEBUG,
      ACE_TEXT("\nTAO_DDS_DCPSDataReader_i::add_associations () :\n")
      ACE_TEXT("\tReader %C Adding association to %d writers:\n"),
      std::string(converter).c_str(),
      length
    ));

    for (CORBA::ULong cnt = 0; cnt < length; ++cnt)
      {
        OpenDDS::DCPS::RepoIdConverter converter(writers[cnt].writerId);
        ACE_DEBUG((LM_DEBUG,
          ACE_TEXT("\tAssociation - %d\n")
          ACE_TEXT("\t writer id - %C\n")
          ACE_TEXT("\t transport_id - %d\n\n"),
          cnt,
          std::string(converter).c_str(),
          writers[ cnt].writerTransInfo.transport_id
        ));
      }

  }


void TAO_DDS_DCPSDataReader_i::remove_associations (
    const OpenDDS::DCPS::WriterIdSeq & writers,
    ::CORBA::Boolean notify_lost
  )
  ACE_THROW_SPEC ((
    CORBA::SystemException
  ))
  {
    ACE_UNUSED_ARG (notify_lost);
    CORBA::ULong length = writers.length();

    ACE_DEBUG((LM_DEBUG,
               ACE_TEXT("\nTAO_DDS_DCPSDataReader_i::remove_associations () :\n")
               ACE_TEXT("\tRemoving association to %d writers:\n"),
               length
               ));

    for (CORBA::ULong cnt = 0; cnt < length; ++cnt)
      {
        OpenDDS::DCPS::RepoIdConverter converter(writers[cnt]);
        ACE_DEBUG((LM_DEBUG,
          ACE_TEXT("\tAssociation - %d\n")
          ACE_TEXT("\t writer_id - %C\n"),
          cnt,
          std::string(converter).c_str()
        ));
      }

  }

void TAO_DDS_DCPSDataReader_i::update_incompatible_qos (
    const OpenDDS::DCPS::IncompatibleQosStatus & status
  )
  ACE_THROW_SPEC ((
    CORBA::SystemException
  ))
  {
    ACE_DEBUG((LM_INFO,
               ACE_TEXT("\n!!! TAO_DDS_DCPSDataReader_i::update_incompatible_qos () :\n")
               ACE_TEXT("\t%d new incompatible DataWriters %d  total\n"),
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

