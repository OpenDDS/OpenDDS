/*
 * $Id$
 *
 * Copyright 2010 Object Computing, Inc.
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_DATAWRITERREMOTE_H
#define OPENDDS_DCPS_DATAWRITERREMOTE_H

#include "dds/DdsDcpsDataWriterRemoteS.h"
#include "Definitions.h"

#if !defined (ACE_LACKS_PRAGMA_ONCE)
#pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

namespace OpenDDS {
namespace DCPS {

class DataWriterImpl;

/**
* @class DataWriterRemoteImpl
*
* @brief Implements the OpenDDS::DCPS::DataWriterRemote interface.
*
*/
class OpenDDS_Dcps_Export DataWriterRemoteImpl
  : public virtual POA_OpenDDS::DCPS::DataWriterRemote {
public:
  ///Constructor
  DataWriterRemoteImpl(DataWriterImpl* parent);

  ///Destructor
  virtual ~DataWriterRemoteImpl();

  virtual void add_associations(
    const OpenDDS::DCPS::RepoId& yourId,
    const ReaderAssociationSeq & readers)
  ACE_THROW_SPEC((CORBA::SystemException));

  virtual void remove_associations(
    const ReaderIdSeq & readers,
    CORBA::Boolean callback)
  ACE_THROW_SPEC((CORBA::SystemException));

  virtual void update_incompatible_qos(
    const OpenDDS::DCPS::IncompatibleQosStatus & status)
  ACE_THROW_SPEC((CORBA::SystemException));

private:
  DataWriterImpl* parent_;
};

} // namespace DCPS
} // namespace OpenDDS

#endif
