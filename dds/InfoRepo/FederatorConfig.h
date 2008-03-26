// -*- C++ -*-
//
// $Id$
#ifndef FEDERATORCONFIG_H
#define FEDERATORCONFIG_H

#include "federator_export.h"

#include <string>

namespace OpenDDS { namespace Federator {

class OpenDDS_Federator_Export Config  {
  public:
    /// Value of the command line option for the configuration file.
    static const std::string FEDERATOR_CONFIG_OPTION;

    /// Default constructor.
    Config( int argc, char** argv);

    /// Virtual destructor.
    virtual ~Config();

    /// Process a configuration file
    void configFile( const std::string arg);

    /// Access the enhanced argv.
    int& argc();
    int  argc() const;

    /// Access the enhanced argc.
    char**& argv();
    char**  argv() const;

  private:
    /// Enhanced argc.
    int argc_;

    /// Enhanced argv.
    char** argv_;

    /// Actual storage size for argument pointers.
    int argvSize_;
};

}} // End namespace OpenDDS::Federator

#endif // FEDERATORCONFIG_H

