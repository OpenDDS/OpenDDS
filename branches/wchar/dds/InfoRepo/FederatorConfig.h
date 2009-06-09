// -*- C++ -*-
//
// $Id$
#ifndef FEDERATORCONFIG_H
#define FEDERATORCONFIG_H

#if !defined (ACE_LACKS_PRAGMA_ONCE)
#pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

#include "federator_export.h"
#include "FederatorC.h"

#include <string>

namespace OpenDDS { namespace Federator {

typedef std::basic_string<ACE_TCHAR> tstring;

class OpenDDS_Federator_Export Config  {
  public:
    /// Command line option specifying the configuration file.
    static const tstring FEDERATOR_CONFIG_OPTION;

    /// Command line option specifying the federation Id value.
    static const tstring FEDERATOR_ID_OPTION;

    /// Command line option specifying a repository to federate with.
    static const tstring FEDERATE_WITH_OPTION;

    /// Default constructor.
    Config( int argc, ACE_TCHAR** argv);

    /// Virtual destructor.
    virtual ~Config();

    /// Access the enhanced argv.
    int& argc();
    int  argc() const;

    /// Access the enhanced argc.
    ACE_TCHAR**& argv();
    ACE_TCHAR**  argv() const;

    /// Federation Id value.
    RepoKey& federationId();
    RepoKey  federationId() const;

    /// Federation Id value.
    long& federationDomain();
    long  federationDomain() const;

    /// Federation Port value.
    short& federationPort();
    short  federationPort() const;

    /// Initial federation IOR value.
    tstring& federateIor();
    tstring  federateIor() const;

    /// Configuration filename.
    tstring& configFile();
    tstring  configFile() const;

  private:
    /// Process a configuration file
    void processFile();

    /// Enhanced argc.
    int argc_;

    /// Enhanced argv.
    ACE_TCHAR** argv_;

    /// Configuration filename, if any.
    tstring configFile_;

    /// Initial federation IOR, if any.
    tstring federateIor_;

    /// Configured Federation Id value.
    RepoKey federationId_;

    /// Configured Federation Domain value.
    long federationDomain_;

    /// Configured Federation Port value.
    short federationPort_;
};

}} // End namespace OpenDDS::Federator

#if defined (__ACE_INLINE__)
# include "FederatorConfig.inl"
#endif  /* __ACE_INLINE__ */

#endif // FEDERATORCONFIG_H

