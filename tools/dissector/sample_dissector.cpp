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

    guint8 *
    Wireshark_Bundle::get_remainder ()
    {
      gint remainder = tvb->length - offset;
      return reinterpret_cast<guint8 *>(ep_tvb_memdup(tvb, offset, remainder));
    }

    Sample_Field::Sample_Field (FixedTypeID id, const char *label)
      : label_ (label),
        type_id_ (id),
        nested_ (0),
        next_(0)
    {
    }

    Sample_Field::Sample_Field (Sample_Dissector *n, const char *label)
      : label_(label),
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
    Sample_Field::chain (FixedTypeID ti, const char *l)
    {
      return chain (new Sample_Field (ti, l));
    }

    Sample_Field *
    Sample_Field::chain (Sample_Dissector *n, const char *l)
    {
      return chain (new Sample_Field (n, l));
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
        case Enumeration:
          {
            len = 4;
            break;
          }
        case Undefined:
          {
            len =
              (nested_ == 0) ? 0 : this->nested_->compute_length (data);
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
    Sample_Field::dissect_i (Wireshark_Bundle_Field &params)
    {
      size_t len = 0;
      if (this->nested_ == 0)
        {
          std::stringstream outstream;
          outstream << this->label_;
          if (params.use_index)
            outstream << "[" << params.index << "]";
          outstream << ": ";
          switch (this->type_id_)
            {
            case Char:
              {
                to_stream<ACE_CDR::Char>(outstream, len, params.data);
                break;
              }
            case Bool:
              {
                to_stream<ACE_CDR::Boolean>(outstream, len, params.data);
                break;
              }
            case Octet:
              {
                to_stream<ACE_CDR::Octet>(outstream, len, params.data);
                break;
              }
            case WChar:
              {
                to_stream<ACE_CDR::WChar>(outstream, len, params.data);
                break;
              }
            case Short:
              {
                to_stream<ACE_CDR::Short>(outstream, len, params.data);
                break;
              }
            case UShort:
              {
                to_stream<ACE_CDR::UShort>(outstream, len, params.data);
                break;
              }
            case Long:
              {
                to_stream<ACE_CDR::Long>(outstream, len, params.data);
                break;
              }
            case ULong:
              {
                to_stream<ACE_CDR::ULong>(outstream, len, params.data);
                break;
              }
            case LongLong:
              {
                to_stream<ACE_CDR::LongLong>(outstream, len, params.data);
                break;
              }
            case ULongLong:
              {
                to_stream<ACE_CDR::ULongLong>(outstream, len, params.data);
                break;
              }
            case Float:
              {
                to_stream<ACE_CDR::Float>(outstream, len, params.data);
                break;
              }
            case Double:
              {
                to_stream<ACE_CDR::Double>(outstream, len, params.data);
                break;
              }
            case LongDouble:
              {
                to_stream<ACE_CDR::LongDouble>(outstream, len, params.data);
                break;
              }
            case Enumeration:
              {
                break; // only the label is used, not directly presented
              }
            case Undefined:
              {
                outstream << "type undefined" << std::ends;
                len = 1;
              }
            }
          std::string buffer = outstream.str();
          proto_tree_add_text (params.tree, params.tvb, params.offset,
                               len, "%s", buffer.c_str());
        }
      else
        {
          len = this->nested_->dissect_i (params, this->label_);
        }

      params.offset += len;
      params.data += len;
      if (next_ != 0)
        len += next_->dissect_i (params);
      return len;
    }

    //------------------------------------------------------------------------

    Sample_Dissector::Sample_Dissector ()
      :ett_payload_ (-1),
       subtree_label_ (0),
       typeId_ (0),
       field_ (0)
    {
    }

    Sample_Dissector::~Sample_Dissector ()
    {
      delete [] this->typeId_;
      delete [] this->subtree_label_;
      delete field_;
    }

    const char *
    Sample_Dissector::typeId() const
    {
      return this->typeId_;
    }

    void
    Sample_Dissector::init (const char *type_id,
                       const char *subtree)
    {
      size_t len = ACE_OS::strlen(type_id);
      if (len > 0)
        {
          this->typeId_ = new char[len+1];
          ACE_OS::strcpy (this->typeId_, type_id);
          Sample_Manager::instance().add (*this);
        }

      len = ACE_OS::strlen(subtree);
      if (len > 0)
        {
          this->subtree_label_ = new char[len+1];
          ACE_OS::strcpy (this->subtree_label_, subtree);

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
    Sample_Dissector::add_field (Sample_Field::FixedTypeID type_id,
                                 const char *label)
    {
      return add_field (new Sample_Field (type_id, label));
    }

    Sample_Field *
    Sample_Dissector::add_field (Sample_Dissector *n, const char *label)
    {
      return add_field (new Sample_Field (n, label));
    }

    size_t
    Sample_Dissector::dissect_i (Wireshark_Bundle_i &params,
                                 const char *label)

    {
      size_t data_pos = 0;

      guint8* data = params.get_remainder();
      guint32 len = field_->compute_length (data);
      std::stringstream outstream;
      bool top_level = true;
      if (label != 0 && ACE_OS::strlen (label) > 0)
        {
          top_level = false;
          outstream << label;
          if (params.use_index)
            outstream << "[" << params.index << "]";
        }
      proto_tree *subtree = params.tree;
      if (this->subtree_label_ != 0)
        {
          if (top_level)
            outstream << "Sample Payload: ";
          else
            outstream << " : ";
          outstream << subtree_label_;
          std::string buffer = outstream.str();
          proto_item *item =
            proto_tree_add_text (params.tree, params.tvb, params.offset, len,
                                 "%s", buffer.c_str());
          subtree = proto_item_add_subtree (item, ett_payload_);
          params.use_index = false;
        }

      if (field_ != 0)
        {
          Wireshark_Bundle_Field fp;
          fp.tvb = params.tvb;
          fp.info = params.info;
          fp.tree = subtree;
          fp.offset = params.offset;
          fp.use_index = params.use_index;
          fp.index = params.index;
          fp.data = data;
          data_pos += field_->dissect_i (fp);
        }

      return data_pos;
    }

    size_t
    Sample_Dissector::compute_length (guint8 *data)
    {

      return field_->compute_length (data);
    }

    gint
    Sample_Dissector::dissect (Wireshark_Bundle &params)
    {
      Wireshark_Bundle_i sp;
      sp.tvb = params.tvb;
      sp.info = params.info;
      sp.tree = params.tree;
      sp.offset = params.offset;
      sp.use_index = false;
      sp.index = 0;
      return params.offset + this->dissect_i (sp, "");
    }

    //----------------------------------------------------------------------

    Sample_String::Sample_String ()
    {
    }

    size_t
    Sample_String::dissect_i (Wireshark_Bundle_i &params, const char *label)
    {
      guint8 *data = params.get_remainder ();
      guint32 len = *(reinterpret_cast< guint32 * >(data));
      data += 4;
      guint8 *last = data + len;

      std::stringstream outstream;
      outstream << label;
      if (params.use_index)
        outstream << "[" << params.index << "]";
      outstream << ": ";
      while (data != last)
        outstream << *(data++);
      outstream << std::ends;
      std::string buffer = outstream.str();

      proto_tree_add_text (params.tree, params.tvb, params.offset, len + 4,
                           "%s", buffer.c_str());
      return len + 4;
    }

    size_t
    Sample_String::compute_length (guint8 *data)
    {
      return 4 + *(reinterpret_cast< guint32 * >(data));
    }


    //----------------------------------------------------------------------

    Sample_WString::Sample_WString ()
    {
    }

    size_t
    Sample_WString::dissect_i (Wireshark_Bundle_i &params, const char *label)
    {
      guint8* data = params.get_remainder();
      guint32 len =
        *(reinterpret_cast< guint32 * >(data));
      guint32 width = len * sizeof (ACE_CDR::WChar);

      std::stringstream outstream;
      outstream << label;
      if (params.use_index)
        outstream << "[" << params.index << "]";
      outstream << ":" << std::ends;
      std::string buffer = outstream.str();

      ACE_CDR::WChar * clone = new ACE_CDR::WChar[len + 1];
      ACE_OS::memcpy (clone, data+4, width);
      clone[len] = 0;
      proto_tree_add_text (params.tree, params.tvb, params.offset,
                           width + 4, "%s %ls", buffer.c_str(), clone);
      delete [] clone;

      return width + 4;
    }

    size_t
    Sample_WString::compute_length (guint8 *data)
    {
      return 4 + *(reinterpret_cast< guint32 * >(data)) * sizeof (ACE_CDR::WChar);
    }

    //----------------------------------------------------------------------

    Sample_Sequence::Sample_Sequence (Sample_Field *f)
      : element_ (0)
    {
      this->init ("","sequence");
      element_ = new Sample_Dissector;
      if (f)
        {
          element_->add_field (f);
        }
    }

    Sample_Sequence::Sample_Sequence (Sample_Dissector *sub)
      : element_ (sub)
    {
      this->init ("","sequence");
    }

    Sample_Sequence::Sample_Sequence (Sample_Field::FixedTypeID type_id)
      : element_ (0)
    {
      this->init ("","sequence");
      element_ = new Sample_Dissector;
      element_->add_field (new Sample_Field (type_id, "element"));
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
    Sample_Sequence::dissect_i (Wireshark_Bundle_i &params, const char *label)
    {
      guint8 *data = params.get_remainder();
      guint32 len = compute_length (data);
      guint32 count = *(reinterpret_cast< guint32 * >(data));

      std::stringstream outstream;
      outstream << label;
      if (params.use_index)
        outstream << "[" << params.index << "]";
      outstream << " (length = " << count << ")" << std::ends;
      std::string buffer = outstream.str();
      proto_item *item =
        proto_tree_add_text (params.tree, params.tvb, params.offset,
                             len,"%s", buffer.c_str());
      proto_tree *subtree = proto_item_add_subtree (item, ett_payload_);

      size_t data_pos = 4;
      Wireshark_Bundle_i sp = params;
      sp.tree = subtree;
      sp.use_index = true;
      for (guint32 ndx = 0; ndx < count; ndx++)
        {
          sp.index = ndx;
          sp.offset = params.offset + data_pos;
          data_pos += element_->dissect_i (sp, label);
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

    Sample_Array::Sample_Array (size_t count, Sample_Field *field)
      :Sample_Sequence (field),
       count_(count)
    {
    }
    Sample_Array::Sample_Array (size_t count, Sample_Dissector *sub)
      : Sample_Sequence (sub),
        count_ (count)
    {
    }

    Sample_Array::Sample_Array (size_t count, Sample_Field::FixedTypeID type_id)
      : Sample_Sequence (type_id),
        count_ (count)
    {
    }


    size_t
    Sample_Array::dissect_i (Wireshark_Bundle_i &params, const char *label)
    {
      guint8 * data = params.get_remainder();
      size_t len = compute_length (data);

      std::stringstream outstream;
      outstream << label;
      if (params.use_index)
        outstream << "[" << params.index << "]";
      outstream << " (length = " << count_ << ")" << std::ends;
      std::string buffer = outstream.str();
      proto_item *item =
        proto_tree_add_text (params.tree, params.tvb, params.offset,
                             len,"%s", buffer.c_str());
      proto_tree *subtree = proto_item_add_subtree (item, ett_payload_);

      size_t data_pos = 0;
      Wireshark_Bundle_i sp = params;
      sp.tree = subtree;
      sp.use_index = true;
      for (guint32 ndx = 0; ndx < count_; ndx++)
        {
          sp.index = ndx;
          sp.offset = params.offset + data_pos;
          data_pos += element_->dissect_i (sp, label);
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

    Sample_Enum::Sample_Enum ()
      : value_ (0)
    {
    }

    Sample_Enum::~Sample_Enum ()
    {
      delete value_;
    }

    Sample_Field *
    Sample_Enum::add_value (const char *name)
    {
      Sample_Field *sf = new Sample_Field (Sample_Field::Enumeration, name);
      return (value_ == 0) ? (value_ = sf) : value_->chain (sf);
    }

    size_t
    Sample_Enum::dissect_i (Wireshark_Bundle_i &params, const char *label)
    {
      guint8 * data = params.get_remainder();
      size_t len = 4;
      guint32 value = *(reinterpret_cast< guint32 * >(data));
      Sample_Field *sf = value_->get_link (value);

      std::stringstream outstream;
      outstream << label;
      if (params.use_index)
        outstream << "[" << params.index << "]";
      outstream << ": " << sf->label_ << std::ends;
      std::string buffer = outstream.str();

      proto_tree_add_text (params.tree, params.tvb, params.offset,
                           len, "%s", buffer.c_str());
      return len;
    }

     size_t
    Sample_Enum::compute_length (guint8 *)
    {
      return 4;
    }

    //----------------------------------------------------------------------

    Sample_Union::Sample_Union ()
      :_d (0),
       case_ (0)
    {
    }

    Sample_Union::~Sample_Union ()
    {
      delete _d;
      delete case_;
    }

    void
    Sample_Union::discriminator (Sample_Dissector *d)
    {
      this->_d = d;
    }

    Sample_Field *
    Sample_Union::add_case (Sample_Field *key, Sample_Field *value)
    {
      if (this->case_ == 0)
        this->case_ = key;
      else
        this->case_->chain (key);
      return key;

    }

    Sample_Field *
    Sample_Union::add_case_range (Sample_Field *key_low,
                                  Sample_Field *key_high,
                                  Sample_Field *value)
    {
      return key_low;
    }

    void
    Sample_Union::add_default (Sample_Field *value)
    {
    }

    size_t
    Sample_Union::compute_length (guint8 *data)
    {
      return 0;
    }

    size_t
    Sample_Union::dissect_i (Wireshark_Bundle_i &p, const char *l)
    {
      return 0;
    }

  }
}
