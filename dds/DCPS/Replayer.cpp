/*
 * $Id$
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */
 
#include "Replayer.h"
 
namespace OpenDDS {
namespace DCPS {
  ReplayerListener::~ReplayerListener()
  {
  }
  
  void ReplayerListener::on_replayer_matched(const Replayer* replayer,
                            const ::DDS::PublicationMatchedStatus & status )
  {
  }
  
  Replayer::~Replayer()
  {
  }
  
} // namespace DCPS
} // namespace
