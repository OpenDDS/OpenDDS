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

    Sample_Dissector_Manager 
    Sample_Dissector_Manager::instance_;

    Sample_Dissector_Manager &
    Sample_Dissector_Manager::instance () 
    {
      return instance_;
    }

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
    Sample_Dissector_Manager::init ()
    {
      Sample_Base *dummy = new LocationInfo_Dissector;
      dummy = new PlanInfo_Dissector;
      dummy = new MoreInfo_Dissector;
      dummy = new UnrelatedInfo_Dissector;
      dummy = new Resulting_Dissector;

      ACE_UNUSED_ARG (dummy);
    };

    void
    Sample_Dissector_Manager::add (Sample_Base &d)
    {
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

    //--------------------------------------------------------------------

    void LocationInfo_Dissector::dissect (tvbuff_t *tvb,
                                          packet_info *pinfo,
                                          proto_tree *tree,
                                          gint &offset)
    {
      ACE_DEBUG ((LM_DEBUG, "LocationInfo_Dissector::dissect\n"));
    }

    void PlanInfo_Dissector::dissect (tvbuff_t *tvb,
                                          packet_info *pinfo,
                                          proto_tree *tree,
                                          gint &offset)
    {
      ACE_DEBUG ((LM_DEBUG, "PlanInfo_Dissector::dissect\n"));
    }

    void MoreInfo_Dissector::dissect (tvbuff_t *tvb,
                                          packet_info *pinfo,
                                          proto_tree *tree,
                                          gint &offset)
    {
      ACE_DEBUG ((LM_DEBUG, "MoreInfo_Dissector::dissect\n"));
    }

    void UnrelatedInfo_Dissector::dissect (tvbuff_t *tvb,
                                          packet_info *pinfo,
                                          proto_tree *tree,
                                          gint &offset)
    {
      ACE_DEBUG ((LM_DEBUG, "UnrelatedInfo_Dissector::dissect\n"));
    }

    void Resulting_Dissector::dissect (tvbuff_t *tvb,
                                          packet_info *pinfo,
                                          proto_tree *tree,
                                          gint &offset)
    {
      ACE_DEBUG ((LM_DEBUG, "Resulting_Dissector::dissect\n"));
    }

  }
}
