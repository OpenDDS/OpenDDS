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

#include <map>
#include <string>

namespace OpenDDS { namespace Federator {

class OpenDDS_Federator_Export Config  {
  public:
    /// Command line option specifying the configuration file.
    static const std::string FEDERATOR_CONFIG_OPTION;

    /// Type to map route information.
    typedef std::map< std::string, std::string> HostToRouteMap;

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

    /// Federation Id value.
    RepoKey& federationId();
    RepoKey  federationId() const;

    /// Federation Port value.
    short& federationPort();
    short  federationPort() const;

    /// Default route value.
    std::string& defaultRoute();
    std::string  defaultRoute() const;

    /**
     * Access the routing information.
     *
     * N.B. This is a const reference, so operator[] will not be usable.
     *      Only const_iterators will be available.
     */
    const HostToRouteMap& route() const;

    /// Common use case for obtaining a route.
    std::string route( const std::string& remote) const;

  private:
    /// Process a configuration file
    void processFile();

    /// Build the preferredInterfaces option value.
    void buildInterfaceList();

    /// Enhanced argc.
    int argc_;

    /// Enhanced argv.
    char** argv_;

    /// Configuration filename, if any.
    std::string configFile_;

    /**
     * Preferred interfaces list.
     *
     * N.B. This is a member to allow its lifetime to span the entire
     *      lifetime of the enhanced argv value in which it will be placed.
     */
    std::string interfaceList_;

    /// Configured Federation Id value.
    RepoKey federationId_;

    /// Configured Federation Port value.
    short federationPort_;

    /// Default route if none specified.
    std::string defaultRoute_;

    /// Routing information.
    HostToRouteMap route_;
};

}} // End namespace OpenDDS::Federator

#if defined (__ACE_INLINE__)
# include "FederatorConfig.inl"
#endif  /* __ACE_INLINE__ */

#endif // FEDERATORCONFIG_H

