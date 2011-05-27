// -*- C++ -*-
//
// $Id$


#include "DCPS/DdsDcps_pch.h" //Only the _pch include should start with DCPS/
#include "DataWriterRemoteImpl.h"
#include "DataWriterImpl.h"

namespace OpenDDS
{
  namespace DCPS
  {

    //TBD - add check for enabled in most methods.
    //      currently this is not needed because auto_enable_created_entities
    //      cannot be false.
#if 0
    // Emacs trick to align code with first column
    // This will cause emacs to emit bogus alignment message
    // For now just disregard them.
  }}
#endif

DataWriterRemoteImpl::DataWriterRemoteImpl (DataWriterImpl* parent)
  : parent_ (parent)
{
}

// This method is called when there are no longer any reference to the
// the servant.
DataWriterRemoteImpl::~DataWriterRemoteImpl (void)
{
}


void
DataWriterRemoteImpl::add_associations ( const ::OpenDDS::DCPS::RepoId& yourId,
           const ReaderAssociationSeq & readers
           )
  ACE_THROW_SPEC (( CORBA::SystemException ))
{
  parent_->add_associations (yourId, readers);
}



void
DataWriterRemoteImpl::remove_associations ( const ReaderIdSeq & readers,
              ::CORBA::Boolean notify_lost
              )
  ACE_THROW_SPEC (( CORBA::SystemException ))
{
  parent_->remove_associations (readers, notify_lost);
}



void
DataWriterRemoteImpl::update_incompatible_qos ( const OpenDDS::DCPS::IncompatibleQosStatus & status
            )
  ACE_THROW_SPEC (( CORBA::SystemException ))
{
  parent_->update_incompatible_qos(status);
}

} // namespace DCPS
} // namespace OpenDDS
