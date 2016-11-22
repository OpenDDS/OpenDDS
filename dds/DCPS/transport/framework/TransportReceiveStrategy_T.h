/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_TRANSPORTRECEIVESTRATEGY
#define OPENDDS_DCPS_TRANSPORTRECEIVESTRATEGY

#include "dds/DCPS/dcps_export.h"
#include "ReceivedDataSample.h"
#include "TransportStrategy.h"
#include "TransportDefs.h"
#include "TransportHeader.h"

#include "ace/INET_Addr.h"
#include "ace/Lock_Adapter_T.h"
#include "ace/Synch_Traits.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

/**
 * This class provides buffer for data received by transports, de-assemble
 * the data to individual samples and deliver them.
 */
template<typename TH = TransportHeader, typename DSH = DataSampleHeader>
class TransportReceiveStrategy
  : public TransportStrategy {
public:

  virtual ~TransportReceiveStrategy();

  int start();
  void stop();

  int handle_dds_input(ACE_HANDLE fd);

  /// The subclass needs to provide the implementation
  /// for re-establishing the datalink. This is called
  /// when recv returns an error.
  virtual void relink(bool do_suspend = true);

  /// Provides access to the received transport header
  /// for subclasses.
  const TH& received_header() const;
  TH& received_header();

  /// Provides access to the received sample header
  /// for subclasses.
  const DSH& received_sample_header() const;
  DSH& received_sample_header();

protected:
  TransportReceiveStrategy();

  /// Only our subclass knows how to do this.
  virtual ssize_t receive_bytes(iovec          iov[],
                                int            n,
                                ACE_INET_Addr& remote_address,
                                ACE_HANDLE     fd) = 0;

  /// Check the transport header for suitability.
  virtual bool check_header(const TH& header);

  /// Check the data sample header for suitability.
  virtual bool check_header(const DSH& header);

  /// Called when there is a ReceivedDataSample to be delivered.
  virtual void deliver_sample(ReceivedDataSample&  sample,
                              const ACE_INET_Addr& remote_address) = 0;

  /// Let the subclass start.
  virtual int start_i() = 0;

  /// Let the subclass stop.
  virtual void stop_i() = 0;

  /// Ignore bad PDUs by skipping over them.
  int skip_bad_pdus();

  /// For datagram-based derived classes, reset() can be called to clear any
  /// state that may be remaining from parsing the previous datagram.
  void reset();

  size_t pdu_remaining() const { return this->pdu_remaining_; }

  /// Flag indicates if the GRACEFUL_DISCONNECT message is received.
  bool gracefully_disconnected_;

private:

  /// Manage an index into the receive buffer array.
  size_t successor_index(size_t index) const;

  void update_buffer_index(bool& done);

  virtual bool reassemble(ReceivedDataSample& data);

  /// Bytes remaining in the current DataSample.
  size_t receive_sample_remaining_;

  /// Current receive TransportHeader.
  TH receive_transport_header_;

  //
  // The total available space in the receive buffers must have enough to hold
  // a max sized message.  The max message is about 64K and the low water for
  // a buffer is 4096.  Therefore, 16 receive buffers is appropriate.
  //
  enum { RECEIVE_BUFFERS  =   16 };
  enum { BUFFER_LOW_WATER = 4096 };

  //
  // Message Block Allocators are more plentiful since they hold samples
  // as well as data read from the handle(s).
  //
  enum { MESSAGE_BLOCKS   = 1000 };
  enum { DATA_BLOCKS      =  100 };

//MJM: We should probably bring the allocator typedefs down into this
//MJM: class since they are limited to this scope.
  TransportMessageBlockAllocator mb_allocator_;
  TransportDataBlockAllocator    db_allocator_;
  TransportDataAllocator         data_allocator_;

  /// Locking strategy for the allocators.
  ACE_Lock_Adapter<ACE_SYNCH_MUTEX> receive_lock_;

  /// Set of receive buffers in use.
  ACE_Message_Block* receive_buffers_[RECEIVE_BUFFERS];

  /// Current receive buffer index in use.
  size_t buffer_index_;

  /// Current data sample header.
  DSH data_sample_header_;

  ACE_Message_Block* payload_;

  /** Flag indicating that the currently resident PDU is a good one
    * (i.e. has not been received and processed previously).  This is
    * included in case we receive PDUs that were resent for reliability
    * reasons and we receive one even if we have already processed it.
    * This is a use case from multicast transports.
    */
  bool good_pdu_;

  /// Amount of the current PDU that has not been processed yet.
  size_t pdu_remaining_;
};

} // namespace DCPS */
} // namespace OpenDDS */

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#if defined (__ACE_INLINE__)
#include "TransportReceiveStrategy_T.inl"
#endif /* __ACE_INLINE__ */

#ifdef ACE_TEMPLATES_REQUIRE_SOURCE
#include "TransportReceiveStrategy_T.cpp"
#endif

#endif /* OPENDDS_DCPS_TRANSPORTRECEIVESTRATEGY */
