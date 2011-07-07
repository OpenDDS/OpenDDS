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


    Sample_Field::Sample_Field (const char *f, gint l)
      : data_ (0),
        format_(f),
        len_(l),
        nested_ (0),
        next_(0)
    {
    }

    Sample_Field::Sample_Field (const char *f, Sample_Base *n)
      : data_ (0),
        format_(f),
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

    guint
    Sample_Field::length ()
    {
      return this->nested_ ? this->nested_->compute_length(this->data_) : this->len_;
    }

    //------------------------------------------------------------------------

    Sample_Base::Sample_Base (const char *type_id)
      :fixed_length_ (0),
       ett_payload_ (-1),
       proto_ (-1),
       use_subtree_ (true),
       typeId_ (0),
       field_ (0),
       last_field_ (0)
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
      delete field_;
    }

    const char *
    Sample_Base::typeId() const
    {
      return this->typeId_;
    }

    void
    Sample_Base::add_field (Sample_Field *f)
    {
      if (this->field_ == 0)
        {
          this->field_ = f;
        }
      else
        {
          this->last_field_->next_ = f;
        }
      this->last_field_ = f;
      fixed_length_ += f->len_;
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

      if (use_subtree_)
        {
          proto_item *item =
            proto_tree_add_item (tree, proto_, tvb, offset, -1, 0);
          tree = proto_item_add_subtree (item, ett_payload_);
        }

      for (Sample_Field *sf = field_; sf != 0; sf = sf->next_)
        {
          if (sf->nested_ == 0)
            {
              proto_tree_add_text (tree, tvb, offset + data_pos, sf->len_,
                                   sf->format_, *(data + data_pos));
              data_pos += sf->length();
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
      if (tvb->length - offset < this->fixed_length_)
        return; // error! length is not sufficient for this type.

      offset += this->dissect_i (tvb, pinfo, tree, offset);
    }

    guint
    Sample_Base::compute_length (const char *data)
    {

      this->fixed_length_ = 0;
      for (Sample_Field *sf = field_; sf != 0; sf = sf->next_)
        {
          sf->data_ = data +  this->fixed_length_;
          this->fixed_length_ += sf->length();
        }
      return this->fixed_length_;
    }

    guint
    Sample_Base::length () const
    {
      return this->fixed_length_;
    }


    //----------------------------------------------------------------------

    LocationInfo_Dissector::LocationInfo_Dissector ()
      : Sample_Base ("IDL:LocationInfoTypeSupport:1.0")
    {
      Sample_Dissector_Manager::instance().add (*this);

      this->add_field (new Sample_Field ("flight_id1: %u", 4));
      this->add_field (new Sample_Field ("flight_id2: %u", 4));
      this->add_field (new Sample_Field ("x: %d", 4));
      this->add_field (new Sample_Field ("y: %d", 4));
      this->add_field (new Sample_Field ("z: %d", 4));

      gint *ett[] = {
        &ett_payload_
      };

      proto_ =
        proto_register_protocol
        ("Sample Payload: LocationInfo",
         "LocationInfo",
         "locationinfo");
      proto_register_subtree_array(ett, array_length(ett));

    }

  }
}
