/*
 * $Id$
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef CONFIGUTILS_H
#define CONFIGUTILS_H

#include "ace/Configuration.h"
#include "dcps_export.h"
#include "dds/DCPS/PoolAllocator.h"

#include <map>
#include <list>

#ifdef ACE_LYNXOS_MAJOR
# include <strstream>
# include <string>
#else
# include <sstream>
#endif

namespace OpenDDS {
namespace DCPS {

  /// Helper types and functions for config file parsing
  typedef OPENDDS_MAP(OPENDDS_STRING, OPENDDS_STRING) ValueMap;
  typedef std::pair<OPENDDS_STRING, ACE_Configuration_Section_Key> SubsectionPair;
  typedef std::list<SubsectionPair> KeyList;

  template <typename T> bool convertToInteger( const OPENDDS_STRING& s,
                                               T& value )
  {
#ifdef ACE_LYNXOS_MAJOR
    std::istrstream istr(s.c_str());
#else
    std::stringstream istr(s.c_str());
#endif
    if (!(istr >> value) || (istr.peek() != EOF)) return false;
    return true;
  }

  ///     Function that pulls all the values from the
  ///     specified ACE Configuration Section and places them in a
  ///     value map based on the field name.  Returns the number of
  ///     values found in the section (and added to the value map).
  ///
  ///     cf     ACE_Configuration_Heap object being processed
  ///     key    ACE_Configuration_Section_Key object that specifies
  ///            the section of the .ini file to process
  ///     values Map of field names to values (both OPENDDS_STRING)
  ///            that this function will add to.
  ///
  OpenDDS_Dcps_Export int pullValues( ACE_Configuration_Heap& cf,
                                      const ACE_Configuration_Section_Key& key,
                                      ValueMap& values );

  ///     Function that processes the specified ACE Configuration Section
  ///     for subsections.  If multiple levels of subsections are found,
  ///     a non-zero value is returned to indicate the error.
  ///     All valid subsection will be placed into the supplied
  ///     KeyList (std::pair<> of the subsection number and
  ///     ACE_Configuration_Section_Key).  A return value of zero indicates
  ///     error-free success.
  ///
  ///
  ///     cf              ACE_Configuration_Heap object being processed
  ///     key             ACE_Configuration_Section_Key object that
  ///                     specifies the section of the .ini file to process
  ///     subsections     List of subsections found (list contains a
  ///                     std::pair<> of the subsection number and
  ///                     ACE_Configuration_Section_Key).
  ///
  OpenDDS_Dcps_Export int processSections( ACE_Configuration_Heap& cf,
                                           const ACE_Configuration_Section_Key& key,
                                           KeyList& subsections );

} // namespace DCPS
} // namespace OpenDDS

#endif /* CONFIGUTILS_H */
