/*
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */
#ifndef OPENDDS_DCPS_RTPS_ASSOCIATION_RECORD_H
#define OPENDDS_DCPS_RTPS_ASSOCIATION_RECORD_H

#include <dds/Versioned_Namespace.h>
#include <dds/DCPS/transport/framework/TransportClient.h>

#ifndef ACE_LACKS_PRAGMA_ONCE
#  pragma once
#endif

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace RTPS {

const int AC_EMPTY = 0;
const int AC_REMOTE_RELIABLE = 1 << 0;
const int AC_REMOTE_DURABLE = 1 << 1;
const int AC_GENERATE_REMOTE_MATCHED_CRYPTO_HANDLE = 1 << 2;
const int AC_SEND_LOCAL_TOKEN = 1 << 3;
const int AC_LOCAL_TOKENS_SENT = 1 << 4;

class BuiltinAssociationRecord {
public:
  BuiltinAssociationRecord(DCPS::TransportClient_rch transport_client,
                           const DCPS::GUID_t& remote_id,
                           int flags)
    : transport_client_(transport_client)
    , remote_id_(remote_id)
    , flags_(flags)
  {}

  const DCPS::GUID_t local_id() const
  {
    return transport_client_->repo_id();
  }

  const DCPS::GUID_t& remote_id() const
  {
    // FUTURE: Remove this and just store the entity id.
    return remote_id_;
  }

  bool remote_reliable() const
  {
    return flags_ & AC_REMOTE_RELIABLE;
  }

  bool remote_durable() const
  {
    return flags_ & AC_REMOTE_DURABLE;
  }

  bool generate_remote_matched_crypto_handle() const
  {
    return flags_ & AC_GENERATE_REMOTE_MATCHED_CRYPTO_HANDLE;
  }

  bool send_local_token() const
  {
    return flags_ & AC_SEND_LOCAL_TOKEN;
  }

  void local_tokens_sent(bool flag)
  {
    if (flag) {
      flags_ |= AC_LOCAL_TOKENS_SENT;
    } else {
      flags_ &= ~AC_LOCAL_TOKENS_SENT;
    }
  }

  bool local_tokens_sent() const
  {
    return flags_ & AC_LOCAL_TOKENS_SENT;
  }

  const DCPS::TransportClient_rch transport_client_;

private:
  const DCPS::GUID_t remote_id_;
  int flags_;
};

class WriterAssociationRecord : public DCPS::RcObject {
public:
  WriterAssociationRecord(DCPS::DataWriterCallbacks_wrch callbacks,
                          const DCPS::GUID_t& writer_id,
                          const DCPS::ReaderAssociation& reader_association)
    : callbacks_(callbacks)
    , writer_id_(writer_id)
    , reader_association_(reader_association)
  {}

  const DCPS::GUID_t& writer_id() const { return writer_id_; }
  const DCPS::GUID_t& reader_id() const { return reader_association_.readerId; }

  const DCPS::DataWriterCallbacks_wrch callbacks_;
  const DCPS::GUID_t writer_id_;
  const DCPS::ReaderAssociation reader_association_;
};
typedef DCPS::RcHandle<WriterAssociationRecord> WriterAssociationRecord_rch;

class ReaderAssociationRecord : public DCPS::RcObject {
public:
  ReaderAssociationRecord(DCPS::DataReaderCallbacks_wrch callbacks,
                          const DCPS::GUID_t& reader_id,
                          const DCPS::WriterAssociation& writer_association)
    : callbacks_(callbacks)
    , reader_id_(reader_id)
    , writer_association_(writer_association)
  {}

  const DCPS::GUID_t& reader_id() const { return reader_id_; }
  const DCPS::GUID_t& writer_id() const { return writer_association_.writerId; }

  const DCPS::DataReaderCallbacks_wrch callbacks_;
  const DCPS::GUID_t reader_id_;
  const DCPS::WriterAssociation writer_association_;
};
typedef DCPS::RcHandle<ReaderAssociationRecord> ReaderAssociationRecord_rch;

}
}

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif // OPENDDS_DCPS_RTPS_ASSOCIATION_RECORD_H
