/*
 * $Id$
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "tools/dissector/sample_dissector.h"
#include "tools/dissector/sample_manager.h"

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

    Sample_Field::Sample_Field (FixedTypeID id, const char *label)
      : format_ (label),
        len_ (0),
        type_id_ (id),
        nested_ (0),
        next_(0)
    {
    }

    Sample_Field::Sample_Field (const char *f, gint l)
      : format_(f),
        len_(l),
        type_id_ (Undefined),
        nested_ (0),
        next_(0)
    {
    }

    Sample_Field::Sample_Field (Sample_Dissector *n)
      : format_(""),
        len_(0),
        type_id_ (Undefined),
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

    Sample_Field *
    Sample_Field::chain (const char *f, gint l)
    {
      return chain (new Sample_Field (f,l));
    }

    Sample_Field *
    Sample_Field::chain (FixedTypeID ti, const char *l)
    {
      return chain (new Sample_Field (ti,l));
    }

    Sample_Field *
    Sample_Field::chain (Sample_Dissector *n)
    {
      return chain (new Sample_Field (n));
    }

    Sample_Field *
    Sample_Field::get_link (size_t index)
    {
      if (index == 0)
        return this;

      return (next_ == 0) ? 0 : next_->get_link (index - 1);
    }

    size_t
    Sample_Field::compute_length (guint8 *data)
    {
      size_t len = 0;
      switch (this->type_id_)
        {
        case Char:
          {
            len = sizeof (ACE_CDR::Char);
            break;
          }
        case Bool:
          {
            len = sizeof(ACE_CDR::Boolean);
            break;
          }
        case Octet:
          {
            len = sizeof(ACE_CDR::Octet);
            break;
          }
        case WChar:
          {
            len = sizeof(ACE_CDR::WChar);
            break;
          }
        case Short:
          {
            len = sizeof(ACE_CDR::Short);
            break;
          }
        case UShort:
          {
            len = sizeof(ACE_CDR::UShort);
            break;
          }
        case Long:
          {
            len = sizeof(ACE_CDR::Long);
            break;
          }
        case ULong:
          {
            len = sizeof(ACE_CDR::ULong);
            break;
          }
        case LongLong:
          {
            len = sizeof(ACE_CDR::LongLong);
            break;
          }
        case ULongLong:
          {
            len = sizeof(ACE_CDR::ULongLong);
            break;
          }
        case Float:
          {
            len = sizeof(ACE_CDR::Float);
            break;
          }
        case Double:
          {
            len = sizeof(ACE_CDR::Double);
            break;
          }
        case LongDouble:
          {
            len = sizeof(ACE_CDR::LongDouble);
            break;
          }
        case Undefined:
          {
            len =
              (nested_ == 0) ? this->len_ : this->nested_->compute_length (data);
          }
        }
      if (next_ != 0)
        len += next_->compute_length (data+len);
      return len;
    }

    template <typename T>
    void
    to_stream (std::stringstream &outstream, size_t &len, guint8 *data)
    {
      len = sizeof (T);
      T *c = reinterpret_cast <T *>(data);
      outstream << *c << std::ends;
    }

    size_t
    Sample_Field::dissect_i (tvbuff_t *tvb, packet_info* pinfo,
                             proto_tree *tree,
                             gint offset, guint8 *data)
    {
      size_t len = 0;
      if (this->type_id_ != Undefined)
        {
          std::stringstream outstream;
          outstream << this->format_ << ": ";
          switch (this->type_id_)
            {
            case Char:
              {
                to_stream<ACE_CDR::Char>(outstream, len, data);
                break;
              }
            case Bool:
              {
                to_stream<ACE_CDR::Boolean>(outstream, len, data);
                break;
              }
            case Octet:
              {
                to_stream<ACE_CDR::Octet>(outstream, len, data);
                break;
              }
            case WChar:
              {
                to_stream<ACE_CDR::WChar>(outstream, len, data);
                break;
              }
            case Short:
              {
                to_stream<ACE_CDR::Short>(outstream, len, data);
                break;
              }
            case UShort:
              {
                to_stream<ACE_CDR::UShort>(outstream, len, data);
                break;
              }
            case Long:
              {
                to_stream<ACE_CDR::Long>(outstream, len, data);
                break;
              }
            case ULong:
              {
                to_stream<ACE_CDR::ULong>(outstream, len, data);
                break;
              }
            case LongLong:
              {
                to_stream<ACE_CDR::LongLong>(outstream, len, data);
                break;
              }
            case ULongLong:
              {
                to_stream<ACE_CDR::ULongLong>(outstream, len, data);
                break;
              }
            case Float:
              {
                to_stream<ACE_CDR::Float>(outstream, len, data);
                break;
              }
            case Double:
              {
                to_stream<ACE_CDR::Double>(outstream, len, data);
                break;
              }
            case LongDouble:
              {
                to_stream<ACE_CDR::LongDouble>(outstream, len, data);
                break;
              }
            case Undefined:
              {
                outstream << "type undefined" << std::ends;
                len = 1;
              }
            }
          std::string buffer = outstream.str();
          proto_tree_add_text (tree, tvb, offset, len, "%s", buffer.c_str());
        }
      else if (nested_ == 0)
        {
          proto_tree_add_text (tree, tvb, offset, len_, format_, *(data));
          len = this->len_;
        }
      else
        {
          len = this->nested_->dissect_i (tvb, pinfo, tree, offset);
        }

      offset += len;
      data += len;
      if (next_ != 0)
        len += next_->dissect_i (tvb, pinfo, tree, offset, data);
      return len;
    }

    //------------------------------------------------------------------------

    Sample_Dissector::Sample_Dissector ()
      :ett_payload_ (-1),
       label_ (0),
       typeId_ (0),
       field_ (0)
    {
    }

    Sample_Dissector::~Sample_Dissector ()
    {
      delete [] this->typeId_;
      delete field_;
    }

    const char *
    Sample_Dissector::typeId() const
    {
      return this->typeId_;
    }

    void
    Sample_Dissector::init (const char *type_id,
                       const char *subtree_label)
    {
      size_t len = ACE_OS::strlen(type_id);
      if (len > 0)
        {
          this->typeId_ = new char[len+1];
          ACE_OS::strcpy (this->typeId_, type_id);
          Sample_Manager::instance().add (*this);
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
    Sample_Dissector::add_field (Sample_Field *f)
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

    Sample_Field *
    Sample_Dissector::add_field (const char *f, gint l)
    {
      return add_field (new Sample_Field (f,l));
    }

    Sample_Field *
    Sample_Dissector::add_field (Sample_Field::FixedTypeID type_id,
                                 const char *label)
    {
      return add_field (new Sample_Field (type_id, label));
    }

    Sample_Field *
    Sample_Dissector::add_field (Sample_Dissector *n)
    {
      return add_field (new Sample_Field (n));
    }

    size_t
    Sample_Dissector::dissect_i (tvbuff_t *tvb,
                            packet_info *pinfo,
                            proto_tree *tree,
                            gint offset)
    {
      size_t data_pos = 0;

      gint remainder = tvb->length - offset;
      guint8* data = (guint8*)ep_tvb_memdup(tvb, offset, remainder);

      guint32 len = field_->compute_length (data);

      if (this->label_ != 0)
        {
          proto_item *item =
            proto_tree_add_text (tree, tvb, offset, len,
                                 "Sample Payload: %s", label_);
          tree = proto_item_add_subtree (item, ett_payload_);
        }

      if (field_ != 0)
        data_pos += field_->dissect_i (tvb, pinfo, tree, offset, data);

      return data_pos;
    }

    size_t
    Sample_Dissector::compute_length (guint8 *data)
    {

      return field_->compute_length (data);
    }

    void
    Sample_Dissector::dissect (tvbuff_t *tvb,
                          packet_info *pinfo,
                          proto_tree *tree,
                          gint &offset)
    {
      offset += this->dissect_i (tvb, pinfo, tree, offset);
    }

    //----------------------------------------------------------------------

    Sample_String::Sample_String (const char *label)
    {
      size_t len = label != 0 ? ACE_OS::strlen(label) : 0;
      this->label_ = new char[len+1];
      if (len > 0)
        ACE_OS::strcpy (this->label_, label);
    }

    size_t
    Sample_String::dissect_i (tvbuff_t *tvb,
                              packet_info *,
                              proto_tree *tree,
                              gint offset)
    {
      gint remainder = tvb->length - offset;
      char * data = (char *)ep_tvb_memdup(tvb, offset, remainder);
      guint32 len = *(reinterpret_cast< guint32 * >(data));

      char * clone = new char[len + 1];
      ACE_OS::memcpy (clone, data+4, len);
      clone[len] = '\0';
      proto_tree_add_text (tree, tvb, offset, (len) + 4,
                           "%s: %s", this->label_, clone);
      delete [] clone;

      return len + 4;
    }

    size_t
    Sample_String::compute_length (guint8 *data)
    {
      return 4 + *(reinterpret_cast< guint32 * >(data));
    }


    //----------------------------------------------------------------------

    Sample_WString::Sample_WString (const char *label)
    {
      size_t len = label != 0 ? ACE_OS::strlen(label) : 0;
      this->label_ = new char[len+1];
      if (len > 0)
        ACE_OS::strcpy (this->label_, label);
    }

    size_t
    Sample_WString::dissect_i (tvbuff_t *tvb,
                               packet_info *,
                               proto_tree *tree,
                               gint offset)
    {
      gint remainder = tvb->length - offset;
      char * data = (char *)ep_tvb_memdup(tvb, offset, remainder);
      guint32 len =
        *(reinterpret_cast< guint32 * >(data)) * sizeof (ACE_CDR::WChar);

      wchar_t * clone = new wchar_t[len + 1];
      ACE_OS::memcpy (clone, data+4, len);
      clone[len] = '\0';
      proto_tree_add_text (tree, tvb, offset, (len) + 4,
                           "%s: %ls", this->label_, clone);
      delete [] clone;

      return len + 4;
    }

    size_t
    Sample_WString::compute_length (guint8 *data)
    {
      return 4 + *(reinterpret_cast< guint32 * >(data)) * sizeof (ACE_CDR::WChar);
    }



    //----------------------------------------------------------------------

    Sample_Sequence::Sample_Sequence (const char *label)
      : element_ (0)
    {
      this->init ("",label);
      element_ = new Sample_Dissector;
    }

    Sample_Sequence::~Sample_Sequence ()
    {
      delete element_;
    }

    Sample_Dissector *
    Sample_Sequence::element ()
    {
      return element_;
    }

    size_t
    Sample_Sequence::dissect_i (tvbuff_t *tvb,
                                packet_info *pinfo,
                                proto_tree *tree,
                                gint offset)
    {
      gint remainder = tvb->length - offset;
      guint8 *data = (guint8 *)ep_tvb_memdup(tvb, offset, remainder);
      guint32 len = compute_length (data);
      guint32 count = *(reinterpret_cast< guint32 * >(data));
      if (this->label_ != 0)
        {
          proto_item *item =
            proto_tree_add_text (tree, tvb, offset, len,
                                 "%s [%d]", label_, count);
          tree = proto_item_add_subtree (item, ett_payload_);
        }

      size_t data_pos = 4;
      for (guint32 i = 0; i < count; i++)
        {
          data_pos += element_->dissect_i (tvb, pinfo, tree,
                                           offset + data_pos);
        }
      return data_pos;
    }

    size_t
    Sample_Sequence::compute_length (guint8 *data)
    {

      size_t len = 4;
      guint32 count = *(reinterpret_cast< guint32 * >(data));
      data += 4;
      while (count > 0)
        {
          size_t elen = element_->compute_length(data);
          data += elen;
          len += elen;
          --count;
        }

      return len;
    }



    //----------------------------------------------------------------------

    Sample_Array::Sample_Array (const char *label,size_t count)
      : Sample_Sequence (label),
        count_(count)
    {
    }

    size_t
    Sample_Array::dissect_i (tvbuff_t *tvb,
                             packet_info *pinfo,
                             proto_tree *tree,
                             gint offset)
    {

      gint remainder = tvb->length - offset;
      guint8 * data = (guint8 *)ep_tvb_memdup(tvb, offset, remainder);
      size_t len = compute_length (data);

      if (this->label_ != 0)
        {
          proto_item *item =
            proto_tree_add_text (tree, tvb, offset, len, "%s", label_);
          tree = proto_item_add_subtree (item, ett_payload_);
        }

      size_t data_pos = 0;
      for (size_t i = 0; i < count_; i++)
        {
          data_pos += element_->dissect_i (tvb, pinfo, tree,
                                           offset + data_pos );
        }
      return data_pos;
    }

    size_t
    Sample_Array::compute_length (guint8 *data)
    {
      size_t len = 0;
      for (guint32 i = 0; i < count_; i++)
        {
          size_t elen = element_->compute_length(data);
          data += elen;
          len += elen;
        }

      return len;
    }

    //----------------------------------------------------------------------

    Sample_Enum::Sample_Enum (const char *label)
      : name_ (0)
    {
      size_t len = ACE_OS::strlen(label);
      if (len > 0)
        {
          this->label_ = new char[len+1];
          ACE_OS::strcpy (this->label_, label);
        }
    }

    Sample_Enum::~Sample_Enum ()
    {
      delete name_;
    }

    Sample_Field *
    Sample_Enum::add_name (const char *name)
    {
      return (name_ == 0) ?
        (name_ = new Sample_Field (name, 4)) :
        name_->chain (new Sample_Field (name, 4));
    }

    size_t
    Sample_Enum::dissect_i (tvbuff_t *tvb,
                            packet_info *,
                            proto_tree *tree,
                            gint offset)
    {
      size_t len = 4;
      char * data = (char *)ep_tvb_memdup(tvb, offset, len);
      guint32 value = *(reinterpret_cast< guint32 * >(data));

      Sample_Field *sf = name_->get_link (value);

      proto_tree_add_text (tree, tvb, offset, len,
                           "%s: %s", label_, sf->format_);
      return len;
    }

     size_t
    Sample_Enum::compute_length (guint8 *)
    {
      return 4;
    }


  }
}
