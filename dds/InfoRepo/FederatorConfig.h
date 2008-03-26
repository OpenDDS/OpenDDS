// -*- C++ -*-
//
// $Id$
#ifndef FEDERATORCONFIG_H
#define FEDERATORCONFIG_H

#if !defined (ACE_LACKS_PRAGMA_ONCE)
#pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

#include "federator_export.h"

#include <string>

namespace OpenDDS { namespace Federator {

class OpenDDS_Federator_Export Config  {
  public:
    /// Command line option specifying the configuration file.
    static const std::string FEDERATOR_CONFIG_OPTION;

    /// Default constructor.
    Config( int argc, char** argv);

    /// Virtual destructor.
    virtual ~Config();

    /// Access the enhanced argv.
    int& argc();
    int  argc() const;

    /// Access the enhanced argc.
    char**& argv();
    char**  argv() const;

  private:
    /// Process a configuration file
    void process();

    /// Enhanced argc.
    int argc_;

    /// Enhanced argv.
    char** argv_;

    /// Configuration filename, if any.
    std::string configFile_;
};

}} // End namespace OpenDDS::Federator

#if defined (__ACE_INLINE__)
# include "FederatorConfig.inl"
#endif  /* __ACE_INLINE__ */

#endif // FEDERATORCONFIG_H

