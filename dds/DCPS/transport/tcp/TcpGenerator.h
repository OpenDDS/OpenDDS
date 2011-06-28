/*
 * $Id$
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef SIMPLETCP_GENERATOR_H
#define SIMPLETCP_GENERATOR_H

#include "Tcp_export.h"

#include "dds/DCPS/transport/framework/TransportGenerator.h"
#include "ace/Synch.h"

namespace OpenDDS {
namespace DCPS {

class OpenDDS_Tcp_Export TcpGenerator : public TransportGenerator {
public:

  /// Default ctor.
  TcpGenerator();

  /// Dtor
  virtual ~TcpGenerator();

  /// Provide a new TcpFactory instance.
  virtual TransportImplFactory* new_factory();

  /// Provide a new TcpInst instance.
  virtual TransportInst* new_configuration(const TransportIdType id);

  /// Provide a list of default transport id.
  virtual void default_transport_ids(TransportIdList & ids);
};

} // namespace DCPS
} // namespace OpenDDS

#endif
