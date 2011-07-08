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

    void
    Sample_Dissector_Manager::init ()
    {
      Sample_Base *dummy = new LocationInfo_Dissector;
      dummy = new PlanInfo_Dissector;
//       dummy = new MoreInfo_Dissector;
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


    Sample_Field::Sample_Field (const char *f, gint l)
      : format_(f),
        len_(l),
        nested_ (0),
        next_(0)
    {
    }

    Sample_Field::Sample_Field (Sample_Base *n)
      : format_(""),
        len_(0),
        nested_ (n),
        next_(0)
    {
    }

    Sample_Field::~Sample_Field ()
    {
      delete nested_;
      delete next_;
    }

    Sample_Field *
    Sample_Field::chain (Sample_Field *f)
    {
      if (next_ != 0)
        next_->chain (f);
      else
        next_ = f;
      return f;
    }


    //------------------------------------------------------------------------

    Sample_Base::Sample_Base ()
      :ett_payload_ (-1),
       label_ (0),
       typeId_ (0),
       field_ (0)
    {
    }

    Sample_Base::~Sample_Base ()
    {
      delete [] this->typeId_;
      delete field_;
    }

    const char *
    Sample_Base::typeId() const
    {
      return this->typeId_;
    }

    void
    Sample_Base::init (const char *type_id,
                       const char *subtree_label)
    {
      size_t len = ACE_OS::strlen(type_id);
      if (len > 0)
        {
          this->typeId_ = new char[len+1];
          ACE_OS::strcpy (this->typeId_, type_id);
          Sample_Dissector_Manager::instance().add (*this);
        }

      len = ACE_OS::strlen(subtree_label);
      if (len > 0)
        {
          this->label_ = new char[len+1];
          ACE_OS::strcpy (this->label_, subtree_label);

          gint *ett[] = {
            &ett_payload_
          };
          proto_register_subtree_array(ett, array_length(ett));
        }
    }

    Sample_Field *
    Sample_Base::add_field (Sample_Field *f)
    {
      if (this->field_ == 0)
        {
          this->field_ = f;
        }
      else
        {
          this->field_->chain (f);
        }
      return f;
    }

    size_t
    Sample_Base::dissect_i (tvbuff_t *tvb,
                            packet_info *pinfo,
                            proto_tree *tree,
                            gint offset)
    {
      size_t data_pos = 0;

      gint remainder = tvb->length - offset;
      guint8* data = (guint8*)ep_tvb_memdup(tvb, offset, remainder);

      if (this->label_ != 0)
        {
          proto_item *item =
            proto_tree_add_text (tree, tvb, offset, -1,
                                 "Sample Payload: %s", label_);
          tree = proto_item_add_subtree (item, ett_payload_);
        }

      for (Sample_Field *sf = field_; sf != 0; sf = sf->next_)
        {
          if (sf->nested_ == 0)
            {
              proto_tree_add_text (tree, tvb, offset + data_pos, sf->len_,
                                   sf->format_, *(data + data_pos));
              data_pos += sf->len_;
            }
          else
            {
              data_pos +=
                sf->nested_->dissect_i (tvb, pinfo, tree, offset + data_pos);
            }
        }

      return data_pos;
    }

    void
    Sample_Base::dissect (tvbuff_t *tvb,
                          packet_info *pinfo,
                          proto_tree *tree,
                          gint &offset)
    {
      offset += this->dissect_i (tvb, pinfo, tree, offset);
    }

    //----------------------------------------------------------------------

    Sample_String::Sample_String (const char *label)
    {
      size_t len = ACE_OS::strlen(label);
      if (len > 0)
        {
          this->label_ = new char[len+1];
          ACE_OS::strcpy (this->label_, label);
        }
    }

    size_t
    Sample_String::dissect_i (tvbuff_t *tvb,
                              packet_info *,
                              proto_tree *tree,
                              gint offset)
    {
      gint remainder = tvb->length - offset;
      char * data = (char *)ep_tvb_memdup(tvb, offset, remainder);
      guint32 *len = (reinterpret_cast< guint32 * >(data));
      char * clone = new char[*len + 1];
      ACE_OS::memcpy (clone, data+4, *len);
      clone[*len] = '\0';
      proto_tree_add_text (tree, tvb, offset, (*len) + 4,
                           "%s: %s", this->label_, clone);
      delete [] clone;

      return (*len) + 4;
    }

#if 0
    Sample_Sequence::Sample_Sequence (const char *label, Sample_Base *element)
      : element_ (element)
    {
      size_t len = ACE_OS::strlen(label);
      if (len > 0)
        {
          this->label_ = new char[len+1];
          ACE_OS::strcpy (this->label_, label);
        }


    }

    protected:
      virtual size_t dissect_i (tvbuff_t *, packet_info *, proto_tree *, gint);

      Sample_Base *element_;
    };

#endif


    //----------------------------------------------------------------------

    LocationInfo_Dissector::LocationInfo_Dissector ()
    {

      this->init ("IDL:LocationInfoTypeSupport:1.0",
                  "LocationInfo");
#if 0
      this->add_field (new Sample_Field ("flight_id1: %u", 4));
      this->add_field (new Sample_Field ("flight_id2: %u", 4));
      this->add_field (new Sample_Field ("x: %d", 4));
      this->add_field (new Sample_Field ("y: %d", 4));
      this->add_field (new Sample_Field ("z: %d", 4));
#else
      Sample_Field *f =
        this->add_field (new Sample_Field ("flight_id1: %u", 4));
      f = f->chain (new Sample_Field ("flight_id2: %u", 4));
      f = f->chain (new Sample_Field ("x: %d", 4));
      f = f->chain (new Sample_Field ("y: %d", 4));
      f = f->chain (new Sample_Field ("z: %d", 4));
#endif

    }

    PlanInfo_Dissector::PlanInfo_Dissector ()
    {
      this->init ("IDL:PlanInfoTypeSupport:1.0",
                  "PlanInfo");

      this->add_field (new Sample_Field ("flight_id1: %u", 4));
      this->add_field (new Sample_Field ("flight_id2: %u", 4));
      this->add_field (new Sample_Field (new Sample_String ("flight_name")));
      this->add_field (new Sample_Field (new Sample_String ("tailno")));
    }

    MoreInfo_Dissector::MoreInfo_Dissector ()
    {

      this->init ("IDL:MoreInfoTypeSupport:1.0",
                  "MoreInfo");

      this->add_field (new Sample_Field ("flight_id1: %u", 4));
      this->add_field (new Sample_Field (new Sample_String ("more")));
    }

    UnrelatedInfo_Dissector::UnrelatedInfo_Dissector ()
    {


      this->init ("IDL:UnrelatedInfoTypeSupport:1.0",
                  "UnrelatedInfo");

     this->add_field (new Sample_Field (new Sample_String ("misc")));

    }

    Resulting_Dissector::Resulting_Dissector ()
    {
      this->init ("IDL:ResultingTypeSupport:1.0",
                  "Resulting");

      this->add_field (new Sample_Field ("flight_id1: %u", 4));
      this->add_field (new Sample_Field ("flight_id2: %u", 4));
      this->add_field (new Sample_Field (new Sample_String ("flight_name")));
      this->add_field (new Sample_Field ("x: %d", 4));
      this->add_field (new Sample_Field ("y: %d", 4));
      this->add_field (new Sample_Field ("height: %d", 4));
      this->add_field (new Sample_Field (new Sample_String ("more")));
      this->add_field (new Sample_Field (new Sample_String ("misc")));

    }

  }
}
