/*
 * $Id$
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "DCPS/DdsDcps_pch.h" //Only the _pch include should start with DCPS/
#include "OwnershipManager.h"
#include "GuidConverter.h"
#include "Util.h"
#include "DataReaderImpl.h"
#include <algorithm>


namespace Util {

bool DescendingOwnershipStrengthSort (const OpenDDS::DCPS::OwnershipManager::WriterInfo& w1, const OpenDDS::DCPS::OwnershipManager::WriterInfo& w2)
{
  return w1.ownership_strength_ > w2.ownership_strength_;
};

} // namespace Util

namespace OpenDDS {
namespace DCPS {

//TBD - add check for enabled in most methods.
//      Currently this is not needed because auto_enable_created_entities
//      cannot be false.

// Implementation skeleton constructor
OwnershipManager::OwnershipManager()
{
}

// Implementation skeleton destructor
OwnershipManager::~OwnershipManager()
{
  // The type->instance should be empty if unregister instance are performed
  // by all readers, but in case the instance not unregistered for some reason,
  // an error will be logged.
  TypeInstanceMap::iterator const the_end = type_instance_map_.end ();
  TypeInstanceMap::iterator iter = type_instance_map_.begin ();
  while (iter != the_end)
  {
    // There is no way to pass the instance map to concrete datareader
    // to delete, so it will be leaked.
    // delete iter->second.map_;
    ++ iter;
  }

  type_instance_map_.clear ();
}


int
OwnershipManager::instance_lock_acquire ()
{
  return this->instance_lock_.acquire ();
}

int
OwnershipManager::instance_lock_release ()
{
  return this->instance_lock_.release ();
}

void*
OwnershipManager::get_instance_map (const char* type_name,
                                         DataReaderImpl* reader)
{
  InstanceMap* instance = 0;
  if (0 != find(type_instance_map_, type_name, instance)) {
    return 0;
  }

  instance->readers_.push_back (reader);
  return instance->map_;
}

void
OwnershipManager::set_instance_map (const char* type_name,
                                    void* instance_map,
                                    DataReaderImpl* reader)
{
  if (DCPS_debug_level >= 1) {
    ACE_DEBUG ((LM_DEBUG, ACE_TEXT ("(%P|%t) OwnershipManager::set_instance_map ")
                          ACE_TEXT (" instance map %X is created by reader %X \n"),
                instance_map, reader));
  }

  if (0 != OpenDDS::DCPS::bind(type_instance_map_, type_name, InstanceMap (instance_map, reader))) {
    ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: OwnershipManager::set_instance_map failed to "
                         "bind instance for type \"%s\"\n",type_name));
  }
}

void
OwnershipManager::unregister_reader (const char* type_name,
                                     DataReaderImpl* reader)
{
  ACE_GUARD(ACE_Thread_Mutex,
            guard,
            this->instance_lock_);

  InstanceMap* instance = 0;
  if (0 != find(type_instance_map_, type_name, instance)) {
    return;
  }

  ReaderVec::iterator end = instance->readers_.end();

  for (ReaderVec::iterator it(instance->readers_.begin());
      it != end; ++it) {
    if (*it == reader) {
      instance->readers_.erase (it);
      break;
    }
  }

  if (instance->readers_.empty ()) {
    if (DCPS_debug_level >= 1) {
      ACE_DEBUG ((LM_DEBUG, ACE_TEXT ("(%P|%t) OwnershipManager::unregister_reader ")
                            ACE_TEXT (" instance map %X is deleted by reader %X \n"),
                  instance->map_, reader));
    }
    reader->delete_instance_map (instance->map_);
    unbind (type_instance_map_, type_name);
  }
}

void
OwnershipManager::remove_writer (const PublicationId& pub_id)
{
  ACE_GUARD(ACE_Thread_Mutex,
            guard,
            this->instance_lock_);

  InstanceOwnershipWriterInfos::iterator const the_end = instance_ownership_infos_.end ();
  for (InstanceOwnershipWriterInfos::iterator iter = instance_ownership_infos_.begin ();
        iter != the_end; ++ iter) {
    this->remove_writer (iter->first, iter->second, pub_id);
  }
}


void
OwnershipManager::remove_writers (const ::DDS::InstanceHandle_t& instance_handle)
{
  ACE_GUARD(ACE_Thread_Mutex,
            guard,
            this->instance_lock_);

  if (DCPS_debug_level >= 1) {
      ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) OwnershipManager::remove_writers: ")
                           ACE_TEXT("disassociate writers with instance %d\n"),
                           instance_handle));
  }

  InstanceOwnershipWriterInfos::iterator const the_end = instance_ownership_infos_.end ();

  InstanceOwnershipWriterInfos::iterator the_iter
    = instance_ownership_infos_.find (instance_handle);
  if (the_iter != the_end) {
    the_iter->second.owner_ = WriterInfo();
    the_iter->second.candidates_.clear ();
    InstanceStateVec::iterator const end = the_iter->second.instance_states_.end();
    for (InstanceStateVec::iterator iter = the_iter->second.instance_states_.begin ();
      iter != end; ++iter) {
        (*iter)->reset_ownership(instance_handle);
    }
    the_iter->second.instance_states_.clear ();

    instance_ownership_infos_.erase (the_iter);
  }
}


bool
OwnershipManager::is_owner (const ::DDS::InstanceHandle_t& instance_handle,
                                 const PublicationId& pub_id)
{
  ACE_GUARD_RETURN (ACE_Thread_Mutex,
                    guard,
                    this->instance_lock_,
                    false);

  InstanceOwnershipWriterInfos::iterator const the_end = instance_ownership_infos_.end ();

  InstanceOwnershipWriterInfos::iterator iter
    = instance_ownership_infos_.find (instance_handle);
  if (iter != the_end) {
    return iter->second.owner_.pub_id_ == pub_id;
  }

  return false;
}


bool // owner unregister instance
OwnershipManager::remove_writer (
                                 const ::DDS::InstanceHandle_t& instance_handle,
                                 const PublicationId& pub_id)
{
  ACE_GUARD_RETURN (ACE_Thread_Mutex,
                    guard,
                    this->instance_lock_,
                    false);

  InstanceOwnershipWriterInfos::iterator const the_end = instance_ownership_infos_.end ();

  InstanceOwnershipWriterInfos::iterator the_iter
    = instance_ownership_infos_.find (instance_handle);
  if (the_iter != the_end) {
    return this->remove_writer (instance_handle, the_iter->second, pub_id);
  }

  return false;
}

bool
OwnershipManager::remove_writer (const ::DDS::InstanceHandle_t& instance_handle,
                                 OwnershipWriterInfos& infos,
                                 const PublicationId& pub_id)
{
  if (infos.owner_.pub_id_ == pub_id) {
    this->remove_owner (instance_handle, infos, false);
    return true;
  }
  else {
    this->remove_candidate (infos, pub_id);
    return false;
  }

  return false;
}


void
OwnershipManager::remove_owner (const ::DDS::InstanceHandle_t& instance_handle,
                                OwnershipWriterInfos& infos,
                                bool sort)
{
  //change owner
  PublicationId new_owner(GUID_UNKNOWN);
  if (infos.candidates_.empty ()) {
    infos.owner_ = WriterInfo();
  }
  else {
    if (sort) {
      std::sort (infos.candidates_.begin(), infos.candidates_.end(),
                 ::Util::DescendingOwnershipStrengthSort);
    }

    WriterInfos::iterator begin = infos.candidates_.begin();
    infos.owner_ = *begin;
    infos.candidates_.erase (begin);
    new_owner = infos.owner_.pub_id_;
  }

  this->broadcast_new_owner (instance_handle, infos, new_owner);
}


void
OwnershipManager::remove_candidate (OwnershipWriterInfos& infos,const PublicationId& pub_id)
{
  if (! infos.candidates_.empty ()) {
    WriterInfos::iterator const the_end = infos.candidates_.end();

    WriterInfos::iterator found_candidate = the_end;
    // Supplied writer is not an owner, check if it exists in candicate list.If not,
    // add it to the candidate list and sort the list.
    for (WriterInfos::iterator iter = infos.candidates_.begin ();
      iter != the_end; ++iter) {

      if (iter->pub_id_ == pub_id) {
        found_candidate = iter;
        break;
      }
    }

    if (found_candidate != the_end) {
      infos.candidates_.erase (found_candidate);
    }
  }
}

bool
OwnershipManager::select_owner (const ::DDS::InstanceHandle_t& instance_handle,
                                     const PublicationId& pub_id,
                                     const CORBA::Long& ownership_strength,
                                     InstanceState* instance_state)
{
  ACE_GUARD_RETURN (ACE_Thread_Mutex,
                    guard,
                    this->instance_lock_,
                    false);

  InstanceOwnershipWriterInfos::iterator const the_end = instance_ownership_infos_.end ();

  InstanceOwnershipWriterInfos::iterator iter
    = instance_ownership_infos_.find (instance_handle);
  if (iter != the_end) {
    OwnershipWriterInfos& infos = iter->second;
    if (!instance_state->registered()) {
      infos.instance_states_.push_back (instance_state);
      instance_state->registered(true);
    }

    // No owner at some point.
    if (infos.owner_.pub_id_ == GUID_UNKNOWN) {
      infos.owner_ = WriterInfo(pub_id,ownership_strength);
      this->broadcast_new_owner (instance_handle, infos, pub_id);

      return true;
    }
    else if (infos.owner_.pub_id_ == pub_id) { // is current owner
      //still owner but strength changed to be bigger..
      if (infos.owner_.ownership_strength_ <= ownership_strength) {
        infos.owner_.ownership_strength_ = ownership_strength;
        return true;
      }
      else { //update strength and reevaluate owner which broadcast new owner.
        infos.candidates_.push_back (WriterInfo(pub_id,ownership_strength));
        this->remove_owner (instance_handle, infos, true);
        return infos.owner_.pub_id_ == pub_id;
      }
    }
    else { // not current owner, reevaluate the owner
      bool replace_owner = false;
      // Add current owner to candidate list for owner reevaluation
      // if provided pub has strength greater than currrent owner.
      if (ownership_strength > infos.owner_.ownership_strength_) {
        infos.candidates_.push_back (infos.owner_);
        replace_owner = true;
      }

      bool found = false;
      bool sort = true;

      // check if it already existed in candicate list. If not,
      // add it to the candidate list, otherwise update strength
      // if strength was changed.
      WriterInfos::iterator const the_end = infos.candidates_.end();

      for (WriterInfos::iterator iter = infos.candidates_.begin();
        iter != the_end; ++iter) {

        if (iter->pub_id_ == pub_id) {
          if (iter->ownership_strength_ != ownership_strength) {
            iter->ownership_strength_ = ownership_strength;
          }
          else {
            sort = false;
          }
          found = true;
          break;
        }
      }

      if (!found) {
        infos.candidates_.push_back (WriterInfo(pub_id,ownership_strength));
      }

      if (sort) {
        std::sort (infos.candidates_.begin(), infos.candidates_.end(), ::Util::DescendingOwnershipStrengthSort);
      }

      if (replace_owner) {
        // Owner was already moved to candidate list and the list was sorted
        // already so pick owner from sorted list and replace current
        // owner.
        this->remove_owner (instance_handle, infos, false);
      }

      return infos.owner_.pub_id_ == pub_id;
    }
  }
  else {
    // first writer of the instance so it's owner.
    OwnershipWriterInfos& infos = instance_ownership_infos_[instance_handle];
    infos.owner_ = WriterInfo(pub_id,ownership_strength);
    if (!instance_state->registered()) {
      infos.instance_states_.push_back (instance_state);
      instance_state->registered(true);
    }
    this->broadcast_new_owner (instance_handle, infos, infos.owner_.pub_id_);
    return true;
  }

  return false;
}


void
OwnershipManager::broadcast_new_owner ( const ::DDS::InstanceHandle_t& instance_handle,
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
      ACE_TEXT("owner writer %C, instance handle %d strength %d num of candidates %d\n"),
      std::string(writer_converter).c_str(),
      instance_handle, infos.owner_.ownership_strength_, infos.candidates_.size()));
  }

  InstanceStateVec::iterator const the_end = infos.instance_states_.end();
  for (InstanceStateVec::iterator iter = infos.instance_states_.begin ();
    iter != the_end; ++iter) {
    (*iter)->set_owner (owner);
  }
}

void
OwnershipManager::remove_owner (const ::DDS::InstanceHandle_t& instance_handle)
{
  ACE_GUARD(ACE_Thread_Mutex,
            guard,
            this->instance_lock_);

  InstanceOwnershipWriterInfos::iterator iter
    = instance_ownership_infos_.find (instance_handle);

  this->remove_owner (instance_handle, iter->second, false);
}


} // namespace DCPS
} // namespace OpenDDS
