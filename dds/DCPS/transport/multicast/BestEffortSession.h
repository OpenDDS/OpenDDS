/*
 * $Id$
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef DCPS_BESTEFFORTSESSION_H
#define DCPS_BESTEFFORTSESSION_H

#include "Multicast_Export.h"

#include "MulticastSession.h"

namespace OpenDDS {
namespace DCPS {

class OpenDDS_Multicast_Export BestEffortSession
  : public MulticastSession {
public:
  BestEffortSession(MulticastDataLink* link,
                    MulticastPeer remote_peer);

  virtual bool acked();

  virtual bool check_header(const TransportHeader& header);
  virtual bool check_header(const DataSampleHeader& header);

  virtual void control_received(char submessage_id,
                                ACE_Message_Block* control);

  virtual bool start(bool active);
  virtual void stop();

private:
  SequenceNumber last_received_;
};

} // namespace DCPS
} // namespace OpenDDS

#endif  /* DCPS_BESTEFFORTSESSION_H */
