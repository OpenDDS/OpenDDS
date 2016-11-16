/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "DCPS/DdsDcps_pch.h" //Only the _pch include should start with DCPS/

#ifndef OPENDDS_NO_OWNERSHIP_KIND_EXCLUSIVE

#include "OwnershipManager.h"
#include "GuidConverter.h"
#include "Util.h"
#include "DataReaderImpl.h"
#include <algorithm>

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

namespace Util {

bool DescendingOwnershipStrengthSort(const OwnershipManager::WriterInfo& w1,
                                     const OwnershipManager::WriterInfo& w2)
{
  return w1.ownership_strength_ > w2.ownership_strength_;
}

} // namespace Util

OwnershipManager::OwnershipManager()
{
}

OwnershipManager::~OwnershipManager()
{
  // The type->instance should be empty if unregister instance are performed
  // by all readers, but in case the instance not unregistered for some reason,
  // an error will be logged.
  if (!type_instance_map_.empty()) {
    // There is no way to pass the instance map to concrete datareader
    // to delete, so it will be leaked.
    ACE_DEBUG((LM_WARNING,
               ACE_TEXT("(%P|%t) OwnershipManager::~OwnershipManager ")
               ACE_TEXT("- non-empty type_instance_map_\n")));
  }
}

int
OwnershipManager::instance_lock_acquire()
{
  return instance_lock_.acquire();
}

int
OwnershipManager::instance_lock_release()
{
  return instance_lock_.release();
}

void*
OwnershipManager::get_instance_map(const char* type_name,
                                   DataReaderImpl* reader)
{
  InstanceMap* instance = 0;
  if (0 != find(type_instance_map_, type_name, instance)) {
    return 0;
  }

  instance->readers_.insert(reader);
  return instance->map_;
}

void
OwnershipManager::set_instance_map(const char* type_name,
                                   void* instance_map,
                                   DataReaderImpl* reader)
{
  if (DCPS_debug_level >= 1) {
    ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) OwnershipManager::set_instance_map ")
               ACE_TEXT("instance map %X is created by reader %X \n"),
               instance_map, reader));
  }

  if (0 != OpenDDS::DCPS::bind(type_instance_map_, type_name,
                               InstanceMap(instance_map, reader))) {
    ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: OwnershipManager::set_instance_map "
               "failed to bind instance for type \"%C\"\n", type_name));
  }
}

void
OwnershipManager::unregister_reader(const char* type_name,
                                    DataReaderImpl* reader)
{
  ACE_GUARD(ACE_Thread_Mutex, guard, instance_lock_);

  InstanceMap* instance = 0;
  if (0 != find(type_instance_map_, type_name, instance)) {
    return;
  }

  instance->readers_.erase(reader);

  if (instance->readers_.empty()) {
    if (DCPS_debug_level >= 1) {
      ACE_DEBUG((LM_DEBUG,
                 ACE_TEXT("(%P|%t) OwnershipManager::unregister_reader ")
                 ACE_TEXT(" instance map %@ is deleted by reader %@\n"),
                 instance->map_, reader));
    }
    reader->delete_instance_map(instance->map_);
    unbind(type_instance_map_, type_name);
  }
}

void
OwnershipManager::remove_writer(const PublicationId& pub_id)
{
  ACE_GUARD(ACE_Thread_Mutex, guard, instance_lock_);

  const InstanceOwnershipWriterInfos::iterator the_end =
    instance_ownership_infos_.end();
  for (InstanceOwnershipWriterInfos::iterator iter =
         instance_ownership_infos_.begin(); iter != the_end; ++iter) {
    remove_writer(iter->first, iter->second, pub_id);
  }
}

void
OwnershipManager::remove_instance(InstanceState* instance_state)
{
  ACE_GUARD(ACE_Thread_Mutex, guard, instance_lock_);
  const DDS::InstanceHandle_t ih = instance_state->instance_handle();
  InstanceOwnershipWriterInfos::iterator i = instance_ownership_infos_.find(ih);
  if (i != instance_ownership_infos_.end()) {
    InstanceStateVec& states = i->second.instance_states_;
    for (size_t j = 0; j < states.size(); ++j) {
      if (states[j] == instance_state) {
        states.erase(states.begin() + j);
        break;
      }
    }
  }
}

void
OwnershipManager::remove_writers(const DDS::InstanceHandle_t& instance_handle)
{
  InstanceStateVec instances_to_reset;
  {
    ACE_GUARD(ACE_Thread_Mutex, guard, instance_lock_);

    if (DCPS_debug_level >= 1) {
      ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) OwnershipManager::remove_writers:")
                 ACE_TEXT(" disassociate writers with instance %d\n"),
                 instance_handle));
    }

    InstanceOwnershipWriterInfos::iterator owner_wi =
      instance_ownership_infos_.find(instance_handle);
    if (owner_wi != instance_ownership_infos_.end()) {
      owner_wi->second.owner_ = WriterInfo();
      owner_wi->second.candidates_.clear();
      const InstanceStateVec::iterator end =
        owner_wi->second.instance_states_.end();
      for (InstanceStateVec::iterator iter =
             owner_wi->second.instance_states_.begin(); iter != end; ++iter) {
        // call after lock released, will call back to data reader
        instances_to_reset.push_back(*iter);
      }
      owner_wi->second.instance_states_.clear();

      instance_ownership_infos_.erase(owner_wi);
    }
  }
  // Lock released
  for (InstanceStateVec::iterator instance = instances_to_reset.begin();
       instance != instances_to_reset.end(); ++instance) {
    (*instance)->reset_ownership(instance_handle);
  }
}


bool
OwnershipManager::is_owner(const DDS::InstanceHandle_t& instance_handle,
                           const PublicationId& pub_id)
{
  ACE_GUARD_RETURN(ACE_Thread_Mutex, guard, instance_lock_, false);

  InstanceOwnershipWriterInfos::iterator iter
    = instance_ownership_infos_.find(instance_handle);
  if (iter != instance_ownership_infos_.end()) {
    return iter->second.owner_.pub_id_ == pub_id;
  }

  return false;
}


bool // owner unregister instance
OwnershipManager::remove_writer(const DDS::InstanceHandle_t& instance_handle,
                                const PublicationId& pub_id)
{
  ACE_GUARD_RETURN(ACE_Thread_Mutex, guard, instance_lock_, false);

  InstanceOwnershipWriterInfos::iterator the_iter =
    instance_ownership_infos_.find(instance_handle);
  if (the_iter != instance_ownership_infos_.end()) {
    return remove_writer(instance_handle, the_iter->second, pub_id);
  }

  return false;
}

bool
OwnershipManager::remove_writer(const DDS::InstanceHandle_t& instance_handle,
                                OwnershipWriterInfos& infos,
                                const PublicationId& pub_id)
{
  if (infos.owner_.pub_id_ == pub_id) {
    remove_owner(instance_handle, infos, false);
    return true;

  } else {
    remove_candidate(infos, pub_id);
    return false;
  }
}


void
OwnershipManager::remove_owner(const DDS::InstanceHandle_t& instance_handle,
                               OwnershipWriterInfos& infos,
                               bool sort)
{
  //change owner
  PublicationId new_owner(GUID_UNKNOWN);
  if (infos.candidates_.empty()) {
    infos.owner_ = WriterInfo();

  } else {
    if (sort) {
      std::sort(infos.candidates_.begin(), infos.candidates_.end(),
                Util::DescendingOwnershipStrengthSort);
    }

    const WriterInfos::iterator begin = infos.candidates_.begin();
    infos.owner_ = *begin;
    infos.candidates_.erase(begin);
    new_owner = infos.owner_.pub_id_;
  }

  broadcast_new_owner(instance_handle, infos, new_owner);
}


void
OwnershipManager::remove_candidate(OwnershipWriterInfos& infos,
                                   const PublicationId& pub_id)
{
  if (!infos.candidates_.empty()) {
    WriterInfos::iterator const the_end = infos.candidates_.end();

    WriterInfos::iterator found_candidate = the_end;
    // Supplied writer is not an owner, check if it exists in candicate list.
    // If not, add it to the candidate list and sort the list.
    for (WriterInfos::iterator iter = infos.candidates_.begin();
         iter != the_end; ++iter) {
      if (iter->pub_id_ == pub_id) {
        found_candidate = iter;
        break;
      }
    }

    if (found_candidate != the_end) {
      infos.candidates_.erase(found_candidate);
    }
  }
}

bool
OwnershipManager::select_owner(const DDS::InstanceHandle_t& instance_handle,
                               const PublicationId& pub_id,
                               const CORBA::Long& ownership_strength,
                               InstanceState* instance_state)
{
  ACE_GUARD_RETURN(ACE_Thread_Mutex, guard, instance_lock_, false);

  InstanceOwnershipWriterInfos::iterator iter =
    instance_ownership_infos_.find(instance_handle);
  if (iter != instance_ownership_infos_.end()) {
    OwnershipWriterInfos& infos = iter->second;
    if (!instance_state->registered()) {
      infos.instance_states_.push_back(instance_state);
      instance_state->registered(true);
    }

    // No owner at some point.
    if (infos.owner_.pub_id_ == GUID_UNKNOWN) {
      infos.owner_ = WriterInfo(pub_id, ownership_strength);
      broadcast_new_owner(instance_handle, infos, pub_id);
      return true;

    } else if (infos.owner_.pub_id_ == pub_id) { // is current owner
      //still owner but strength changed to be bigger..
      if (infos.owner_.ownership_strength_ <= ownership_strength) {
        infos.owner_.ownership_strength_ = ownership_strength;
        return true;

      } else { //update strength and reevaluate owner which broadcast new owner.
        infos.candidates_.push_back(WriterInfo(pub_id, ownership_strength));
        remove_owner(instance_handle, infos, true);
        return infos.owner_.pub_id_ == pub_id;
      }

    } else { // not current owner, reevaluate the owner
      bool replace_owner = false;
      // Add current owner to candidate list for owner reevaluation
      // if provided pub has strength greater than currrent owner.
      if (ownership_strength > infos.owner_.ownership_strength_) {
        infos.candidates_.push_back(infos.owner_);
        replace_owner = true;
      }

      bool found = false;
      bool sort = true;

      // check if it already existed in candicate list. If not,
      // add it to the candidate list, otherwise update strength
      // if strength was changed.
      const WriterInfos::iterator the_end = infos.candidates_.end();

      for (WriterInfos::iterator iter = infos.candidates_.begin();
           iter != the_end; ++iter) {

        if (iter->pub_id_ == pub_id) {
          if (iter->ownership_strength_ != ownership_strength) {
            iter->ownership_strength_ = ownership_strength;
          } else {
            sort = false;
          }
          found = true;
          break;
        }
      }

      if (!found) {
        infos.candidates_.push_back(WriterInfo(pub_id, ownership_strength));
      }

      if (sort) {
        std::sort(infos.candidates_.begin(), infos.candidates_.end(),
                  Util::DescendingOwnershipStrengthSort);
      }

      if (replace_owner) {
        // Owner was already moved to candidate list and the list was sorted
        // already so pick owner from sorted list and replace current
        // owner.
        remove_owner(instance_handle, infos, false);
      }

      return infos.owner_.pub_id_ == pub_id;
    }

  } else {
    // first writer of the instance so it's owner.
    OwnershipWriterInfos& infos = instance_ownership_infos_[instance_handle];
    infos.owner_ = WriterInfo(pub_id, ownership_strength);
    if (!instance_state->registered()) {
      infos.instance_states_.push_back(instance_state);
      instance_state->registered(true);
    }
    broadcast_new_owner(instance_handle, infos, infos.owner_.pub_id_);
    return true;
  }

  return false;
}


void
OwnershipManager::broadcast_new_owner(const DDS::InstanceHandle_t& instance_handle,
                                      OwnershipWriterInfos& infos,
                                      const PublicationId& owner)
{
  if (DCPS_debug_level >= 1) {
    // This may not be an error since it could happen that the sample
    // is delivered to the datareader after the write is dis-associated
    // with this datareader.
    GuidConverter writer_converter(owner);
    ACE_DEBUG((LM_DEBUG,
               ACE_TEXT("(%P|%t) OwnershipManager::broadcast_new_owner: ")
               ACE_TEXT("owner writer %C, instance handle %d strength %d num ")
               ACE_TEXT("of candidates %d\n"),
               OPENDDS_STRING(writer_converter).c_str(), instance_handle,
               infos.owner_.ownership_strength_,
               (int)infos.candidates_.size()));
  }

  const InstanceStateVec::iterator the_end = infos.instance_states_.end();
  for (InstanceStateVec::iterator iter = infos.instance_states_.begin();
       iter != the_end; ++iter) {
    (*iter)->set_owner(owner);
  }
}

void
OwnershipManager::remove_owner(const DDS::InstanceHandle_t& instance_handle)
{
  ACE_GUARD(ACE_Thread_Mutex, guard, instance_lock_);

  const InstanceOwnershipWriterInfos::iterator iter =
    instance_ownership_infos_.find(instance_handle);

  if (iter != instance_ownership_infos_.end()) {
    remove_owner(instance_handle, iter->second, false);
  }
}


} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif // OPENDDS_NO_OWNERSHIP_KIND_EXCLUSIVE
