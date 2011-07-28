/*
 * $Id$
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_DATAWRITERREMOTE_H
#define OPENDDS_DCPS_DATAWRITERREMOTE_H

#include "dds/DdsDcpsDataWriterRemoteS.h"
#include "Definitions.h"

#include "ace/Thread_Mutex.h"

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
  explicit DataWriterRemoteImpl(DataWriterImpl* parent);

  virtual ~DataWriterRemoteImpl();

  virtual void add_association(const RepoId& yourId,
                               const ReaderAssociation& readers,
                               bool active)
  ACE_THROW_SPEC((CORBA::SystemException));

  virtual void remove_associations(const ReaderIdSeq& readers,
                                   CORBA::Boolean callback)
  ACE_THROW_SPEC((CORBA::SystemException));

  virtual void update_incompatible_qos(const IncompatibleQosStatus& status)
  ACE_THROW_SPEC((CORBA::SystemException));

  virtual void update_subscription_params(const RepoId& readerId,
                                          const DDS::StringSeq& exprParams)
  ACE_THROW_SPEC((CORBA::SystemException));

  void detach_parent();

private:
  DataWriterImpl* parent_;
  ACE_Thread_Mutex mutex_;
};

} // namespace DCPS
} // namespace OpenDDS

#endif
