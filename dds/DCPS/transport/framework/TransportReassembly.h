/*
 * $Id$
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_TRANSPORTREASSEMBLY
#define OPENDDS_DCPS_TRANSPORTREASSEMBLY

#include "dds/DCPS/dcps_export.h"
#include "dds/DCPS/Definitions.h"
#include "ReceivedDataSample.h"

#include <list>
#include <map>
#include <set>

namespace OpenDDS {
namespace DCPS {

class OpenDDS_Dcps_Export TransportReassembly {
public:

  /// Called by TransportReceiveStrategy if the fragmentation header flag
  /// is set.  Returns true/false to indicate if data should be delivered to
  /// the datalink.  If true, 'data' may be modified by this method to include
  /// the previously-buffered fragments.
  bool reassemble(const SequenceNumber& transportSeq, bool firstFrag,
                  ReceivedDataSample& data);

  /// Called by TransportReceiveStrategy to indicate that we can
  /// stop tracking fragments when we know the remaining fragments
  /// are not expected to arrive.
  void data_unavailable(const SequenceRange& dropped);

private:

  struct FragRange {
    FragRange(const SequenceNumber& transportSeq,
              const ReceivedDataSample& data);

    SequenceRange transport_seq_;
    ReceivedDataSample sample_;
  };

  typedef std::map<SequenceNumber, std::list<FragRange> > FragMap;
  FragMap fragments_;

  std::set<SequenceNumber> have_first_;

  static bool insert(std::list<FragRange>& flist,
                     const SequenceNumber& transportSeq,
                     ReceivedDataSample& data);
};

}
}

#endif
