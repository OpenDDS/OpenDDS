/*
 * $Id$
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_DATAREADERREMOTE_H
#define OPENDDS_DCPS_DATAREADERREMOTE_H

#include "dcps_export.h"
#include "DdsDcpsDataReaderRemoteS.h"
#include "Definitions.h"

#if !defined (ACE_LACKS_PRAGMA_ONCE)
#pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

namespace OpenDDS {
namespace DCPS {

class DataReaderImpl;

/**
* @class DataReaderRemoteImpl
*
* @brief Implements the OpenDDS::DCPS::ReaderRemote interface that
*        is used to add and remove associations.
*
*/
class OpenDDS_Dcps_Export DataReaderRemoteImpl
  : public virtual POA_OpenDDS::DCPS::DataReaderRemote {
public:

  //Constructor
  DataReaderRemoteImpl(DataReaderImpl* parent);

  //Destructor
  virtual ~DataReaderRemoteImpl();

  virtual void add_associations(
    const OpenDDS::DCPS::RepoId& yourId,
    const OpenDDS::DCPS::WriterAssociationSeq & writers)
  ACE_THROW_SPEC((CORBA::SystemException));

  virtual void remove_associations(
    const OpenDDS::DCPS::WriterIdSeq & writers,
    CORBA::Boolean callback)
  ACE_THROW_SPEC((CORBA::SystemException));

  virtual void update_incompatible_qos(
    const OpenDDS::DCPS::IncompatibleQosStatus & status)
  ACE_THROW_SPEC((CORBA::SystemException));
private:
  DataReaderImpl* parent_;

};

} // namespace DCPS
} // namespace OpenDDS

#endif /* OPENDDS_DCPS_DATAREADERREMOTE_H  */
