// -*- C++ -*-
/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef _GIOP_BASE_H_
#define _GIOP_BASE_H_

extern "C" {

#include <config.h>

#include <glib.h>
#include <gmodule.h>

#include <epan/packet.h>
#include <epan/conversation.h>
#include <epan/dissectors/packet-giop.h>
} // extern "C"

#include "tools/dissector/dissector_export.h"
#include "dds/DdsDcpsInfoUtilsC.h"

#include <map>
#include <string>

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS
{
  namespace DCPS
  {
    typedef bool (*DecodeFN)(::MessageHeader*);
    typedef std::map<std::string, DecodeFN> DecodeFN_Map;

    class dissector_Export GIOP_Base
    {
    public:
      virtual ~GIOP_Base() {}

      int dissect_giop (MessageHeader *header,
                        const gchar *operation,
                        gchar *idlname);
      gboolean dissect_heur (MessageHeader *header, const gchar *operation);

      void setPacket (tvbuff_t* buf, packet_info* pi, proto_tree* pt, int *poff)
      {
        tvb_ = buf; pinfo_ = pi; tree_ = pt; offset_ = poff;
      }

      conversation_t *find_conversation ();

      void add_ulong (int fieldId, proto_tree *subtree = 0);
      const RepoId *add_repo_id (int fieldId, proto_tree *subtree = 0);

      const char * add_string (int fieldId, proto_tree *subtree = 0);
      TopicStatus add_topic_status (int fieldId, proto_tree *subtree = 0);

      void add_trans_info (int fieldId, int ett, proto_tree *subtree = 0);
      void add_domain_qos (int fieldId, int ett, proto_tree *subtree = 0);
      void add_topic_qos (int fieldId, int ett, proto_tree *subtree = 0);
      void add_writer_qos (int fieldId, int ett, proto_tree *subtree = 0);
      void add_publisher_qos (int fieldId, int ett, proto_tree *subtree = 0);

      void add_exception (int fieldId, const char *exception_id);

      void fix_reqid (MessageHeader *header);

      void start_decoding ();
      virtual void init ();

    protected:
      char *proto_label_;
      char *repo_id_;

      int proto_id_;

      tvbuff_t *tvb_;
      packet_info *pinfo_;
      proto_tree *tree_;
      int *offset_;

      DecodeFN_Map op_functions_;
      DecodeFN find_giop_decoder (const gchar *opname);
      void add_giop_decoder (const char *opname, DecodeFN decoder);
      void init_proto_label (const char *proto);
      void init_repo_id (const char *repoid);

      char * current_op_;
      bool is_big_endian_;

    private:
      static bool initialized_;

      static int ett_base_;
      static int hf_; // add in base defined header fields here
    };


  }
}

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif //  _GIOP_BASE_H_
