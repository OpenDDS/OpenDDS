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

    Sample_Field::Sample_Field (IDLTypeID id, const char *label)
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
    Sample_Field::chain (IDLTypeID ti, const char *l)
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
    Sample_Field::compute_field_length (guint8 *data)
    {
      size_t len = 0;
      switch (this->type_id_)
        {
        case Char:
          {
            len = sizeof (ACE_CDR::Char);
            break;
          }
        case Boolean:
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
        case String:
          {
            len = 4 + *(reinterpret_cast< guint32 * >(data));
            break;
          }
        case WString:
          {
            len = 4 +
              *(reinterpret_cast< guint32 * >(data)) * sizeof (ACE_CDR::WChar);
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
      return len;
    }

    size_t
    Sample_Field::compute_length (guint8 *data)
    {
      size_t len = this->compute_field_length (data);
      if (next_ != 0)
        len += next_->compute_length (data+len);
      return len;
    }

    void
    Sample_Field::to_stream (std::stringstream &s, guint8 *data)
    {
      switch (this->type_id_)
        {
        case Char:
          {
            s << *reinterpret_cast<ACE_CDR::Char *>(data);
            break;
          }
        case Boolean:
          {
            if (*reinterpret_cast<ACE_CDR::Boolean *>(data))
              s << "true";
            else
              s << "false";
            break;
          }
        case Octet:
          {
            s << *reinterpret_cast<ACE_CDR::Octet *>(data);
            break;
          }
        case WChar:
          {
            s << *reinterpret_cast<ACE_CDR::WChar *>(data);
            break;
          }
        case Short:
          {
            s << *reinterpret_cast<ACE_CDR::Short *>(data);
            break;
          }
        case UShort:
          {
            s << *reinterpret_cast<ACE_CDR::UShort *>(data);
            break;
          }
        case Long:
          {
            s << *reinterpret_cast<ACE_CDR::Long *>(data);
            break;
          }
        case ULong:
          {
            s << *reinterpret_cast<ACE_CDR::ULong *>(data);
            break;
          }
        case LongLong:
          {
            s << *reinterpret_cast<ACE_CDR::LongLong *>(data);
            break;
          }
        case ULongLong:
          {
            s << *reinterpret_cast<ACE_CDR::ULongLong *>(data);
            break;
          }
        case Float:
          {
            s << *reinterpret_cast<ACE_CDR::Float *>(data);
            break;
          }
        case Double:
          {
            s << *reinterpret_cast<ACE_CDR::Double *>(data);
            break;
          }
        case LongDouble:
          {
            s << *reinterpret_cast<ACE_CDR::LongDouble *>(data);
            break;
          }
        case String:
          {
            guint32 len = *(reinterpret_cast< guint32 * >(data));
            data += 4;
            guint8 *last = data + len;
            while (data != last)
              s << *(data++);
            break;
          }
        case WString:
          {
            break; // handled separately
          }
        case Enumeration:
          {
            break; // only the label is used, not directly presented
          }
        case Undefined:
          {
            s << "type undefined";
          }
        }
    }

    size_t
    Sample_Field::dissect_i (Wireshark_Bundle_Field &params,
                             std::string &alt_label)
    {
      size_t len = 0;
      if (this->nested_ == 0)
        {
          std::stringstream outstream;
          if (this->label_.empty())
            outstream << alt_label;
          else
            outstream << this->label_;
          if (params.use_index)
            outstream << "[" << params.index << "]";
          outstream << ": ";
          len = compute_field_length (params.data);
          if (this->type_id_ != WString)
            {
              this->to_stream (outstream, params.data);
              outstream << std::ends;
              std::string buffer = outstream.str();
              proto_tree_add_text (params.tree, params.tvb, params.offset,
                                   len, "%s", buffer.c_str());
            }
          else
            {
              guint32 len =
                *(reinterpret_cast< guint32 * >(params.data));
              guint32 width = len * sizeof (ACE_CDR::WChar);
              outstream << std::ends;
              std::string buffer = outstream.str();

              ACE_CDR::WChar * clone = new ACE_CDR::WChar[len + 1];
              ACE_OS::memcpy (clone, params.data+4, width);
              clone[len] = 0;
              proto_tree_add_text (params.tree, params.tvb, params.offset,
                                   width + 4, "%s %ls", buffer.c_str(), clone);
              delete [] clone;
            }
        }
      else
        {
          len = this->nested_->dissect_i (params, this->label_);
        }

      params.offset += len;
      params.data += len;
      if (next_ != 0)
        len += next_->dissect_i (params, this->label_);
      return len;
    }

    //------------------------------------------------------------------------

    Sample_Dissector::Sample_Dissector (const char *type_id,
                                        const char *subtree)
      :ett_payload_ (-1),
       subtree_label_(),
       typeId_ (),
       field_ (0)
    {
      if (type_id != 0 || subtree != 0)
        this->init (type_id, subtree);
    }

    Sample_Dissector::~Sample_Dissector ()
    {
      delete field_;
    }

    std::string &
    Sample_Dissector::typeId()
    {
      return this->typeId_;
    }

    void
    Sample_Dissector::init (const char *type_id,
                            const char *subtree)
    {
      size_t len = (type_id == 0) ? 0 : ACE_OS::strlen(type_id);
      if (len > 0)
        {
          this->typeId_ = type_id;
          Sample_Manager::instance().add (*this);
        }

      len = (subtree == 0) ? 0 : ACE_OS::strlen(subtree);
      if (len > 0)
        {
          this->subtree_label_ = subtree;

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
    Sample_Dissector::add_field (Sample_Field::IDLTypeID type_id,
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
                                 std::string &label)

    {
      size_t data_pos = 0;

      guint8* data = params.get_remainder();
      guint32 len = field_->compute_length (data);
      std::stringstream outstream;
      bool top_level = true;
      if (!label.empty())
        {
          top_level = false;
          outstream << label;
          if (params.use_index)
            outstream << "[" << params.index << "]";
        }
      proto_tree *subtree = params.tree;
      if (!this->subtree_label_.empty())
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
          data_pos += field_->dissect_i (fp, label);
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
      std::string label;
      return params.offset + this->dissect_i (sp, label);
    }

    Sample_Field::IDLTypeID
    Sample_Dissector::get_field_type ()
    {
      if (field_ == 0 || field_->next_ != 0)
        return Sample_Field::Undefined;
      return field_->type_id_;
    }

    std::string
    Sample_Dissector::stringify (guint8 *data)
    {
      std::stringstream outstream;
      if (field_ != 0)
        {
          field_->to_stream (outstream, data);
        }
      return outstream.str();
    }

    //----------------------------------------------------------------------

    Sample_Sequence::Sample_Sequence (const char *type_id, Sample_Field *f)
      : element_ (0)
    {
      this->init (type_id,"sequence");
      element_ = new Sample_Dissector;
      if (f)
        {
          element_->add_field (f);
        }
    }

    Sample_Sequence::Sample_Sequence (const char *type_id,
                                      Sample_Dissector *sub)
      : element_ (sub)
    {
      this->init (type_id, "sequence");
    }

    Sample_Sequence::Sample_Sequence (const char *type_id,
                                      Sample_Field::IDLTypeID field_id)
      : element_ (0)
    {
      this->init (type_id,"sequence");
      element_ = new Sample_Dissector;
      element_->add_field (new Sample_Field (field_id, ""));
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
    Sample_Sequence::dissect_i (Wireshark_Bundle_i &params, std::string &label)
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

    Sample_Array::Sample_Array (const char *type_id,
                                size_t count,
                                Sample_Field *field)
      :Sample_Sequence (type_id, field),
       count_(count)
    {
    }
    Sample_Array::Sample_Array (const char *type_id,
                                size_t count,
                                Sample_Dissector *sub)
      : Sample_Sequence (type_id, sub),
        count_ (count)
    {
    }

    Sample_Array::Sample_Array (const char *type_id,
                                size_t count,
                                Sample_Field::IDLTypeID field_id)
      : Sample_Sequence (type_id, field_id),
        count_ (count)
    {
    }


    size_t
    Sample_Array::dissect_i (Wireshark_Bundle_i &params, std::string &label)
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

    Sample_Enum::Sample_Enum (const char *type_id)
      : value_ (0)
    {
      this->init (type_id, "");
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
    Sample_Enum::dissect_i (Wireshark_Bundle_i &params, std::string &label)
    {
      guint8 * data = params.get_remainder();
      size_t len = 4;
      guint32 value = *(reinterpret_cast< guint32 * >(data));
      Sample_Field *sf = value_->get_link (value);

      std::stringstream outstream;
      outstream << label;
      if (params.use_index)
        outstream << "[" << params.index << "]";
      if (sf == 0)
        outstream << ": <value out of bounds: " << value << "> " << std::ends;
      else
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

    bool
    Sample_Enum::index_of (std::string &value, size_t &result)
    {
      result = 0;
      Sample_Field *iter = value_;
      while (iter != 0)
        {
          if (iter->label_.compare (value) == 0)
            return true;
          result++;
          iter = iter->next_;
        }
      return false;
    }

    //----------------------------------------------------------------------

    Switch_Case::Switch_Case (Sample_Field::IDLTypeID type_id,
                              const char *label,
                              Sample_Field *field)
      : Sample_Field (type_id, label),
        span_(0),
        field_ (field)
    {
    }

    Switch_Case::~Switch_Case ()
    {
      delete span_;
      delete field_;
    }

    Switch_Case *
    Switch_Case::add_range (const char *label)
    {
      Switch_Case *c = new Switch_Case (this->type_id_,
                                        label);
      c->field_ = this->field_;
      this->field_ = 0;
      if (span_ == 0)
        span_ = c;
      else
        {
          Switch_Case *last = span_;
          while (last->span_ != 0)
            last = last->span_;
          c->field_ = last->field_;
          last->field_ = 0;
          last->span_ = c;
        }
      return c;
    }

    Switch_Case *
    Switch_Case::chain (Switch_Case *next)
    {
      Sample_Field::chain (next);
      return next;
    }

    Switch_Case *
    Switch_Case::chain (const char *label, Sample_Field *field)
    {
      return chain (new Switch_Case (this->type_id_,label, field));
    }

    Sample_Field *
    Switch_Case::do_switch (std::string &_d, guint8 *data)
    {
      // extract the discriminator value pointed to by data,
      // convert element label and compare. If lower_bound_ is nil,
      // exactly match label, otherwise match
      // accept any value between the converted "lower_bound_" and the "label_"
      Sample_Field *value = 0;
      if (_d.compare (this->label_) == 0)
        value = this->get_field();

      if (this->span_ != 0)
        value = this->span_->do_switch (_d,data);

      Switch_Case *sc = dynamic_cast<Switch_Case *>(this->next_);
      if ( value == 0 && next_ != 0)
        return sc->do_switch (_d,data);
      return value;
    }

    Sample_Field *
    Switch_Case::get_field ()
    {
      Switch_Case *last = this;
      while (last->span_ != 0)
        last = last->span_;
      return last->field_;
    }

    void
    Switch_Case::set_field (Sample_Field *field)
    {
      Switch_Case *last = this;
      while (last->span_ != 0)
        {
          delete last->field_;
          last->field_ = 0;

          last = last->span_;
        }
      last->field_ = field;
    }

    Sample_Union::Sample_Union (const char *type_id)
      :discriminator_ (0),
       cases_ (0),
       default_ (0)
    {
      this->init (type_id,"union");
    }

    Sample_Union::~Sample_Union ()
    {
      delete discriminator_;
      delete cases_;
      delete default_;
    }

    void
    Sample_Union::discriminator (Sample_Dissector *d)
    {
      this->discriminator_ = d;
    }

    Switch_Case *
    Sample_Union::add_case (const char *label, Sample_Field *field)
    {
      Sample_Field::IDLTypeID type_id =
        discriminator_->get_field_type();
      Switch_Case *c = new Switch_Case (type_id, label, field);
      if (this->cases_ == 0)
        this->cases_ = c;
      else
        this->cases_->chain (c);
      return c;
    }

    void
    Sample_Union::add_default (Sample_Field *value)
    {
      this->default_ = value;
    }

    size_t
    Sample_Union::compute_length (guint8 *data)
    {
      // length = length of discriminator + length of max field.
      // Or is it length of specific field?
      size_t len = this->discriminator_->compute_length(data);
      std::string _d = this->discriminator_->stringify(data);
      Sample_Field *value =
        this->cases_->do_switch (_d, data);
      if (value == 0)
        value = this->default_;
      return value->compute_length (data + len);
    }

    size_t
    Sample_Union::dissect_i (Wireshark_Bundle_i &params, std::string &label)
    {
      guint8 * data = params.get_remainder();
      size_t len = this->discriminator_->compute_length(data);
      std::string _d = this->discriminator_->stringify(data);

      size_t data_pos = len;
      Sample_Field *value =
        this->cases_->do_switch (_d, data);
      if (value == 0)
        value = this->default_;
      len += value->compute_length (data + len);

      std::stringstream outstream;
      outstream << label;
      if (params.use_index)
        outstream << "[" << params.index << "]";
      outstream << " ( on " << _d << ")" << std::ends;

      std::string buffer = outstream.str();
      proto_item *item =
        proto_tree_add_text (params.tree, params.tvb, params.offset,
                             len,"%s", buffer.c_str());
      proto_tree *subtree = proto_item_add_subtree (item, ett_payload_);


      Wireshark_Bundle_Field fp;
      fp.tvb = params.tvb;
      fp.info = params.info;
      fp.tree = subtree;
      fp.offset = params.offset + data_pos;
      fp.use_index = false;
      fp.index = 0;
      fp.data = data;

      data_pos += value->dissect_i (fp, label);

      return data_pos;
    }

  }
}
