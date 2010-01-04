/*
 * $Id$
 *
 * Copyright 2010 Object Computing, Inc.
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "DCPS/DdsDcps_pch.h" //Only the _pch include should start with DCPS/
#include "DataReaderRemoteImpl.h"
#include "DataReaderImpl.h"

namespace OpenDDS {
namespace DCPS {

DataReaderRemoteImpl::DataReaderRemoteImpl(DataReaderImpl* parent) :
    parent_(parent)
{
}

// This method is called when there are no longer any reference to the
// the servant.
DataReaderRemoteImpl::~DataReaderRemoteImpl()
{
}

void DataReaderRemoteImpl::add_associations(const OpenDDS::DCPS::RepoId& yourId,
                                            const OpenDDS::DCPS::WriterAssociationSeq & writers)
ACE_THROW_SPEC((CORBA::SystemException))
{
  parent_->add_associations(yourId, writers);
}

void DataReaderRemoteImpl::remove_associations(
  const OpenDDS::DCPS::WriterIdSeq & writers,
  CORBA::Boolean notify_lost)
ACE_THROW_SPEC((CORBA::SystemException))
{
  parent_->remove_associations(writers, notify_lost);
}

void DataReaderRemoteImpl::update_incompatible_qos(
  const OpenDDS::DCPS::IncompatibleQosStatus & status)
ACE_THROW_SPEC((CORBA::SystemException))
{
  parent_->update_incompatible_qos(status);
}

} // namespace DCPS
} // namespace OpenDDS
