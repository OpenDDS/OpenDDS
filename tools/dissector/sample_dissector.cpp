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
    Wireshark_Bundle::Wireshark_Bundle(
      char * data, size_t size, bool swap_bytes, Serializer::Alignment align
    ) :
      block(data, size),
      serializer(&block, swap_bytes, align)
    {
      block.wr_ptr(data);
    }

    Wireshark_Bundle::Wireshark_Bundle(const Wireshark_Bundle & other) :
      block(other.block.rd_ptr(), other.block.size()),
      serializer(
        &block,
        other.serializer.swap_bytes(),
        other.serializer.alignment()
      )
    {
      block.wr_ptr(other.block.rd_ptr());

      get_size_only = other.get_size_only;
      tvb = other.tvb;
      info = other.info;
      offset = other.offset;
      use_index = other.use_index;
      index = other.index;
    }

    size_t Wireshark_Bundle::buffer_pos() {
      return reinterpret_cast<size_t>(block.rd_ptr());
    }

    guint8 *
    Wireshark_Bundle::get_remainder()
    {
      gint remainder = ws_tvb_length(tvb) - offset;
      return reinterpret_cast<guint8 *>(ws_ep_tvb_memdup(tvb, offset, remainder));
    }

    Sample_Base::~Sample_Base() {
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

    std::string Sample_Base::get_label() {
      return ns_stack_.back();
    }

    void Sample_Base::add_protocol_field(enum ftenum ft, field_display_e fd) {
      Field_Context * fc = get_context();
      Sample_Manager::instance().add_protocol_field(
        &fc->hf_, get_ns(), fc->label_, ft, fd
      );
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

    void Sample_Field::to_stream(std::stringstream &s, Wireshark_Bundle & p) {
      size_t location = p.buffer_pos();

      switch (this->type_id_) {
      case Char:
        ACE_CDR::Char char_value;
        p.serializer >> char_value;
        s << char_value;
        break;

      case Boolean:
        ACE_CDR::Boolean boolean_value;
        p.serializer >> ACE_InputCDR::to_boolean(boolean_value);
        if (boolean_value)
          s << "true";
        else
          s << "false";
        break;

      case Octet:
        ACE_CDR::Octet octet_value;
        p.serializer >> ACE_InputCDR::to_octet(octet_value);
        s << octet_value;
        break;

      case WChar:
        ACE_CDR::WChar wchar_value;
        p.serializer >> ACE_InputCDR::to_wchar(wchar_value);
        s << wchar_value;
        break;

      case Short:
        ACE_CDR::Short short_value;
        p.serializer >> short_value;
        s << short_value;
        break;

      case Long:
        ACE_CDR::Long long_value;
        p.serializer >> long_value;
        s << long_value;
        break;

      case LongLong:
        ACE_CDR::LongLong longlong_value;
        p.serializer >> longlong_value;
        s << longlong_value;
        break;

      case UShort:
        ACE_CDR::UShort ushort_value;
        p.serializer >> ushort_value;
        s << ushort_value;
        break;

      case ULong:
        ACE_CDR::ULong ulong_value;
        p.serializer >> ulong_value;
        s << ulong_value;
        break;

      case ULongLong:
        ACE_CDR::ULongLong ulonglong_value;
        p.serializer >> ulonglong_value;
        s << ulonglong_value;
        break;

      case Float:
        ACE_CDR::Float float_value;
        p.serializer >> float_value;
        s << float_value;
        break;

      case Double:
        ACE_CDR::Double double_value;
        p.serializer >> double_value;
        s << double_value;
        break;

      case LongDouble:
        ACE_CDR::LongDouble longdouble_value;
        p.serializer >> longdouble_value;
        s << longdouble_value;
        break;

      case String:
        // Length
        ACE_CDR::ULong string_length;
        p.serializer >> string_length;
        // Data
        ACE_CDR::Char * string_value;
        string_value = new ACE_CDR::Char[string_length];
        p.serializer.read_char_array(string_value, string_length);
        s << string_value;
        delete [] string_value;
        break;

      case WString:
        // Length
        ACE_CDR::ULong wstring_length;
        p.serializer >> wstring_length;
        
        // Data
        // TODO
        ACE_CDR::WChar * wstring_value;
        wstring_value = new ACE_CDR::WChar[wstring_length];
        p.serializer.read_wchar_array(wstring_value, wstring_length);
        s << wstring_value;
        delete [] wstring_value;
        break;

      case Enumeration:
        break; // only the label is used, not directly presented

      case Undefined:
        s << "type undefined";

      }

      p.offset += (p.buffer_pos() - location);
    }

#define ADD_FIELD_PARAMS params.tree, hf, params.tvb, params.offset, (gint) len
    size_t
    Sample_Field::dissect_i (Wireshark_Bundle &params, bool recur) {

      size_t len = 0;

      if (this->nested_) {

        int hf = get_hf();
        if (hf == -1) {
          ACE_DEBUG((LM_DEBUG, ACE_TEXT("%s is not a registered wireshark field.\n"),
                     get_ns().c_str()));
          // TODO What to do after this

        } else {

          size_t location = params.buffer_pos();

          // Set Field
          switch (this->type_id_) {

          case Sample_Field::Boolean:
            ACE_CDR::Boolean boolean_value;
            params.serializer >> ACE_InputCDR::to_boolean(boolean_value);
            len = params.buffer_pos() - location;
            if (!params.get_size_only) {
              if (params.use_index) {
                proto_tree_add_boolean_format_value(
                  ADD_FIELD_PARAMS, boolean_value,
                  "%s[%u]: %s", get_label().c_str(), params.index,
                  boolean_value ? "True" : "False"
                );
              } else {
                proto_tree_add_boolean(ADD_FIELD_PARAMS, boolean_value);
              }
            }
            break;

          case Sample_Field::Char:
            ACE_CDR::Char char_value;
            params.serializer >> char_value;
            len = params.buffer_pos() - location;
            if (!params.get_size_only) {
              if (params.use_index) {
                proto_tree_add_string_format_value(
                  ADD_FIELD_PARAMS, &char_value,
                  "%s[%u]: %c", get_label().c_str(), params.index, char_value
                );
              } else {
                proto_tree_add_string(ADD_FIELD_PARAMS, &char_value);
              }
            }
            break;

          case Sample_Field::WChar:
            ACE_CDR::WChar wchar_value;
            params.serializer >> ACE_InputCDR::to_wchar(wchar_value);
            len = params.buffer_pos() - location;
            /*
            if (params.use_index) {
              proto_tree_add_string_format_value(
                ADD_FIELD_PARAMS, &wchar_value,
                "%s[%u]: %lc",
                get_label().c_str(),
                params.index,
                wchar_value
              );
            } else {
              proto_tree_add_string_format(
                ADD_FIELD_PARAMS, &wchar_value,
                "%lc", wchar_value
              );
            }
            */
            break;

          case Sample_Field::Octet:
            ACE_CDR::Octet octet_value;
            params.serializer >> ACE_InputCDR::to_octet(octet_value);
            len = params.buffer_pos() - location;
            if (!params.get_size_only) {
              if (params.use_index) {
                proto_tree_add_uint_format_value(
                  ADD_FIELD_PARAMS, octet_value,
                  "%s[%u]: %x", get_label().c_str(), params.index, octet_value
                );
              } else {
                proto_tree_add_uint(ADD_FIELD_PARAMS, octet_value);
              }
            }
            break;

          case Sample_Field::Short:
            ACE_CDR::Short short_value;
            params.serializer >> short_value;
            len = params.buffer_pos() - location;
            if (!params.get_size_only) {
              if (params.use_index) {
                proto_tree_add_int_format_value(
                  ADD_FIELD_PARAMS, short_value,
                  "%s[%u]: %d", get_label().c_str(), params.index, short_value
                );
              } else {
                proto_tree_add_int(ADD_FIELD_PARAMS, short_value);
              }
            }
            break;

          case Sample_Field::Long:
            ACE_CDR::Long long_value;
            params.serializer >> long_value;
            len = params.buffer_pos() - location;
            if (!params.get_size_only) {
              if (params.use_index) {
                proto_tree_add_int_format_value(
                  ADD_FIELD_PARAMS, long_value,
                  "%s[%u]: %d", get_label().c_str(), params.index, long_value
                );
              } else {
                proto_tree_add_int(ADD_FIELD_PARAMS, long_value);
              }
            }
            break;

          case Sample_Field::LongLong:
            ACE_CDR::LongLong longlong_value;
            params.serializer >> longlong_value;
            len = params.buffer_pos() - location;
            if (!params.get_size_only) {
              if (params.use_index) {
                proto_tree_add_int64_format_value(
                  ADD_FIELD_PARAMS, longlong_value,
                  "%s[%u]: %ld", get_label().c_str(), params.index,
                  longlong_value
                );
              } else {
                proto_tree_add_int64(ADD_FIELD_PARAMS, longlong_value);
              }
            }
            break;

          case Sample_Field::UShort:
            ACE_CDR::UShort ushort_value;
            params.serializer >> ushort_value;
            len = params.buffer_pos() - location;
            if (!params.get_size_only) {
              if (params.use_index) {
                proto_tree_add_uint_format_value(
                  ADD_FIELD_PARAMS, ushort_value,
                  "%s[%u]: %u", get_label().c_str(), params.index,
                  ushort_value
                );
              } else {
                proto_tree_add_uint(ADD_FIELD_PARAMS, ushort_value);
              }
            }
            break;

          case Sample_Field::ULong:
            ACE_CDR::ULong ulong_value;
            params.serializer >> ulong_value;
            len = params.buffer_pos() - location;
            if (!params.get_size_only) {
              if (params.use_index) {
                proto_tree_add_uint_format_value(
                  ADD_FIELD_PARAMS, ulong_value,
                  "%s[%u]: %u", get_label().c_str(), params.index,
                  ulong_value
                );
              } else {
                proto_tree_add_uint(ADD_FIELD_PARAMS, ulong_value);
              }
            }
            break;

          case Sample_Field::ULongLong:
            ACE_CDR::ULongLong ulonglong_value;
            params.serializer >> ulonglong_value;
            len = params.buffer_pos() - location;
            if (!params.get_size_only) {
              if (params.use_index) {
                proto_tree_add_uint64_format_value(
                  ADD_FIELD_PARAMS, ulonglong_value,
                  "%s[%u]: %lu", get_label().c_str(), params.index,
                  ulonglong_value
                );
              } else {
                proto_tree_add_uint64(ADD_FIELD_PARAMS, ulonglong_value);
              }
            }
            break;

          case Sample_Field::Float:
            ACE_CDR::Float float_value;
            params.serializer >> float_value;
            len = params.buffer_pos() - location;
            if (!params.get_size_only) {
              if (params.use_index) {
                proto_tree_add_float_format_value(
                  ADD_FIELD_PARAMS, float_value,
                  "%s[%u]: %f", get_label().c_str(), params.index,
                  float_value
                );
              } else {
                proto_tree_add_float(ADD_FIELD_PARAMS, float_value);
              }
            }
            break;

          case Sample_Field::Double:
            ACE_CDR::Double double_value;
            params.serializer >> double_value;
            len = params.buffer_pos() - location;
            if (!params.get_size_only) {
              if (params.use_index) {
                proto_tree_add_double_format_value(
                  ADD_FIELD_PARAMS, double_value,
                  "%s[%u]: %lf", get_label().c_str(), params.index,
                  double_value
                );
              } else {
                proto_tree_add_double(ADD_FIELD_PARAMS, double_value);
              }
            }
            break;

          case Sample_Field::LongDouble:
            ACE_CDR::LongDouble longdouble_value;
            params.serializer >> longdouble_value;
            len = params.buffer_pos() - location;
            if (!params.get_size_only) {
              // Casting to double because ws doesn't support long double
              double casted_value = static_cast<double>(longdouble_value);
              if (params.use_index) {
                proto_tree_add_double_format_value(
                  ADD_FIELD_PARAMS, casted_value,
                  "%s[%u]: %lf", get_label().c_str(), params.index,
                  casted_value
                );
              } else {
                proto_tree_add_double(ADD_FIELD_PARAMS, casted_value);
              }
            }
            break;

          case Sample_Field::String:
            // Length
            ACE_CDR::ULong string_length;
            params.serializer >> string_length;
            // Data
            ACE_CDR::Char * string_value;
            string_value = new ACE_CDR::Char[string_length];
            params.serializer.read_char_array(string_value, string_length);
            len = params.buffer_pos() - location;

            // Add to Tree
            if (params.use_index) {
              proto_tree_add_string_format_value(
                ADD_FIELD_PARAMS,
                string_value,
                "%s[%u]: %s",
                get_label().c_str(),
                params.index,
                string_value 
              );
            } else {
              proto_tree_add_string(
                ADD_FIELD_PARAMS, string_value 
              );
            }
            delete [] string_value;
            break;

          case Sample_Field::WString:
            // Length
            ACE_CDR::ULong wstring_length;
            params.serializer >> wstring_length;

            // TODO
            ACE_CDR::WChar throwaway;
            for (unsigned i = 0; i < wstring_length; i++) {
              params.serializer >> ACE_InputCDR::to_wchar(throwaway);
            }
            len = params.buffer_pos() - location;
            /*
            proto_tree_add_item(
              params.tree, hf,
              params.tvb, params.offset + 4,
              Serializer::WCHAR_SIZE * *(reinterpret_cast< guint32 * >(params.data)),
              ENC_UTF_16
            );
            */
            break;

          default:
            // TODO Figureout what to do here
            break;
          }

          params.offset += len;
        }
      } else {
        push_ns(this->label_);
        len = this->nested_->dissect_i(params);
        pop_ns();
      }

      if (next_ != 0 && recur) {
        len += next_->dissect_i(params);
      }
      return len;
    }

    void Sample_Field::init_ws_fields() {
      if (label_.empty()) {
        
        switch (type_id_) {

        case Sample_Field::Boolean:
          add_protocol_field(FT_BOOLEAN);
          break;

/* Apparently FT_CHAR is not defined in Wireshark 1.x
 * So this ifdef must be put here to allow the dissector to compile under
 * Wireshark 1 even though it's never going to run this.
 */
#ifndef WS_1
        case Sample_Field::Char:
          add_protocol_field(FT_CHAR);
          break;
#endif

        case Sample_Field::Octet:
          add_protocol_field(FT_UINT8, BASE_HEX);
          break;

        case Sample_Field::WChar:
          add_protocol_field(FT_STRINGZ);
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
          add_protocol_field(FT_STRINGZ);
          break;

        case Sample_Field::WString:
          add_protocol_field(FT_STRINGZ);
          break;

        default:
          // TODO Handle Unknown Type
          break;
        }
      } else if (nested_ != NULL) {
        push_ns(label_);
        nested_->init_ws_fields();
        pop_ns();
      }
      if (next_ != NULL) {
        next_->init_ws_fields();
      }
    }

    size_t
    Sample_Field::compute_length(const Wireshark_Bundle & p) {
      Wireshark_Bundle size_params(p);
      size_params.get_size_only = true;
      return dissect_i(size_params);
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
    Sample_Dissector::dissect_i (Wireshark_Bundle &params)
    {
      size_t len = 0;
      if (!params.get_size_only) {
        len = field_->compute_length(params);
      }

      /*
      std::stringstream outstream;
      bool top_level = true;
      if (!label.empty())
        {
          top_level = false;
          outstream << label;
          if (params.use_index)
            outstream << "[" << params.index << "] ";
        }
      */

      if (field_) {
        proto_tree* keep_tree = params.tree;
        if (is_struct_ && !is_root_) {
          if (!params.get_size_only) {
            int hf = get_hf();
            // TODO HANDLE hf = -1
            proto_item* item = proto_tree_add_none_format(
              ADD_FIELD_PARAMS,
              get_label().c_str()
            );
            params.tree = proto_item_add_subtree(item, ett_);
          }
        } else {
          if (is_root_) is_root_ = false;
        }
        len = field_->dissect_i(params);
        params.tree = keep_tree;
      }

      return len;
    }

    size_t
    Sample_Dissector::compute_length(const Wireshark_Bundle & p) {
      Wireshark_Bundle size_params(p);
      size_params.get_size_only = true;
      return dissect_i(size_params);
    }

    gint
    Sample_Dissector::dissect (Wireshark_Bundle &params)
    {
      params.use_index = false;
      is_root_ = true;
      return params.offset + (gint) this->dissect_i(params);
    }

    Sample_Field::IDLTypeID
    Sample_Dissector::get_field_type ()
    {
      if (field_ == 0 || field_->next_ != 0)
        return Sample_Field::Undefined;
      return field_->type_id_;
    }

    std::string Sample_Dissector::stringify(Wireshark_Bundle & p) {
      std::stringstream outstream;
      if (field_) {
        field_->to_stream(outstream, p);
      }
      return outstream.str();
    }

    void Sample_Dissector::init_ws_proto_tree() {
      if (field_) {
        field_->init_ws_fields();
      }
    }

    void Sample_Dissector::init_ws_fields() {
      if (is_struct()) {
        add_protocol_field(FT_NONE);
        init("struct");
      }
      if (field_) {
        field_->init_ws_fields();
      }
    }

    void Sample_Dissector::mark_struct() {
      is_struct_ = true;
    }

    bool Sample_Dissector::is_struct() {
      return is_struct_;
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
    Sample_Sequence::dissect_i (Wireshark_Bundle &params)
    {
      size_t location = params.buffer_pos();
      size_t len = 0;
      if (!params.get_size_only) {
        len = compute_length(params);
      }

      // Get Count of Elements
      ACE_CDR::ULong count;
      params.serializer >> count;
      size_t count_size = params.buffer_pos() - location;
      if (params.get_size_only) {
        len = count_size;
      }
      params.offset += count_size;

      // Add Item
      proto_item *item;
      if (!params.get_size_only) {
        int hf = get_hf();
        // TODO: HANDLE hf = -1
        std::stringstream outstream;
        if (params.use_index)
          outstream << "[" << params.index << "] ";
        outstream << "(length = " << count << ") ";
        item = proto_tree_add_uint_format(
          ADD_FIELD_PARAMS,
          count, outstream.str().c_str()
        );
      }

      // Push namespace and new tree
      proto_tree* keep_tree;
      if (!params.get_size_only) {
        keep_tree = params.tree;
        params.tree = proto_item_add_subtree(item, ett_);
      }
      push_ns(element_namespace);

      // Dissect Elements
      size_t element_size = 0;
      size_t all_elements_size = 0;
      for (guint32 ndx = 0; ndx < count; ndx++) {
        params.index = ndx;
        params.use_index = true;
        element_size = element_->dissect_i(params);
        all_elements_size += element_size;
      }

      pop_ns();
      if (params.get_size_only) {
        len += all_elements_size;
      }
      params.tree = keep_tree;
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
    Sample_Array::dissect_i(Wireshark_Bundle &params) {
      size_t begin_field;
      size_t len = 0;
      if (params.get_size_only) {
        begin_field = params.buffer_pos();
      } else {
        len = compute_length(params);
      }

      // Add Item
      proto_item *item;
      if (!params.get_size_only) {
        int hf = get_hf();
        // TODO: HANDLE hf = -1
        std::stringstream outstream;
        if (params.use_index)
          outstream << "[" << params.index << "] ";
        outstream << "(length = " << count_ << ") ";
        item = proto_tree_add_uint_format(
          ADD_FIELD_PARAMS,
          count_, outstream.str().c_str()
        );
      }

      // Push namespace and new tree
      proto_tree* keep_tree;
      if (!params.get_size_only) {
        keep_tree = params.tree;
        params.tree = proto_item_add_subtree(item, ett_);
      }
      push_ns(element_namespace);

      // Dissect Elements
      size_t element_size = 0;
      size_t all_elements_size = 0;
      for (guint32 ndx = 0; ndx < count_; ndx++) {
        params.index = ndx;
        params.use_index = true;
        element_size = element_->dissect_i(params);
        all_elements_size += element_size;
      }

      if (params.get_size_only) {
        len += all_elements_size;
      }
      pop_ns();
      params.tree = keep_tree;
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
    Sample_Enum::dissect_i (Wireshark_Bundle &params)
    {
      size_t begin_field;
      size_t len = 0;
      if (params.get_size_only) {
        begin_field = params.buffer_pos();
      } else {
        len = compute_length(params);
      }

      // Get Enum Value
      ACE_CDR::ULong value;
      params.serializer >> value;
      size_t value_size = params.buffer_pos() - begin_field;
      params.offset += value_size;
      if (params.get_size_only) {
        return value_size;
      }
      Sample_Field* sf = value_->get_link(value);

      // Add to Tree
      std::stringstream outstream;
      if (params.use_index)
        outstream << "[" << params.index << "] ";
      if (sf == 0)
        outstream << "<value out of bounds: " << value << "> ";
      else
        outstream << sf->label_;
      int hf = get_hf();
      // TODO: HANDLE hf = -1
      proto_tree_add_string(ADD_FIELD_PARAMS, outstream.str().c_str());

      return len;
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
    Sample_Enum::stringify(Wireshark_Bundle & p)
    {
      size_t begin_field = p.buffer_pos();
      ACE_CDR::ULong value;
      p.serializer >> value;
      p.offset += p.buffer_pos() - begin_field;

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

    size_t Sample_Union::dissect_i (Wireshark_Bundle &params) {
      size_t len = discriminator_->compute_length(params);

      // Get Union Value
      Sample_Field* value = this->default_;
      std::string _d = discriminator_->stringify(params);
      MapType::const_iterator pos = map_.find(_d);
      if (pos != map_.end()) {
        value = pos->second;
      }

      proto_tree* subtree;
      proto_tree* keep_tree = params.tree;
      if (!params.get_size_only) {
        std::stringstream outstream;
        if (params.use_index)
          outstream << "[" << params.index << "] ";
        outstream << "(on " << _d << ") ";
        int hf = get_hf();
        // TODO: HANDLE hf = -1
        proto_item* item = proto_tree_add_string_format_value(
          ADD_FIELD_PARAMS,
          _d.c_str(), outstream.str().c_str()
        );
        subtree = proto_item_add_subtree(item, ett_);
      }

      len += value->dissect_i(params, false);

      params.tree = keep_tree;
      return len;
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
    Sample_Alias::dissect_i (Wireshark_Bundle &p)
    {
      return base_->dissect_i (p);
    }

    void Sample_Alias::init_ws_fields() {
      base_->init_ws_fields();
    }

  }
}

OPENDDS_END_VERSIONED_NAMESPACE_DECL
