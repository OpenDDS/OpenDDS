/*
 * $Id$
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_OWNERSHIP_MANAGER_H
#define OPENDDS_DCPS_OWNERSHIP_MANAGER_H

//#include "EntityImpl.h"
#include "Definitions.h"
#include "GuidUtils.h"
#include "dds/DdsDcpsInfrastructureC.h"
#include <map>
#include <string>
#include <vector>

#if !defined (ACE_LACKS_PRAGMA_ONCE)
#pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

namespace OpenDDS {
namespace DCPS {

class InstanceState;
class DataReaderImpl;

class OpenDDS_Dcps_Export OwnershipManager {
public:
  typedef std::vector<DataReaderImpl* > ReaderVec;
  // The TypeInstanceMap is only used for EXCLUSIVE ownership.
  struct InstanceMap {
    InstanceMap () : map_(0) {};
    InstanceMap (void* map, DataReaderImpl* reader)
    : map_(map) {
      readers_.push_back (reader);
    };
    void* map_;
    ReaderVec readers_;
  };

  typedef std::map<std::string, InstanceMap> TypeInstanceMap;

  struct WriterInfo {
    WriterInfo (const PublicationId& pub_id,
                const CORBA::Long& ownership_strength)
                         : pub_id_ (pub_id),
                           ownership_strength_ (ownership_strength)
                           {};
    WriterInfo ()
                      : pub_id_ (GUID_UNKNOWN),
                        ownership_strength_ (0)
                        {};

    PublicationId pub_id_;
    CORBA::Long ownership_strength_;
  };

  typedef std::vector <WriterInfo> WriterInfos;
  typedef std::vector <InstanceState* > InstanceStateVec;

  struct OwnershipWriterInfos {
    WriterInfo owner_;
    WriterInfos candidates_;
    InstanceStateVec instance_states_;
  };

  typedef std::map < ::DDS::InstanceHandle_t, OwnershipWriterInfos> InstanceOwnershipWriterInfos;

  OwnershipManager ();
  ~OwnershipManager ();

  /**
  * Acquire/release lock for type instance map.
  * The following functions are synchnorized by instance_lock_.
  */
  int instance_lock_acquire ();
  int instance_lock_release ();

  /**
  * The instance map per type is created by the concrete datareader
  * when first sample with the type is received.
  */
  void set_instance_map (const char* type_name,
                         void* instance_map,
                         DataReaderImpl* reader);

  /**
  * Accesor of the instance map for provided type. It is called once
  * for each new instance in a datareader.
  */
  void* get_instance_map (const char* type_name, DataReaderImpl* reader);

  /**
  * The readers that access the instance map are keep tracked as ref
  * counting to the instance map. The readers need unregister itself
  * with the instance map upon unregistering instance.The instance map
  * is deleted upon the last reader unregistering an instance of the
  * type.
  */
  void  unregister_reader (const char* type_name,
                           DataReaderImpl* reader);

  /**
  * Remove a writer from all instances ownership collection.
  */
  void remove_writer (const PublicationId& pub_id);

  /**
  * Remove all writers that write to the specified instance.
  */
  void remove_writers (const ::DDS::InstanceHandle_t& instance_handle);

  /**
  * Remove a writer that write to the specified instance.
  * Return true if it's the owner writer removed.
  */
  bool remove_writer (const ::DDS::InstanceHandle_t& instance_handle,
                      const PublicationId& pub_id);

  /**
  * Return true if the provide writer is the owner of the instance.
  */
  bool is_owner (const ::DDS::InstanceHandle_t& instance_handle,
                 const PublicationId& pub_id);

  /**
  * Determine if the provided publication can be the owner.
  */
  bool select_owner(const ::DDS::InstanceHandle_t& instance_handle,
                    const PublicationId& pub_id,
                    const CORBA::Long& ownership_strength,
                    InstanceState* instance_state);

  /**
  * Remove an owner of the specified instance.
  */
  void remove_owner (const ::DDS::InstanceHandle_t& instance_handle);

  /**
  * Update the ownership strength of a publication.
  */
  void update_ownership_strength (const PublicationId& pub_id,
                                  const CORBA::Long& ownership_strength);

private:

  bool remove_writer (const ::DDS::InstanceHandle_t& instance_handle,
                      OwnershipWriterInfos& infos,
                      const PublicationId& pub_id);

  void remove_owner (const ::DDS::InstanceHandle_t& instance_handle,
                     OwnershipWriterInfos& infos,
                     bool sort);

  void remove_candidate (OwnershipWriterInfos& infos,const PublicationId& pub_id);

  void broadcast_new_owner (const ::DDS::InstanceHandle_t& instance_handle,
                            OwnershipWriterInfos& infos,
                            const PublicationId& owner);

  ACE_Thread_Mutex instance_lock_;
  TypeInstanceMap type_instance_map_;
  InstanceOwnershipWriterInfos instance_ownership_infos_;

};

} // namespace DCPS
} // namespace OpenDDS

#endif /* OPENDDS_DCPS_OWNERSHIP_MANAGER_H  */
