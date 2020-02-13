/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_TRANSPORTQUEUEELEMENT_H
#define OPENDDS_DCPS_TRANSPORTQUEUEELEMENT_H

#include "dds/DCPS/dcps_export.h"
#include "dds/DCPS/Definitions.h"
#include "dds/DCPS/GuidUtils.h"
#include "dds/DCPS/PoolAllocationBase.h"
#include "dds/DCPS/SequenceNumber.h"

#include <utility>

ACE_BEGIN_VERSIONED_NAMESPACE_DECL
class ACE_Message_Block;
ACE_END_VERSIONED_NAMESPACE_DECL

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

class DataSampleElement;

class TransportQueueElement;
typedef std::pair<TransportQueueElement*, TransportQueueElement*> ElementPair;

/**
 * @class TransportQueueElement
 *
 * @brief Base wrapper class around a data/control sample to be sent.
 *
 * This class serves as the base class for different types of samples
 * that can be sent.  For example, there are data samples and control
 * samples.  A subclass of TransportQueueElement exists for each of
 * these types of samples.
 *
 * This class maintains a counter that, when decremented to 0, will
 * trigger some logic (defined in the subclass) that will "return
 * the loan" of the sample.  The sample is "loaned" to the transport
 * via a send() or send_control() call on the TransportClient.
 * This wrapper object will "return the loan" when all DataLinks have
 * "returned" their sub-loans.
 */
class OpenDDS_Dcps_Export TransportQueueElement : public PoolAllocationBase {
public:

  virtual ~TransportQueueElement();

  class OpenDDS_Dcps_Export MatchCriteria {
  protected:
    virtual ~MatchCriteria();
    MatchCriteria() {}
  public:
    virtual bool matches(const TransportQueueElement& candidate) const = 0;
    virtual bool unique() const = 0; // (only expect to match 1 element)
  private: // and unimplemented...
    MatchCriteria(const MatchCriteria&);
    MatchCriteria& operator=(const MatchCriteria&);
  };

  class OpenDDS_Dcps_Export MatchOnPubId : public MatchCriteria {
  public:
    explicit MatchOnPubId(const RepoId& id) : pub_id_(id) {}
    virtual ~MatchOnPubId();
    virtual bool matches(const TransportQueueElement& candidate) const;
    virtual bool unique() const { return false; }
  private:
    RepoId pub_id_;
  };

  class OpenDDS_Dcps_Export MatchOnDataPayload : public MatchCriteria {
  public:
    explicit MatchOnDataPayload(const char* data) : data_(data) {}
    virtual ~MatchOnDataPayload();
    virtual bool matches(const TransportQueueElement& candidate) const;
    virtual bool unique() const { return true; }
  private:
    const char* data_;
  };

  /// Invoked when the sample is dropped from a DataLink due to a
  /// remove_sample() call.
  /// The dropped_by_transport flag true indicates the data dropping is initiated
  /// by transport when the transport send strategy is in a MODE_TERMINATED.
  /// The dropped_by_transport flag false indicates the dropping is initiated
  /// by the remove_sample and data_dropped() is a result of remove_sample().
  /// The return value indicates if this element is released.
  bool data_dropped(bool dropped_by_transport = false);

  /// Invoked when the sample has been sent by a DataLink.
  /// The return value indicates if this element is released.
  bool data_delivered();

  /// Does the sample require an exclusive transport packet?
  virtual bool requires_exclusive_packet() const;

  /// Accessor for the publication id that sent the sample.
  virtual RepoId publication_id() const = 0;

  /// Accessor for the subscription id, if sent the sample is sent to 1 sub
  virtual RepoId subscription_id() const {
    return GUID_UNKNOWN;
  }

  virtual SequenceNumber sequence() const {
    return SequenceNumber::SEQUENCENUMBER_UNKNOWN();
  }

  /// The marshalled sample (sample header + sample data)
  virtual const ACE_Message_Block* msg() const = 0;

  /// The marshalled payload only (sample data)
  virtual const ACE_Message_Block* msg_payload() const = 0;

  /// Is the element a "control" sample from the specified pub_id?
  virtual bool is_control(RepoId pub_id) const;

  /// Is the listener get called ?
  bool released() const;
  void released(bool flag);

  /// Clone method with provided message block allocator and data block
  /// allocators.
  static ACE_Message_Block* clone_mb(const ACE_Message_Block* msg,
                                     MessageBlockAllocator* mb_allocator,
                                     DataBlockAllocator* db_allocator);

  /// Is the sample created by the transport?
  virtual bool owned_by_transport() = 0;

  /// Create two TransportQueueElements representing the same data payload
  /// as the current TransportQueueElement, with the first one (including its
  /// DataSampleHeader) fitting in "size" bytes.  This method leaves the
  /// current TransportQueueElement alone (but can't be made const because
  /// the newly-created elements will need to invoke non-const methods on it).
  /// Each element in the pair will contain its own serialized modified
  /// DataSampleHeader.
  virtual ElementPair fragment(size_t size);

  /// Is this QueueElement the result of fragmentation?
  virtual bool is_fragment() const { return false; }

  virtual bool is_request_ack() const { return false; }

  virtual bool is_retained_replaced() const { return false; }

protected:

  /// Ctor.  The initial_count is the number of DataLinks to which
  /// this TransportQueueElement will be sent.
  explicit TransportQueueElement(unsigned long initial_count);

  /// Invoked when the counter reaches 0.
  virtual void release_element(bool dropped_by_transport) = 0;

  /// May be used by subclass' implementation of release_element()
  /// to determine if any DataLinks dropped the data instead of
  /// delivering it.
  bool was_dropped() const;

private:

  /// Common logic for data_dropped() and data_delivered().
  bool decision_made(bool dropped_by_transport);
  friend class TransportCustomizedElement;

  /// Counts the number of outstanding sub-loans.
  ACE_Atomic_Op<ACE_Thread_Mutex, unsigned long> sub_loan_count_;

  /// Flag flipped to true if any DataLink dropped the sample.
  bool dropped_;

  /// If the callback to DW is made.
  bool released_;
};

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#if defined(__ACE_INLINE__)
#include "TransportQueueElement.inl"
#endif /* __ACE_INLINE__ */

#endif  /* OPENDDS_DCPS_TRANSPORTQUEUEELEMENT_H */
