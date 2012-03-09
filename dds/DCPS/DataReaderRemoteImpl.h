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

#include "ace/Thread_Mutex.h"

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

  explicit DataReaderRemoteImpl(DataReaderImpl* parent);

  virtual ~DataReaderRemoteImpl();

  virtual void add_association(const RepoId& yourId,
                               const WriterAssociation& writer,
                               bool active);

  virtual void association_complete(const RepoId& remote_id);

  virtual void remove_associations(const WriterIdSeq& writers,
                                   CORBA::Boolean callback);

  virtual void update_incompatible_qos(const IncompatibleQosStatus& status);

  void detach_parent();

private:
  DataReaderImpl* parent_;
  ACE_Thread_Mutex mutex_;
};

} // namespace DCPS
} // namespace OpenDDS

#endif /* OPENDDS_DCPS_DATAREADERREMOTE_H  */
