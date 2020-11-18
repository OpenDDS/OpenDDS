#ifdef OPENDDS_SECURITY

/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "DCPS/DdsDcps_pch.h" //Only the _pch include should start with DCPS/
#include "HandleRegistry.h"

#include "dds/DCPS/GuidConverter.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace Security {

HandleRegistry::~HandleRegistry()
{
  if (DCPS::security_debug.bookkeeping) {
    ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) {bookkeeping} ")
               ACE_TEXT("HandleRegistry::~HandleRegistry local datareader %B local datawriter %B remote participant %B remote datareader %B remote datawriter %B\n"),
               local_datareader_crypto_handles_.size(),
               local_datawriter_crypto_handles_.size(),
               remote_participant_crypto_handles_.size(),
               remote_datareader_crypto_handles_.size(),
               remote_datawriter_crypto_handles_.size()));
  }
}

void
HandleRegistry::insert_local_datareader_crypto_handle(const DCPS::RepoId& id,
                                                          DDS::Security::DatareaderCryptoHandle handle,
                                                          DDS::Security::EndpointSecurityAttributes attributes)
{
  if (handle != DDS::HANDLE_NIL) {
    ACE_GUARD(ACE_Thread_Mutex, guard, mutex_);
    local_datareader_crypto_handles_[id] = std::make_pair(handle, attributes);

    if (DCPS::security_debug.bookkeeping) {
      ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) {bookkeeping} ")
                 ACE_TEXT("HandleRegistry::insert_local_datareader_crypto_handle %C %d (total %B)\n"),
                 DCPS::LogGuid(id).c_str(),
                 handle,
                 local_datareader_crypto_handles_.size()));
    }
  }
}

DDS::Security::DatareaderCryptoHandle
HandleRegistry::get_local_datareader_crypto_handle(const DCPS::RepoId& id) const
{
  ACE_GUARD_RETURN(ACE_Thread_Mutex, guard, mutex_, DDS::HANDLE_NIL);
  DatareaderCryptoHandleMap::const_iterator pos = local_datareader_crypto_handles_.find(id);
  if (pos != local_datareader_crypto_handles_.end()) {
    return pos->second.first;
  }
  return DDS::HANDLE_NIL;
}

DDS::Security::EndpointSecurityAttributes
HandleRegistry::get_local_datareader_security_attributes(const DCPS::RepoId& id) const
{
  ACE_GUARD_RETURN(ACE_Thread_Mutex, guard, mutex_, DDS::Security::EndpointSecurityAttributes());
  DatareaderCryptoHandleMap::const_iterator pos = local_datareader_crypto_handles_.find(id);
  if (pos != local_datareader_crypto_handles_.end()) {
    return pos->second.second;
  }
  return DDS::Security::EndpointSecurityAttributes();
}

void
HandleRegistry::erase_local_datareader_crypto_handle(const DCPS::RepoId& id)
{
  ACE_GUARD(ACE_Thread_Mutex, guard, mutex_);
  local_datareader_crypto_handles_.erase(id);

  if (DCPS::security_debug.bookkeeping) {
    ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) {bookkeeping} ")
               ACE_TEXT("HandleRegistry::erase_local_datareader_crypto_handle %C (%B)\n"),
               DCPS::LogGuid(id).c_str(),
               local_datareader_crypto_handles_.size()));
  }
}

void
HandleRegistry::insert_local_datawriter_crypto_handle(const DCPS::RepoId& id,
                                                          DDS::Security::DatawriterCryptoHandle handle,
                                                          DDS::Security::EndpointSecurityAttributes attributes)
{
  if (handle != DDS::HANDLE_NIL) {
    ACE_GUARD(ACE_Thread_Mutex, guard, mutex_);
    local_datawriter_crypto_handles_[id] = std::make_pair(handle, attributes);

    if (DCPS::security_debug.bookkeeping) {
      ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) {bookkeeping} ")
                 ACE_TEXT("HandleRegistry::insert_local_datawriter_crypto_handle %C %d (total %B)\n"),
                 DCPS::LogGuid(id).c_str(),
                 handle,
                 local_datawriter_crypto_handles_.size()));
    }
  }
}

DDS::Security::DatawriterCryptoHandle
HandleRegistry::get_local_datawriter_crypto_handle(const DCPS::RepoId& id) const
{
  ACE_GUARD_RETURN(ACE_Thread_Mutex, guard, mutex_, DDS::HANDLE_NIL);
  DatawriterCryptoHandleMap::const_iterator pos = local_datawriter_crypto_handles_.find(id);
  if (pos != local_datawriter_crypto_handles_.end()) {
    return pos->second.first;
  }
  return DDS::HANDLE_NIL;
}

DDS::Security::EndpointSecurityAttributes
HandleRegistry::get_local_datawriter_security_attributes(const DCPS::RepoId& id) const
{
  ACE_GUARD_RETURN(ACE_Thread_Mutex, guard, mutex_, DDS::Security::EndpointSecurityAttributes());
  DatawriterCryptoHandleMap::const_iterator pos = local_datawriter_crypto_handles_.find(id);
  if (pos != local_datawriter_crypto_handles_.end()) {
    return pos->second.second;
  }
  return DDS::Security::EndpointSecurityAttributes();
}

void
HandleRegistry::erase_local_datawriter_crypto_handle(const DCPS::RepoId& id)
{
  ACE_GUARD(ACE_Thread_Mutex, guard, mutex_);
  local_datawriter_crypto_handles_.erase(id);

  if (DCPS::security_debug.bookkeeping) {
    ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) {bookkeeping} ")
               ACE_TEXT("HandleRegistry::erase_local_datawriter_crypto_handle %C (total %B)\n"),
               DCPS::LogGuid(id).c_str(),
               local_datawriter_crypto_handles_.size()));
  }
}

void
HandleRegistry::insert_remote_participant_crypto_handle(const DCPS::RepoId& id,
                                                            DDS::Security::ParticipantCryptoHandle handle)
{
  if (handle != DDS::HANDLE_NIL) {
    ACE_GUARD(ACE_Thread_Mutex, guard, mutex_);
    remote_participant_crypto_handles_[id] = handle;

    if (DCPS::security_debug.bookkeeping) {
      ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) {bookkeeping} ")
                 ACE_TEXT("HandleRegistry::insert_remote_participant_crypto_handle %C %d (total %B)\n"),
                 DCPS::LogGuid(id).c_str(),
                 handle,
                 remote_participant_crypto_handles_.size()));
    }
  }
}

DDS::Security::ParticipantCryptoHandle
HandleRegistry::get_remote_participant_crypto_handle(const DCPS::RepoId& id) const
{
  ACE_GUARD_RETURN(ACE_Thread_Mutex, guard, mutex_, DDS::HANDLE_NIL);
  ParticipantCryptoHandleMap::const_iterator pos = remote_participant_crypto_handles_.find(id);
  if (pos != remote_participant_crypto_handles_.end()) {
    return pos->second;
  }
  return DDS::HANDLE_NIL;
}

void
HandleRegistry::erase_remote_participant_crypto_handle(const DCPS::RepoId& id)
{
  ACE_GUARD(ACE_Thread_Mutex, guard, mutex_);
  remote_participant_crypto_handles_.erase(id);

  if (DCPS::security_debug.bookkeeping) {
    ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) {bookkeeping} ")
               ACE_TEXT("HandleRegistry::erase_remote_participant_crypto_handle %C (total %B)\n"),
               DCPS::LogGuid(id).c_str(),
               remote_participant_crypto_handles_.size()));
  }
}

void
HandleRegistry::insert_remote_datareader_crypto_handle(const DCPS::RepoId& id,
                                                           DDS::Security::DatareaderCryptoHandle handle,
                                                           DDS::Security::EndpointSecurityAttributes attributes)
{
  if (handle != DDS::HANDLE_NIL) {
    ACE_GUARD(ACE_Thread_Mutex, guard, mutex_);
    remote_datareader_crypto_handles_[id] = std::make_pair(handle, attributes);
  }

  if (DCPS::security_debug.bookkeeping) {
    ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) {bookkeeping} ")
               ACE_TEXT("HandleRegistry::insert_remote_datareader_crypto_handle %C %d (total %B)\n"),
               DCPS::LogGuid(id).c_str(),
               handle,
               remote_datareader_crypto_handles_.size()));
  }
}

DDS::Security::DatareaderCryptoHandle
HandleRegistry::get_remote_datareader_crypto_handle(const DCPS::RepoId& id) const
{
  ACE_GUARD_RETURN(ACE_Thread_Mutex, guard, mutex_, DDS::HANDLE_NIL);
  DatareaderCryptoHandleMap::const_iterator pos = remote_datareader_crypto_handles_.find(id);
  if (pos != remote_datareader_crypto_handles_.end()) {
    return pos->second.first;
  }
  return DDS::HANDLE_NIL;
}

DDS::Security::EndpointSecurityAttributes
HandleRegistry::get_remote_datareader_security_attributes(const DCPS::RepoId& id) const
{
  ACE_GUARD_RETURN(ACE_Thread_Mutex, guard, mutex_, DDS::Security::EndpointSecurityAttributes());
  DatareaderCryptoHandleMap::const_iterator pos = remote_datareader_crypto_handles_.find(id);
  if (pos != remote_datareader_crypto_handles_.end()) {
    return pos->second.second;
  }
  return DDS::Security::EndpointSecurityAttributes();
}

HandleRegistry::DatareaderCryptoHandleList
HandleRegistry::get_all_remote_datareaders(const DCPS::RepoId& prefix) const
{
  const DCPS::GuidPrefixEqual guid_prefix_equal;
  DatareaderCryptoHandleList retval;
  ACE_GUARD_RETURN(ACE_Thread_Mutex, guard, mutex_, retval);
  for (DatareaderCryptoHandleMap::const_iterator pos =
         remote_datareader_crypto_handles_.lower_bound(DCPS::make_id(prefix, DCPS::ENTITYID_UNKNOWN)),
         limit = remote_datareader_crypto_handles_.end();
       pos != limit && guid_prefix_equal(pos->first, prefix); ++pos) {
    retval.push_back(std::make_pair(pos->first, pos->second.first));
  }
  return retval;
}

void
HandleRegistry::erase_remote_datareader_crypto_handle(const DCPS::RepoId& id)
{
  ACE_GUARD(ACE_Thread_Mutex, guard, mutex_);
  remote_datareader_crypto_handles_.erase(id);

  if (DCPS::security_debug.bookkeeping) {
    ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) {bookkeeping} ")
               ACE_TEXT("HandleRegistry::erase_remote_datareader_crypto_handle %C (total %B)\n"),
               DCPS::LogGuid(id).c_str(),
               remote_datareader_crypto_handles_.size()));
  }
}

void
HandleRegistry::insert_remote_datawriter_crypto_handle(const DCPS::RepoId& id,
                                                           DDS::Security::DatawriterCryptoHandle handle,
                                                           DDS::Security::EndpointSecurityAttributes attributes)
{
  OPENDDS_ASSERT(id.entityId != DCPS::ENTITYID_UNKNOWN);
  if (handle != DDS::HANDLE_NIL) {
    ACE_GUARD(ACE_Thread_Mutex, guard, mutex_);
    remote_datawriter_crypto_handles_[id] = std::make_pair(handle, attributes);

    if (DCPS::security_debug.bookkeeping) {
      ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) {bookkeeping} ")
                 ACE_TEXT("HandleRegistry::insert_remote_datawriter_crypto_handle %C %d (total %B)\n"),
                 DCPS::LogGuid(id).c_str(),
                 handle,
                 remote_datawriter_crypto_handles_.size()));
    }
  }
}

DDS::Security::DatawriterCryptoHandle
HandleRegistry::get_remote_datawriter_crypto_handle(const DCPS::RepoId& id) const
{
  ACE_GUARD_RETURN(ACE_Thread_Mutex, guard, mutex_, DDS::HANDLE_NIL);
  DatawriterCryptoHandleMap::const_iterator pos = remote_datawriter_crypto_handles_.find(id);
  if (pos != remote_datawriter_crypto_handles_.end()) {
    return pos->second.first;
  }
  return DDS::HANDLE_NIL;
}

DDS::Security::EndpointSecurityAttributes
HandleRegistry::get_remote_datawriter_security_attributes(const DCPS::RepoId& id) const
{
  ACE_GUARD_RETURN(ACE_Thread_Mutex, guard, mutex_, DDS::Security::EndpointSecurityAttributes());
  DatawriterCryptoHandleMap::const_iterator pos = remote_datawriter_crypto_handles_.find(id);
  if (pos != remote_datawriter_crypto_handles_.end()) {
    return pos->second.second;
  }
  return DDS::Security::EndpointSecurityAttributes();
}

HandleRegistry::DatawriterCryptoHandleList
HandleRegistry::get_all_remote_datawriters(const DCPS::RepoId& prefix) const
{
  const DCPS::GuidPrefixEqual guid_prefix_equal;
  DatawriterCryptoHandleList retval;
  ACE_GUARD_RETURN(ACE_Thread_Mutex, guard, mutex_, retval);
  for (DatawriterCryptoHandleMap::const_iterator pos =
         remote_datawriter_crypto_handles_.lower_bound(DCPS::make_id(prefix, DCPS::ENTITYID_UNKNOWN)),
         limit = remote_datawriter_crypto_handles_.end();
       pos != limit && guid_prefix_equal(pos->first, prefix); ++pos) {
    retval.push_back(std::make_pair(pos->first, pos->second.first));
  }
  return retval;
}

void
HandleRegistry::erase_remote_datawriter_crypto_handle(const DCPS::RepoId& id)
{
  ACE_GUARD(ACE_Thread_Mutex, guard, mutex_);
  remote_datawriter_crypto_handles_.erase(id);

  if (DCPS::security_debug.bookkeeping) {
    ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) {bookkeeping} ")
               ACE_TEXT("HandleRegistry::erase_remote_datawriter_crypto_handle %C (total %B)\n"),
               DCPS::LogGuid(id).c_str(),
               remote_datawriter_crypto_handles_.size()));
  }
}

} // Security
} // OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif // OPENDDS_SECURITY
