// -*- C++ -*-
//
// $Id$

#include "DCPS/DdsDcps_pch.h" //Only the _pch include should start with DCPS/
#include "DataReaderRemoteImpl.h"
#include "DataReaderImpl.h"


namespace TAO
{
  namespace DCPS
  {

#if 0
    // Emacs trick to align code with first column
    // This will cause emacs to emit bogus alignment message
    // For now just disregard them.
  }}
#endif


DataReaderRemoteImpl::DataReaderRemoteImpl (DataReaderImpl* parent) :
  parent_(parent)
{
}

// This method is called when there are no longer any reference to the
// the servant.
DataReaderRemoteImpl::~DataReaderRemoteImpl (void)
{
}



void DataReaderRemoteImpl::add_associations (::TAO::DCPS::RepoId yourId,
				       const TAO::DCPS::WriterAssociationSeq & writers)
  ACE_THROW_SPEC ((CORBA::SystemException))
{
  parent_->add_associations( yourId, writers);
}


void DataReaderRemoteImpl::remove_associations (
					  const TAO::DCPS::WriterIdSeq & writers,
					  ::CORBA::Boolean notify_lost
					  )
  ACE_THROW_SPEC ((
		   CORBA::SystemException
		   ))
{
  parent_->remove_associations ( writers, notify_lost);
}



void DataReaderRemoteImpl::update_incompatible_qos (
					      const TAO::DCPS::IncompatibleQosStatus & status
					      )
  ACE_THROW_SPEC ((
		   CORBA::SystemException
		   ))
{
    parent_->update_incompatible_qos (status);
}

} // namespace DCPS
} // namespace TAO
