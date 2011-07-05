/*
 * $Id$
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "tools/dissector/sample_base.h"

#include <ace/Basic_Types.h>
#include <ace/CDR_Base.h>
#include <ace/Message_Block.h>
#include <ace/Log_Msg.h>
#include <ace/ACE.h>

#include <cstring>

#include <algorithm>
#include <iomanip>
#include <sstream>
#include <string>

namespace OpenDDS
{
  namespace DCPS
  {

    Sample_Base::Sample_Base (const char *type_id)
      :typeId_ (0),
       publication_()
    {
      size_t len = ACE_OS::strlen(type_id);
      if (len > 0)
        {
          this->typeId_ = new char[len+1];
          ACE_OS::strcpy (this->typeId_, type_id);
        }
    }

    Sample_Base::~Sample_Base ()
    {
      delete [] this->typeId_;
    }

    const char *
    Sample_Base::typeId() const
    {
      return this->typeId_;
    }

    const DCPS::RepoId&
    Sample_Base::publication () const
    {
      return this->publication_;
    }

    void
    Sample_Base::publication (const RepoId& pub)
    {
      ACE_OS::memcpy (&this->publication_, &pub, sizeof(RepoId));
    }

    void
    Sample_Dissector_Manager::add (Sample_Base &d)
    {
#if 0
      dds_sub_handle_t *subh;

      subh = g_malloc(sizeof (giop_sub_handle_t));

      subh->sub_name = name;
      subh->sub_fn = sub;
      subh->sub_proto = find_protocol_by_id(sub_proto);     /* protocol_t for sub dissectors's proto_register_protocol() */

      giop_sub_list = g_slist_append (giop_sub_list, subh);
#endif

      const char *key = d.typeId();
      ACE_DEBUG ((LM_DEBUG,"Adding new dissector for %s\n",key));

      dissectors_.bind(key,&d);
    }

    Sample_Base *
    Sample_Dissector_Manager::find (const char *data_name)
    {
      Sample_Base *result = 0;
      dissectors_.find (data_name, result);
      return result;
    }

  }
}
