#include "dds/DCPS/security/HandleRegistryImpl.h"

#include "dds/DCPS/GuidConverter.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace Security {

HandleRegistryImpl::~HandleRegistryImpl()
{
  if (DCPS::security_debug.bookkeeping) {
    ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) {bookkeeping} ")
               ACE_TEXT("HandleRegistryImpl::~HandleRegistryImpl local datareader %d local datawriter %d remote participant %d remote datareader %d remote datawriter %d\n"),
               local_datareader_crypto_handles_.size(),
               local_datawriter_crypto_handles_.size(),
               remote_participant_crypto_handles_.size(),
               remote_datareader_crypto_handles_.size(),
               remote_datawriter_crypto_handles_.size()));
  }
}

void
HandleRegistryImpl::insert_local_datareader_crypto_handle(const DCPS::RepoId& id,
                                                          DDS::Security::DatareaderCryptoHandle handle,
                                                          DDS::Security::EndpointSecurityAttributes attributes)
{
  if (handle != DDS::HANDLE_NIL) {
    ACE_GUARD(ACE_Thread_Mutex, guard, mutex_);
    local_datareader_crypto_handles_[id] = std::make_pair(handle, attributes);

    if (DCPS::security_debug.bookkeeping) {
      ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) {bookkeeping} ")
                 ACE_TEXT("HandleRegistryImpl::insert_local_datareader_crypto_handle %C %d (%d)\n"),
                 DCPS::LogGuid(id).c_str(),
                 handle,
                 local_datareader_crypto_handles_.size()));
    }
  }
}

DDS::Security::DatareaderCryptoHandle
HandleRegistryImpl::get_local_datareader_crypto_handle(const DCPS::RepoId& id) const
{
  ACE_GUARD_RETURN(ACE_Thread_Mutex, guard, mutex_, DDS::HANDLE_NIL);
  DatareaderCryptoHandleMap::const_iterator pos = local_datareader_crypto_handles_.find(id);
  if (pos != local_datareader_crypto_handles_.end()) {
    return pos->second.first;
  }
  return DDS::HANDLE_NIL;
}

DDS::Security::EndpointSecurityAttributes
HandleRegistryImpl::get_local_datareader_security_attributes(const DCPS::RepoId& id) const
{
  ACE_GUARD_RETURN(ACE_Thread_Mutex, guard, mutex_, DDS::Security::EndpointSecurityAttributes());
  DatareaderCryptoHandleMap::const_iterator pos = local_datareader_crypto_handles_.find(id);
  if (pos != local_datareader_crypto_handles_.end()) {
    return pos->second.second;
  }
  return DDS::Security::EndpointSecurityAttributes();
}

void
HandleRegistryImpl::remove_local_datareader_crypto_handle(const DCPS::RepoId& id)
{
  ACE_GUARD(ACE_Thread_Mutex, guard, mutex_);
  local_datareader_crypto_handles_.erase(id);

  if (DCPS::security_debug.bookkeeping) {
    ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) {bookkeeping} ")
               ACE_TEXT("HandleRegistryImpl::remove_local_datareader_crypto_handle %C (%d)\n"),
               DCPS::LogGuid(id).c_str(),
               local_datareader_crypto_handles_.size()));
  }
}

void
HandleRegistryImpl::insert_local_datawriter_crypto_handle(const DCPS::RepoId& id,
                                                          DDS::Security::DatawriterCryptoHandle handle,
                                                          DDS::Security::EndpointSecurityAttributes attributes)
{
  if (handle != DDS::HANDLE_NIL) {
    ACE_GUARD(ACE_Thread_Mutex, guard, mutex_);
    local_datawriter_crypto_handles_[id] = std::make_pair(handle, attributes);

    if (DCPS::security_debug.bookkeeping) {
      ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) {bookkeeping} ")
                 ACE_TEXT("HandleRegistryImpl::insert_local_datawriter_crypto_handle %C %d (%d)\n"),
                 DCPS::LogGuid(id).c_str(),
                 handle,
                 local_datawriter_crypto_handles_.size()));
    }
  }
}

DDS::Security::DatawriterCryptoHandle
HandleRegistryImpl::get_local_datawriter_crypto_handle(const DCPS::RepoId& id) const
{
  ACE_GUARD_RETURN(ACE_Thread_Mutex, guard, mutex_, DDS::HANDLE_NIL);
  DatawriterCryptoHandleMap::const_iterator pos = local_datawriter_crypto_handles_.find(id);
  if (pos != local_datawriter_crypto_handles_.end()) {
    return pos->second.first;
  }
  return DDS::HANDLE_NIL;
}

DDS::Security::EndpointSecurityAttributes
HandleRegistryImpl::get_local_datawriter_security_attributes(const DCPS::RepoId& id) const
{
  ACE_GUARD_RETURN(ACE_Thread_Mutex, guard, mutex_, DDS::Security::EndpointSecurityAttributes());
  DatawriterCryptoHandleMap::const_iterator pos = local_datawriter_crypto_handles_.find(id);
  if (pos != local_datawriter_crypto_handles_.end()) {
    return pos->second.second;
  }
  return DDS::Security::EndpointSecurityAttributes();
}

void
HandleRegistryImpl::remove_local_datawriter_crypto_handle(const DCPS::RepoId& id)
{
  ACE_GUARD(ACE_Thread_Mutex, guard, mutex_);
  local_datawriter_crypto_handles_.erase(id);

  if (DCPS::security_debug.bookkeeping) {
    ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) {bookkeeping} ")
               ACE_TEXT("HandleRegistryImpl::remove_local_datawriter_crypto_handle %C (%d)\n"),
               DCPS::LogGuid(id).c_str(),
               local_datawriter_crypto_handles_.size()));
  }
}

void
HandleRegistryImpl::insert_remote_participant_crypto_handle(const DCPS::RepoId& id,
                                                            DDS::Security::ParticipantCryptoHandle handle)
{
  if (handle != DDS::HANDLE_NIL) {
    ACE_GUARD(ACE_Thread_Mutex, guard, mutex_);
    remote_participant_crypto_handles_[id] = handle;

    if (DCPS::security_debug.bookkeeping) {
      ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) {bookkeeping} ")
                 ACE_TEXT("HandleRegistryImpl::insert_remote_participant_crypto_handle %C %d (%d)\n"),
                 DCPS::LogGuid(id).c_str(),
                 handle,
                 remote_participant_crypto_handles_.size()));
    }
  }
}

DDS::Security::ParticipantCryptoHandle
HandleRegistryImpl::get_remote_participant_crypto_handle(const DCPS::RepoId& id) const
{
  ACE_GUARD_RETURN(ACE_Thread_Mutex, guard, mutex_, DDS::HANDLE_NIL);
  ParticipantCryptoHandleMap::const_iterator pos = remote_participant_crypto_handles_.find(id);
  if (pos != remote_participant_crypto_handles_.end()) {
    return pos->second;
  }
  return DDS::HANDLE_NIL;
}

void
HandleRegistryImpl::remove_remote_participant_crypto_handle(const DCPS::RepoId& id)
{
  ACE_GUARD(ACE_Thread_Mutex, guard, mutex_);
  remote_participant_crypto_handles_.erase(id);

  if (DCPS::security_debug.bookkeeping) {
    ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) {bookkeeping} ")
               ACE_TEXT("HandleRegistryImpl::remove_remote_participant_crypto_handle %C (%d)\n"),
               DCPS::LogGuid(id).c_str(),
               remote_participant_crypto_handles_.size()));
  }
}

void
HandleRegistryImpl::insert_remote_datareader_crypto_handle(const DCPS::RepoId& id,
                                                           DDS::Security::DatareaderCryptoHandle handle,
                                                           DDS::Security::EndpointSecurityAttributes attributes)
{
  if (handle != DDS::HANDLE_NIL) {
    ACE_GUARD(ACE_Thread_Mutex, guard, mutex_);
    remote_datareader_crypto_handles_[id] = std::make_pair(handle, attributes);
  }

  if (DCPS::security_debug.bookkeeping) {
    ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) {bookkeeping} ")
               ACE_TEXT("HandleRegistryImpl::insert_remote_datareader_crypto_handle %C %d (%d)\n"),
               DCPS::LogGuid(id).c_str(),
               handle,
               remote_datareader_crypto_handles_.size()));
  }
}

DDS::Security::DatareaderCryptoHandle
HandleRegistryImpl::get_remote_datareader_crypto_handle(const DCPS::RepoId& id) const
{
  ACE_GUARD_RETURN(ACE_Thread_Mutex, guard, mutex_, DDS::HANDLE_NIL);
  DatareaderCryptoHandleMap::const_iterator pos = remote_datareader_crypto_handles_.find(id);
  if (pos != remote_datareader_crypto_handles_.end()) {
    return pos->second.first;
  }
  return DDS::HANDLE_NIL;
}

DDS::Security::EndpointSecurityAttributes
HandleRegistryImpl::get_remote_datareader_security_attributes(const DCPS::RepoId& id) const
{
  ACE_GUARD_RETURN(ACE_Thread_Mutex, guard, mutex_, DDS::Security::EndpointSecurityAttributes());
  DatareaderCryptoHandleMap::const_iterator pos = remote_datareader_crypto_handles_.find(id);
  if (pos != remote_datareader_crypto_handles_.end()) {
    return pos->second.second;
  }
  return DDS::Security::EndpointSecurityAttributes();
}

HandleRegistry::DatareaderCryptoHandleList
HandleRegistryImpl::get_all_remote_datareaders(const DCPS::RepoId& prefix) const
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
HandleRegistryImpl::remove_remote_datareader_crypto_handle(const DCPS::RepoId& id)
{
  ACE_GUARD(ACE_Thread_Mutex, guard, mutex_);
  remote_datareader_crypto_handles_.erase(id);

  if (DCPS::security_debug.bookkeeping) {
    ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) {bookkeeping} ")
               ACE_TEXT("HandleRegistryImpl::remove_remote_datareader_crypto_handle %C (%d)\n"),
               DCPS::LogGuid(id).c_str(),
               remote_datareader_crypto_handles_.size()));
  }
}

void
HandleRegistryImpl::insert_remote_datawriter_crypto_handle(const DCPS::RepoId& id,
                                                           DDS::Security::DatawriterCryptoHandle handle,
                                                           DDS::Security::EndpointSecurityAttributes attributes)
{
  OPENDDS_ASSERT(id.entityId != DCPS::ENTITYID_UNKNOWN);
  if (handle != DDS::HANDLE_NIL) {
    ACE_GUARD(ACE_Thread_Mutex, guard, mutex_);
    remote_datawriter_crypto_handles_[id] = std::make_pair(handle, attributes);

    if (DCPS::security_debug.bookkeeping) {
      ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) {bookkeeping} ")
                 ACE_TEXT("HandleRegistryImpl::insert_remote_datawriter_crypto_handle %C %d (%d)\n"),
                 DCPS::LogGuid(id).c_str(),
                 handle,
                 remote_datawriter_crypto_handles_.size()));
    }
  }
}

DDS::Security::DatawriterCryptoHandle
HandleRegistryImpl::get_remote_datawriter_crypto_handle(const DCPS::RepoId& id) const
{
  ACE_GUARD_RETURN(ACE_Thread_Mutex, guard, mutex_, DDS::HANDLE_NIL);
  DatawriterCryptoHandleMap::const_iterator pos = remote_datawriter_crypto_handles_.find(id);
  if (pos != remote_datawriter_crypto_handles_.end()) {
    return pos->second.first;
  }
  return DDS::HANDLE_NIL;
}

DDS::Security::EndpointSecurityAttributes
HandleRegistryImpl::get_remote_datawriter_security_attributes(const DCPS::RepoId& id) const
{
  ACE_GUARD_RETURN(ACE_Thread_Mutex, guard, mutex_, DDS::Security::EndpointSecurityAttributes());
  DatawriterCryptoHandleMap::const_iterator pos = remote_datawriter_crypto_handles_.find(id);
  if (pos != remote_datawriter_crypto_handles_.end()) {
    return pos->second.second;
  }
  return DDS::Security::EndpointSecurityAttributes();
}

HandleRegistryImpl::DatawriterCryptoHandleList
HandleRegistryImpl::get_all_remote_datawriters(const DCPS::RepoId& prefix) const
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
HandleRegistryImpl::remove_remote_datawriter_crypto_handle(const DCPS::RepoId& id)
{
  ACE_GUARD(ACE_Thread_Mutex, guard, mutex_);
  remote_datawriter_crypto_handles_.erase(id);

  if (DCPS::security_debug.bookkeeping) {
    ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) {bookkeeping} ")
               ACE_TEXT("HandleRegistryImpl::remove_remote_datawriter_crypto_handle %C (%d)\n"),
               DCPS::LogGuid(id).c_str(),
               remote_datawriter_crypto_handles_.size()));
  }
}

} // Security
} // OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL
