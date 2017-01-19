/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef REGISTERED_DATA_TYPES_H_
#define REGISTERED_DATA_TYPES_H_

#include "dcps_export.h"
#include "dds/DdsDcpsDomainC.h"
#include "dds/DdsDcpsTypeSupportExtC.h"
#include "Definitions.h"

#include "PoolAllocator.h"

#include "ace/Singleton.h"

#if !defined (ACE_LACKS_PRAGMA_ONCE)
#pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

typedef OPENDDS_MAP(OPENDDS_STRING, TypeSupport_var) TypeSupportMap;
typedef OPENDDS_MAP(DDS::DomainParticipant_ptr, TypeSupportMap) ParticipantMap;

/**
* A singleton class that keeps track of the registered DDS data types
* local to this process.
* Data types are registered by domain participant.
*/
class OpenDDS_Dcps_Export Data_Types_Register {
  friend class ACE_Singleton<Data_Types_Register, ACE_SYNCH_MUTEX>;

public:

  /// Return a singleton instance of this class.
  static Data_Types_Register* instance();

  /**
   * Register a type.
   *
   * @returns RETCODE_OK if the type_name is unique to the domain participant
   *         or the type_name is already registered to the_type's class.
   *         Otherwise returns RETCODE_ERROR
   */
  DDS::ReturnCode_t register_type(DDS::DomainParticipant_ptr domain_participant,
                                  const char* type_name,
                                  TypeSupport_ptr the_type);

  /**
   * Unregister a type.
   *
   * @returns RETCODE_OK if the type_name has been removed or if the type_name
   *        cannot be found in the map associated with the domain_participant.
   *        Otherwise returns RETCODE_ERROR
   */
  DDS::ReturnCode_t unregister_type(DDS::DomainParticipant_ptr domain_participant,
                                    const char* type_name,
                                    TypeSupport_ptr the_type);

  DDS::ReturnCode_t unregister_participant(DDS::DomainParticipant_ptr domain_participant);

  /**
   * Find a data type by its type name.
   * @returns the TypeSupport object registered as type_name
   *         Otherwise returns TypeSupport::_nil()
   */
  TypeSupport_ptr lookup(DDS::DomainParticipant_ptr domain_participant,
                         const char* type_name) const;
private:
  Data_Types_Register();
  ~Data_Types_Register();

  mutable ACE_SYNCH_MUTEX lock_;
  ParticipantMap participants_;
};

#define Registered_Data_Types Data_Types_Register::instance()

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif /* REGISTERED_DATA_TYPES_H_  */
