/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_TRANSPORT_FRAMEWORK_RECEIVEDDATASAMPLE_H
#define OPENDDS_DCPS_TRANSPORT_FRAMEWORK_RECEIVEDDATASAMPLE_H

#include <dds/DCPS/DataSampleHeader.h>
#include <dds/DCPS/PoolAllocator.h>

#include <dds/DCPS/MessageBlock.h>

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

/**
 * @class ReceivedDataSample
 *
 * @brief Holds a data sample received by the transport.
 *
 * This is the type of object that is delivered to the
 * TransportReceiveListener objects by the transport.
 * Note that the data sample header has already been
 * demarshalled by the transport, and the ACE_Message_Block (chain)
 * represents the "data" portion of the sample.
 *
 * Internally, ReceivedDataSample uses an alternate representation
 * of the ACE_Message_Block with contiguous storage (vector)
 * instead of a linked list to implement the continuation chain.
 */
class OpenDDS_Dcps_Export ReceivedDataSample {
public:
  ReceivedDataSample();

  explicit ReceivedDataSample(const ACE_Message_Block& payload);

  ReceivedDataSample(const ReceivedDataSample& other);
  ReceivedDataSample& operator=(const ReceivedDataSample& rhs);

#if defined (ACE_HAS_CPP11)
  ReceivedDataSample(ReceivedDataSample&& other);
  ReceivedDataSample& operator=(ReceivedDataSample&& rhs);
#endif

  /// The demarshalled sample header.
  DataSampleHeader header_;

  /// Fragment size used by this sample
  ACE_UINT32 fragment_size_;

  /// true if at least one Data Block is stored (even if it has 0 useable bytes)
  bool has_data() const { return !blocks_.empty(); }

  /// total length of usable bytes (between rd_ptr and wr_ptr) of all Data Blocks
  size_t data_length() const;

  void clear() { blocks_.clear(); }

  ACE_Message_Block* data(ACE_Allocator* mb_alloc = 0) const;

  /// write the data payload to the Serializer
  bool write_data(Serializer& ser) const;

  /// copy the data payload into an OctetSeq
  DDS::OctetSeq copy_data() const;

  /// @brief Retreive one byte of data from the payload
  /// @param offset must be in the range [0, data_length())
  unsigned char peek(size_t offset) const;

  /// @brief Update this ReceivedDataSample's data payload to include
  /// the prefix's data payload before any existing bytes.
  /// Headers are not modified.
  /// @param prefix the source ReceivedDataSample, its data will be removed and
  /// taken over by this ReceivedDataSample
  void prepend(ReceivedDataSample& prefix);

  /// @brief Update this ReceivedDataSample's data payload to include
  /// the suffix's data payload after any existing bytes.
  /// Headers are not modified.
  /// @param suffix the source ReceivedDataSample, its data will be removed and
  /// taken over by this ReceivedDataSample
  void append(ReceivedDataSample& suffix);

  /// @brief Add passed-in data to payload bytes
  /// @param data start of bytes to add to the payload (makes a copy)
  /// @param size number of bytes to add to the payload
  void append(const char* data, size_t size);

  /// @brief Replace all payload bytes with passed-in data
  /// Based on the ACE_Message_Block(const char*, size_t) constructor, doesn't copy data
  /// @param data start of bytes to use as the payload
  /// @param size number of bytes to use as the payload
  void replace(const char* data, size_t size);

  ReceivedDataSample get_fragment_range(FragmentNumber start_frag, FragmentNumber end_frag = INVALID_FRAGMENT);

private:

  OPENDDS_VECTOR(MessageBlock) blocks_;
};

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif  /* OPENDDS_DCPS_RECEIVEDDATASAMPLE_H */
