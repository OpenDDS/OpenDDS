/*
 * $Id$
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_TRANSPORT_CLIENT_H
#define OPENDDS_DCPS_TRANSPORT_CLIENT_H

#include "dds/DCPS/dcps_export.h"
#include "TransportImpl.h"
#include "DataLinkSet.h"

#include <vector>

namespace OpenDDS {
namespace DCPS {

class EntityImpl;
class TransportInst;

/**
 * @brief Mix-in class for DDS entities which directly use the transport layer.
 *
 * DataReaderImpl and DataWriterImpl are TransportClients.  The TransportClient
 * class manages the TransportImpl objects that represent the available
 * communication mechanisms and the DataLink objects that represent the
 * currently active communication channels to peers.
 */
class OpenDDS_Dcps_Export TransportClient {
protected:
  TransportClient();
  virtual ~TransportClient();

  void enable_transport();

private:
  virtual bool check_transport_qos(const TransportInst& inst) = 0;

  std::vector<TransportImpl_rch> impls_;
  DataLinkSet links_;
};

}
}

#endif
