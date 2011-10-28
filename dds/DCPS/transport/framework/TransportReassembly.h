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
  /// the datalink.  The 'data' argument may be modified by this method.
  bool reassemble(const SequenceNumber& transportSeq, bool firstFrag,
                  ReceivedDataSample& data);

  /// Called by TransportReceiveStrategy to indicate that we can
  /// stop tracking partially-reassembled messages when we know the
  /// remaining fragments are not expected to arrive.
  void data_unavailable(const SequenceRange& dropped);

private:

  // A FragRange represents a chunk of a partially-reassembled message.
  // The transport_seq_ range is the range of transport sequence numbers
  // that were used to send the given chunk of data.
  struct FragRange {
    FragRange(const SequenceNumber& transportSeq,
              const ReceivedDataSample& data);

    SequenceRange transport_seq_;
    ReceivedDataSample rec_ds_;
  };

  // Each element of the FragMap "fragments_" represents one sent message
  // (one DataSampleHeader before fragmentation).  The map's key is the
  // DataSampleHeader sequence_ number.  The list must have at
  // least one value in it.  If a FragRange in the list has a sample_ with
  // a null ACE_Message_Block*, it's one that was data_unavailable().
  typedef std::map<SequenceNumber, std::list<FragRange> > FragMap;
  FragMap fragments_;

  std::set<SequenceNumber> have_first_;

  static bool insert(std::list<FragRange>& flist,
                     const SequenceNumber& transportSeq,
                     ReceivedDataSample& data);

  void dropped_one(const SequenceNumber& dropped, const SequenceNumber& first);
};

}
}

#endif
