/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef FEDERATORCONFIG_H
#define FEDERATORCONFIG_H

#if !defined (ACE_LACKS_PRAGMA_ONCE)
#pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

#include "federator_export.h"
#include "FederationId.h"
#include "FederatorC.h"

#include <string>

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace Federator {

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
  Config(int argc, ACE_TCHAR** argv);

  virtual ~Config();

  /// Access the enhanced argv.
  int& argc();
  int  argc() const;

  /// Access the enhanced argc.
  ACE_TCHAR**& argv();
  ACE_TCHAR**  argv() const;

  /// Add an argument.
  void addArg(ACE_TCHAR* arg);

  /// Federation Id value.
  TAO_DDS_DCPSFederationId& federationId();
  const TAO_DDS_DCPSFederationId& federationId() const;

  /// Federation Id value.
  void federationDomain(long domain);
  long  federationDomain() const;

  /// Federation Port value.
  void federationPort(short port);
  short  federationPort() const;

  /// Initial federation IOR value.
  void federateIor(const tstring& ior);
  tstring  federateIor() const;

  /// Configuration filename.
  void configFile(const tstring& file);
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
  TAO_DDS_DCPSFederationId federationId_;

  /// Configured Federation Domain value.
  long federationDomain_;

  /// Configured Federation Port value.
  short federationPort_;
};

} // namespace Federator
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#if defined (__ACE_INLINE__)
# include "FederatorConfig.inl"
#endif  /* __ACE_INLINE__ */

#endif /* FEDERATORCONFIG_H */
