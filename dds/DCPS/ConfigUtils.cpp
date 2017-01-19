/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "DCPS/DdsDcps_pch.h" //Only the _pch include should start with DCPS/

#include "ConfigUtils.h"
#include "ace/SString.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS { namespace DCPS {

  int pullValues( ACE_Configuration_Heap& cf,
                  const ACE_Configuration_Section_Key& key,
                  ValueMap& values ) {
    int index = 0;
    ACE_TString name;
    ACE_Configuration::VALUETYPE type;

    while (cf.enumerate_values( key, index, name, type ) == 0) {

      ACE_TString value;
      if (type == ACE_Configuration::STRING) {
        cf.get_string_value( key, name.c_str(), value );
        values[ACE_TEXT_ALWAYS_CHAR(name.c_str())] =
          ACE_TEXT_ALWAYS_CHAR(value.c_str());
      } else {
        ACE_DEBUG((LM_WARNING, "Unexpected value type in config file (ignored): "
                   "name=%s, type=%d\n", name.c_str(), type));
      }
      index++;
    }
    return index;
  }


  int processSections( ACE_Configuration_Heap& cf,
                       const ACE_Configuration_Section_Key& key,
                       KeyList& subsections ) {
    int index = 0;
    ACE_TString name;
    while (cf.enumerate_sections( key, index, name ) == 0) {
      ACE_Configuration_Section_Key subkey;
      if (cf.open_section( key, name.c_str(), 0, subkey ) != 0) {
        return 1;
      }
      subsections.push_back( SubsectionPair( ACE_TEXT_ALWAYS_CHAR(name.c_str()),
                                             subkey ) );
      int subindex = 0;
      ACE_TString subname;
      if (cf.enumerate_sections( subkey, subindex, subname ) == 0) {
        // Found additional nesting of subsections that we don't care
        // to allow (e.g. [transport/my/yours]), so return an error.
        return 1;
      }
      index++;
    }
    return 0;
  }

}  }

OPENDDS_END_VERSIONED_NAMESPACE_DECL
