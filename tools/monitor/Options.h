// -*- C++ -*-
//
/*
*
*
* Distributed under the OpenDDS License.
* See: http://www.opendds.org/license.html
*/

#ifndef OPTIONS_H
#define OPTIONS_H

// Needed here to avoid the pragma below when necessary.
#include /**/ "ace/pre.h"
#include /**/ "ace/config-all.h"

#if !defined (ACE_LACKS_PRAGMA_ONCE)
#pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

#include <iosfwd>
#include <string>
#include <map>

ACE_BEGIN_VERSIONED_NAMESPACE_DECL
class ACE_Configuration_Heap;
class ACE_Configuration_Section_Key;
ACE_END_VERSIONED_NAMESPACE_DECL

namespace Monitor {

/**
 * @class Options
 *
 * @brief manage execution options
 *
 * This class extracts option information from the command line and makes
 * it available to the process.
 *
 * Options extracted by this class are:
 *
 *   -v
 *      Be verbose when executing.
 *
 * NOTE: The order on the command line determines the final value of a
 *       configurable element.  This means that if more than one value is
 *       given on the command line the rightmost value will be used.
 *       Likewise if a value is configured on both the command line and
 *       in a configuration file, the processing order is honored.
 *       This means that it is preferable to specify the configuration
 *       file first on the command line so that the remaining values
 *       can override the values from the command line.
 */
class Options  {
  public:
    Options( int argc, ACE_TCHAR** argv, char** envp = 0);

    virtual ~Options();

    /// Indication of configuration status.
    operator bool() const;

    /// @name Option processing information.
    /// @{

    /// Verbosity.
    bool  verbose() const;

    /// Configuration status.
    bool  configured() const;

    /// @}

    /// Instrumentation domain.
    long domain() const;

  private:
    /// Test verbosity.
    bool verbose_;

    /// Success of configuration steps.
    bool configured_;

    /// Instrumentation domain.
    long domain_;
};

} // End of namespace Monitor

#endif // OPTIONS_H

