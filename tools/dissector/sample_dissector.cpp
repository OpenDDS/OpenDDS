/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "tools/dissector/sample_dissector.h"
#include "tools/dissector/sample_manager.h"
#include "tools/dissector/ws_common.h"

#include "dds/DCPS/Serializer.h"

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

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS
{
  namespace DCPS
  {
    Sample_Base::~Sample_Base() {
      /*
       * Clean Up Field Contexts
       */
      for (
        Field_Contexts::iterator i = field_contexts_.begin();
        i != field_contexts_.end();
        ++i
      ) {
        delete i->second;
        i->second = NULL;
      }
    }

    Field_Context * Sample_Base::get_context() {
      if (field_contexts_.count(ns_)) {
        return field_contexts_[ns_];
      }
      Field_Context * fc = new Field_Context;
      field_contexts_[ns_] = fc;
      fc->label_ = ns_stack_.back();
      return fc;
    }

    std::list<std::string> Sample_Base::ns_stack_;
    std::string Sample_Base::ns_;

    int Sample_Base::get_hf() {
      if (!field_contexts_.count(ns_)) {
        return -1;
      }
      return field_contexts_[ns_]->hf_;
    }

    void Sample_Base::rebuild_ns() {
      std::stringstream ss;
      for (std::list<std::string>::iterator i = ns_stack_.begin();
        i != ns_stack_.end(); i++
      ) {
        ss << "." << *i;
      }
      ns_ = ss.str();
    }

    std::string Sample_Base::get_ns() {
      return payload_namespace + ns_;
    }

    void Sample_Base::push_ns(const std::string & name) {
      ns_stack_.push_back(name);
      rebuild_ns();
    }

    void Sample_Base::clear_ns() {
      ns_stack_.clear();
      ns_ = "";
    }

    std::string Sample_Base::pop_ns() {
      std::string name = ns_stack_.back();
      ns_stack_.pop_back();
      rebuild_ns();
      return name;
    }

    void Sample_Base::add_protocol_field(enum ftenum ft, field_display_e fd) {
      Field_Context * fc = get_context();
      Sample_Manager::instance().add_protocol_field(
        &fc->hf_, get_ns(), fc->label_, ft, fd
      );
    }

    guint8 *
    Wireshark_Bundle::get_remainder ()
    {
      gint remainder = ws_tvb_length(tvb) - offset;
      return reinterpret_cast<guint8 *>(ws_ep_tvb_memdup(tvb, offset, remainder));
    }

    Sample_Field::Sample_Field (IDLTypeID id, const std::string &label)
      : label_ (label),
        type_id_ (id),
        nested_ (0),
        next_(0)
    {
    }

    Sample_Field::Sample_Field (Sample_Dissector *n, const std::string &label)
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
    Sample_Field::chain (IDLTypeID ti, const std::string &l)
    {
      return chain (new Sample_Field (ti, l));
    }

    Sample_Field *
    Sample_Field::chain (Sample_Dissector *n, const std::string &l)
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
            len = 1 + Serializer::WCHAR_SIZE;
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
              *(reinterpret_cast< guint32 * >(data)) * Serializer::WCHAR_SIZE;
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
            guint8 *last = data + len - 1 /*len included the trailing null*/;
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
                             const std::string &alt_label,
                             bool recur)
    {
      size_t len = 0;
      if (this->nested_ == 0) {
        len = compute_field_length (params.data);

        int hf = get_hf();
        if (hf == -1) {
          ACE_DEBUG((LM_DEBUG, ACE_TEXT("%s is not a registered wireshark field.\n"),
                     get_ns().c_str()));
        } else {

          // These Variables are Used by String and WString
          std::stringstream s;
          guint32 l;
          guint8 * last;
          guint32 width;
          ACE_CDR::WChar * clone;
          guint8 * str_data;
          
          // Set Field
          switch (this->type_id_) {

          case Sample_Field::Boolean:
            proto_tree_add_item(
              params.tree, hf,
              params.tvb, params.offset, (gint)len,
              *reinterpret_cast<ACE_CDR::Boolean *>(params.data));
            break;

          case Sample_Field::Char:
            proto_tree_add_item(
              params.tree, hf,
              params.tvb, params.offset, (gint)len,
              *reinterpret_cast<ACE_CDR::Char *>(params.data));
            break;

          case Sample_Field::Octet:
            proto_tree_add_uint(
              params.tree, hf,
              params.tvb, params.offset, (gint)len,
              *reinterpret_cast<ACE_CDR::Octet *>(params.data));
            break;

          case Sample_Field::WChar:
            // TODO
            /* proto_tree_add_string( */
            /*   params.tree, hf, */
            /*   params.tvb, params.offset, (gint)len, */
            /*   reinterpret_cast<ACE_CDR::WChar *>(params.data)); */
            break;

          case Sample_Field::Short:
            proto_tree_add_int(
              params.tree, hf,
              params.tvb, params.offset, (gint)len,
              *reinterpret_cast<ACE_CDR::Short *>(params.data));
            break;

          case Sample_Field::Long:
            proto_tree_add_int(
              params.tree, hf,
              params.tvb, params.offset, (gint)len,
              *reinterpret_cast<ACE_CDR::Long *>(params.data));
            break;

          case Sample_Field::LongLong:
            proto_tree_add_int64(
              params.tree, hf,
              params.tvb, params.offset, (gint)len,
              *reinterpret_cast<ACE_CDR::LongLong *>(params.data));
            break;

          case Sample_Field::UShort:
            proto_tree_add_uint(
              params.tree, hf,
              params.tvb, params.offset, (gint)len,
              *reinterpret_cast<ACE_CDR::UShort *>(params.data));
            break;

          case Sample_Field::ULong:
            proto_tree_add_uint(
              params.tree, hf,
              params.tvb, params.offset, (gint)len,
              *reinterpret_cast<ACE_CDR::ULong *>(params.data));
            break;

          case Sample_Field::ULongLong:
            proto_tree_add_uint64(
              params.tree, hf,
              params.tvb, params.offset, (gint)len,
              *reinterpret_cast<ACE_CDR::ULongLong *>(params.data));
            break;

          case Sample_Field::Float:
            proto_tree_add_float(
              params.tree, hf,
              params.tvb, params.offset, (gint)len,
              *reinterpret_cast<ACE_CDR::Float *>(params.data));
            break;

          case Sample_Field::Double:
            proto_tree_add_double(
              params.tree, hf,
              params.tvb, params.offset, (gint)len,
              *reinterpret_cast<ACE_CDR::Double *>(params.data));
            break;

          case Sample_Field::LongDouble:
            // Casting to double
            proto_tree_add_double(
              params.tree, hf,
              params.tvb, params.offset, (gint)len,
              (double) (*reinterpret_cast<ACE_CDR::LongDouble *>(params.data)));
            break;

          case Sample_Field::String:
            str_data = params.data;
            l = *(reinterpret_cast< guint32 * >(params.data));
            str_data += 4;
            last = str_data + l - 1; // len included the trailing null
            while (str_data != last) {
              s << *(str_data++);
            }
            proto_tree_add_string(
              params.tree, hf,
              params.tvb, params.offset, (gint)len,
              s.str().c_str());
            break;

          case Sample_Field::WString:
            // TODO: Investigate this more and make sure it works
            // as it should.
            /* len = *(reinterpret_cast< guint32 * >(params.data)); */
            /* width = len * Serializer::WCHAR_SIZE; */
            /* clone = new ACE_CDR::WChar[len + 1]; */
            /* ACE_OS::memcpy(clone, params.data+4, width); */
            /* clone[len] = 0; */
            /* proto_tree_add_string_format( */
            /*   params.tree, hf, */
            /*   params.tvb, params.offset, width + 4, */
            /*   (char *) params.data, */
            /*   "%s %ls", params.data, clone); */
            /* delete [] clone; */
            break;

          default:
            proto_tree_add_bytes(
              params.tree, hf,
              params.tvb, params.offset, (gint)len,
              params.data);
            break;
          }
        }
      } else {
        push_ns(this->label_);
        len = this->nested_->dissect_i(params, this->label_);
        pop_ns();
      }

      params.offset += (gint)len;
      params.data += len;
      if (next_ != 0 && recur) {
        len += next_->dissect_i(params, this->label_);
      }
      return len;
    }

    void Sample_Field::init_ws_fields() {
      if (!label_.empty() && nested_ == NULL) {
        push_ns(label_);
        pop_ns();
      } else if (label_.empty()) {
        
        switch (type_id_) {

        case Sample_Field::Boolean:
          add_protocol_field(FT_BOOLEAN);
          break;

        case Sample_Field::Char:
          add_protocol_field(FT_CHAR);
          break;

        case Sample_Field::Octet:
          add_protocol_field(FT_UINT8, BASE_HEX);
          break;

        case Sample_Field::WChar:
          add_protocol_field(FT_STRING);
          break;

        case Sample_Field::Short:
          add_protocol_field(FT_INT16, BASE_DEC);
          break;

        case Sample_Field::Long:
          add_protocol_field(FT_INT32, BASE_DEC);
          break;

        case Sample_Field::LongLong:
          add_protocol_field(FT_INT64, BASE_DEC);
          break;

        case Sample_Field::UShort:
          add_protocol_field(FT_UINT16, BASE_DEC);
          break;

        case Sample_Field::ULong:
          add_protocol_field(FT_UINT32, BASE_DEC);
          break;

        case Sample_Field::ULongLong:
          add_protocol_field(FT_UINT64, BASE_DEC);
          break;

        case Sample_Field::Float:
          add_protocol_field(FT_FLOAT);
          break;

        case Sample_Field::Double:
          add_protocol_field(FT_DOUBLE);
          break;

        case Sample_Field::LongDouble:
          // Long Doubles will be cast to doubles, resulting in possible 
          // data loss if long doubles are larger than doubles.
          add_protocol_field(FT_DOUBLE);
          break;

        case Sample_Field::String:
          add_protocol_field(FT_STRING);
          break;

        case Sample_Field::WString:
          add_protocol_field(FT_STRING);
          break;

        default:
          add_protocol_field(FT_BYTES);
          break;
        }
      } else {
        push_ns(label_);
        if (nested_ != NULL) {
          nested_->init_ws_fields();
        }
        pop_ns();
      }
      if (next_ != NULL) {
        next_->init_ws_fields();
      }
    }

    //------------------------------------------------------------------------

    Sample_Dissector::Sample_Dissector (const std::string &subtree)
      :ett_ (-1),
       subtree_label_(),
       field_ (0)
    {
      if (!subtree.empty())
        this->init (subtree);
    }

    Sample_Dissector::~Sample_Dissector ()
    {
      delete field_;
    }

    void
    Sample_Dissector::init (const std::string &subtree)
    {

      if (subtree.length() > 0)
        {
          this->subtree_label_ = subtree;

          gint *ett[] = {
            &ett_
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
    Sample_Dissector::add_field(Sample_Field::IDLTypeID type_id,
                                const std::string &label)
    {
      return add_field(new Sample_Field(type_id, label));
    }

    Sample_Field *
    Sample_Dissector::add_field(Sample_Dissector *n,
                                const std::string &label)
    {
      return add_field(new Sample_Field(n, label));
    }

    size_t
    Sample_Dissector::dissect_i (Wireshark_Bundle_i &params,
                                 const std::string &label)

    {
      size_t data_pos = 0;

      guint8* data = params.get_remainder();
      guint32 len = (guint32)field_->compute_length (data);
      std::stringstream outstream;
      bool top_level = true;
      if (!label.empty())
        {
          top_level = false;
          outstream << label;
          if (params.use_index)
            outstream << "[" << params.index << "] ";
        }
      proto_tree *subtree = params.tree;
      bool use_index = params.use_index;
      if (!this->subtree_label_.empty())
        {
          if (top_level)
            outstream << "Sample Payload: ";
          else
            outstream << ": ";
          outstream << subtree_label_;
          std::string buffer = outstream.str();
          proto_item *item =
            ws_proto_tree_add_text (params.tree, params.tvb, params.offset, len,
                                 "%s", buffer.c_str());
          subtree = proto_item_add_subtree (item, ett_);
          use_index = false;
        }

      if (field_ != 0)
        {
          Wireshark_Bundle_Field fp;
          fp.tvb = params.tvb;
          fp.info = params.info;
          fp.tree = subtree;
          fp.offset = params.offset;
          fp.use_index = use_index;
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
      return params.offset + (gint)this->dissect_i (sp, label);
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

    void Sample_Dissector::init_ws_fields() {
      if (field_ != NULL) {
        field_->init_ws_fields();
      }
    }

    //----------------------------------------------------------------------
    Sample_Sequence::Sample_Sequence (Sample_Dissector *sub)
      : element_ (sub)
    {
      this->init ("sequence");
    }

    Sample_Dissector *
    Sample_Sequence::element ()
    {
      return element_;
    }

    size_t
    Sample_Sequence::dissect_i (Wireshark_Bundle_i &params,
                                const std::string &label)
    {
      guint8 *data = params.get_remainder();
      guint32 len = (guint32)compute_length (data);
      guint32 count = *(reinterpret_cast< guint32 * >(data));

      std::stringstream outstream;
      if (params.use_index)
        outstream << "[" << params.index << "] ";
      outstream << "(length = " << count << ") ";
      proto_item *item = proto_tree_add_uint_format_value(
        params.tree, get_hf(), params.tvb, params.offset, (gint)len,
        count, outstream.str().c_str()
      );
      proto_tree *subtree = proto_item_add_subtree (item, ett_);

      size_t data_pos = 4;
      Wireshark_Bundle_i sp = params;
      sp.tree = subtree;
      push_ns(element_namespace);
      for (guint32 ndx = 0; ndx < count; ndx++)
        {
          sp.index = ndx;
          sp.use_index = true;
          sp.offset = params.offset + (gint)data_pos;
          data_pos += element_->dissect_i (sp, label);
        }
      pop_ns();
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

    void Sample_Sequence::init_ws_fields() {
      add_protocol_field(FT_UINT32, BASE_DEC);
      push_ns(element_namespace);
      element_->init_ws_fields();
      pop_ns();
    }

    //----------------------------------------------------------------------

    Sample_Array::Sample_Array (size_t count,
                                Sample_Dissector *sub)
      : Sample_Sequence (sub),
        count_ (count)
    {
    }

    size_t
    Sample_Array::dissect_i (Wireshark_Bundle_i &params, const std::string &label)
    {
      guint8 * data = params.get_remainder();
      size_t len = compute_length(data);

      std::stringstream outstream;
      if (params.use_index)
        outstream << "[" << params.index << "] ";
      outstream << "(length = " << count_ << ") ";
      proto_item *item = proto_tree_add_uint_format_value(
        params.tree, get_hf(), params.tvb, params.offset, (gint)len,
        count_, outstream.str().c_str()
      );
      proto_tree *subtree = proto_item_add_subtree (item, ett_);

      size_t data_pos = 0;
      Wireshark_Bundle_i sp = params;
      sp.tree = subtree;
      push_ns(element_namespace);
      for (guint32 ndx = 0; ndx < count_; ndx++) {
        sp.index = ndx;
        sp.offset = params.offset + (gint)data_pos;
        sp.use_index = true;
        data_pos += element_->dissect_i(sp, label);
      }
      pop_ns();
      return data_pos;
    }

    size_t
    Sample_Array::compute_length (guint8 *data)
    {
      size_t len = 0;
      for (guint32 i = 0; i < count_; i++) {
        size_t elen = element_->compute_length(data);
        data += elen;
        len += elen;
      }

      return len;
    }

    void Sample_Array::init_ws_fields() {
      add_protocol_field(FT_UINT32, BASE_DEC);
      push_ns(element_namespace);
      element_->init_ws_fields();
      pop_ns();
    }

    //----------------------------------------------------------------------

    Sample_Enum::Sample_Enum ()
      : value_ (0)
    {
      this->init ("");
    }

    Sample_Enum::~Sample_Enum ()
    {
      delete value_;
    }

    Sample_Field *
    Sample_Enum::add_value(const std::string &name)
    {
      Sample_Field *sf = new Sample_Field(Sample_Field::Enumeration, name);
      return (value_ == 0) ? (value_ = sf) : value_->chain(sf);
    }

    size_t
    Sample_Enum::dissect_i (Wireshark_Bundle_i &params,
                            const std::string &label)
    {
      guint8 * data = params.get_remainder();
      size_t len = 4;
      guint32 value = *(reinterpret_cast< guint32 * >(data));
      Sample_Field *sf = value_->get_link(value);

      std::stringstream outstream;
      if (params.use_index)
        outstream << "[" << params.index << "] ";
      if (sf == 0)
        outstream << "<value out of bounds: " << value << "> ";
      else
        outstream << sf->label_;

      proto_tree_add_string(params.tree, get_hf(), params.tvb, params.offset,
                           (gint)len, outstream.str().c_str());
      return len;
    }

    size_t
    Sample_Enum::compute_length (guint8 *)
    {
      return 4;
    }

    bool
    Sample_Enum::index_of (const std::string &value, size_t &result)
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

    std::string
    Sample_Enum::stringify (guint8 *data)
    {
      guint32 value = *(reinterpret_cast< guint32 * >(data));
      Sample_Field *sf = value_->get_link (value);

      if (sf) {
        return sf->label_;
      }

      std::stringstream outstream;
      outstream << value;
      return outstream.str();
    }

    void Sample_Enum::init_ws_fields() {
      add_protocol_field(FT_STRING);
      if (value_ != NULL)
        value_->init_ws_fields();
    }

    //----------------------------------------------------------------------

    Sample_Union::Sample_Union ()
      :discriminator_ (0),
       default_ (0)
    {
      this->init ("union");
    }

    Sample_Union::~Sample_Union ()
    {
      delete default_;
    }

    void
    Sample_Union::discriminator (Sample_Dissector *d)
    {
      this->discriminator_ = d;
    }

    void
    Sample_Union::add_label (const std::string& label, Sample_Field* field)
    {
      map_[label] = field;
    }

    void
    Sample_Union::add_default (Sample_Field *value)
    {
      this->default_ = value;
    }

    size_t
    Sample_Union::compute_length (guint8 *data)
    {
      size_t len = this->discriminator_->compute_length(data);
      std::string _d = this->discriminator_->stringify(data);
      Sample_Field* value = this->default_;
      MapType::const_iterator pos = map_.find(_d);
      if (pos != map_.end()) {
        value = pos->second;
      }
      return len + value->compute_field_length(data + len);
    }

    size_t
    Sample_Union::dissect_i (Wireshark_Bundle_i &params,
                             const std::string &label)
    {
      guint8 * data = params.get_remainder();
      size_t len = this->discriminator_->compute_length(data);
      std::string _d = this->discriminator_->stringify(data);

      size_t data_pos = len;
      Sample_Field* value = this->default_;
      MapType::const_iterator pos = map_.find(_d);
      if (pos != map_.end()) {
        value = pos->second;
      }
      len += value->compute_field_length (data + len);

      std::stringstream outstream;
      if (params.use_index)
        outstream << "[" << params.index << "] ";
      outstream << "(on " << _d << ") ";

      proto_item *item = proto_tree_add_string_format_value(
        params.tree, get_hf(), params.tvb, params.offset, (gint)len,
        _d.c_str(), outstream.str().c_str()
      );
      proto_tree *subtree = proto_item_add_subtree (item, ett_);

      Wireshark_Bundle_Field fp;
      fp.tvb = params.tvb;
      fp.info = params.info;
      fp.tree = subtree;
      fp.offset = params.offset + (gint)data_pos;
      fp.use_index = false;
      fp.index = 0;
      fp.data = data;

      data_pos += value->dissect_i (fp, label, false);

      return data_pos;
    }

    void Sample_Union::init_ws_fields() {
      add_protocol_field(FT_STRING);
      if (field_ != NULL) field_->init_ws_fields();
      if (default_ != NULL) default_->init_ws_fields();
    }

    //-----------------------------------------------------------------------

    Sample_Alias::Sample_Alias (Sample_Dissector *base)
      :base_(base)
    {
      this->init ("alias");
    }

    size_t
    Sample_Alias::dissect_i (Wireshark_Bundle_i &p, const std::string &l)
    {
      return base_->dissect_i (p, l);
    }

    void Sample_Alias::init_ws_fields() {
      base_->init_ws_fields();
    }

  }
}

OPENDDS_END_VERSIONED_NAMESPACE_DECL
