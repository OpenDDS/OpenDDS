// -*- C++ -*-
/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef _PACKET_DATA_WRITER_H_
#define _PACKET_DATA_WRITER_H_

extern "C" {

#include "config.h"

#include <glib.h>
#include <gmodule.h>

#include <epan/packet.h>
#include <epan/dissectors/packet-giop.h>
} // extern "C"

#include "tools/dissector/dissector_export.h"
#include "tools/dissector/giop_base.h"
#include "tools/dissector/ws_common.h"

#include "ace/Synch.h"
#include "ace/Hash_Map_Manager.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS
{
  namespace DCPS
  {

    extern "C"
    {
      gboolean explicit_datawriter_callback (tvbuff_t *, packet_info *,
                                             proto_tree *,int *,
                                             ::MessageHeader *, WS_CONST gchar *,
                                             gchar *);
      gboolean heuristic_datawriter_callback (tvbuff_t *, packet_info *,
                                              proto_tree *,int *,
                                              ::MessageHeader *, WS_CONST gchar *,
                                              gchar *);
    }

    class dissector_Export DataWriterRemote_Dissector : public GIOP_Base
    {
    public:
      static DataWriterRemote_Dissector& instance ();

      virtual void init ();
      void register_handoff ();

      static bool add_associations      (::MessageHeader *);

#if 0
      static bool remove_associations   (::MessageHeader *);
      static bool update_incompatible_qos (::MessageHeader *);
      static bool update_subscription_params (::MessageHeader *);
#endif
    private:
      static DataWriterRemote_Dissector instance_;
    };

  } // namespace DCPS
} // namespace OpenDDS


OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif //  _PACKET_DATA_WRITER_H_
