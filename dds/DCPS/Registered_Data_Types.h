/*
 * $Id$
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef REGISTERED_DATA_TYPES_H_
#define REGISTERED_DATA_TYPES_H_

#include "dcps_export.h"
#include "dds/DdsDcpsInfrastructureS.h"
#include "dds/DdsDcpsTopicS.h"
#include "dds/DdsDcpsTypeSupportExtS.h"

#include "tao/TAO_Singleton.h"

#include <map>
#include <string>

#if !defined (ACE_LACKS_PRAGMA_ONCE)
#pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

namespace OpenDDS {
namespace DCPS {

typedef std::map<std::string, OpenDDS::DCPS::TypeSupport_ptr> TypeSupportHash;
typedef std::map<void*, TypeSupportHash*> DomainHash;

/**
* A singleton class that keeps track of the registered DDS data types
* local to this process.
* Data types are split into separate domains.
*/
class OpenDDS_Dcps_Export Data_Types_Register {
  friend class TAO_Singleton<Data_Types_Register, TAO_SYNCH_MUTEX>;

public:

  /// Return a singleton instance of this class.
  static Data_Types_Register * instance();

  /**
   * Register a type.
   *
   * @note This class takes Ownership of the memory pointed to by the_type
   *  when this method returns RETCODE_OK  It does this by calling
   *  _add_ref() on the TypeSupport_ptr
   *
   * @returns RETCODE_OK if the type_name is unique to the domain participant
   *         or the type_name is already registered to the_type.
   *         Otherwise returns RETCODE_ERROR
   */
  DDS::ReturnCode_t register_type(DDS::DomainParticipant_ptr domain_participant,
                                  ACE_CString type_name,
                                  OpenDDS::DCPS::TypeSupport_ptr the_type);

  DDS::ReturnCode_t unregister_participant(DDS::DomainParticipant_ptr domain_participant);

  /**
   * Find a data type by its type name.
   * @note This class retains Ownership of the memory returned
   * @returns a pointer to the memory registered to the
   *         type_name
   *         Otherwise returns TypeSupport::_nil()
   */
  OpenDDS::DCPS::TypeSupport_ptr lookup(DDS::DomainParticipant_ptr domain_participant,
                                        ACE_CString type_name);
private:
  Data_Types_Register();
  ~Data_Types_Register();

  ACE_SYNCH_RECURSIVE_MUTEX lock_;
  DomainHash domains_;
};

#define Registered_Data_Types Data_Types_Register::instance()

} // namespace DCPS
} // namespace OpenDDS

#endif /* REGISTERED_DATA_TYPES_H_  */
