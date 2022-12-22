/*
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "sample_dissector.h"
#include "sample_manager.h"
#include "ws_common.h"

#include <dds/DCPS/Serializer.h>

#include <ace/Basic_Types.h>
#include <ace/CDR_Base.h>
#include <ace/Message_Block.h>
#include <ace/Log_Msg.h>
#include <ace/ACE.h>

#include <tao/String_Manager_T.h>

#ifndef NO_EXPERT
#  include <epan/expert.h>
#endif

#include <cstring>
#include <algorithm>
#include <iomanip>
#include <sstream>
#include <string>
#include <vector>

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS
{
  namespace DCPS
  {

    /*
     * To make it easy to pass to Wireshark, we should convert DDS wide chars
     * and strings to UTF-8. Returns true if there was an error and sets
     * result to the reason it failed.
     */
    bool utf16_to_utf8(
       std::string& result, const ACE_CDR::WChar* from, const size_t length = 1
    ) {

      const gchar* from_;
#if ACE_SIZEOF_WCHAR == 4
      /*
       * We have to trim the extra 16 bits of zeros between the characters
       * before we can pass it to glib's g_convert. Otherwise it will
       * interpret them as NULL characters and stop after the first two bytes.
       */
      std::vector<guint16> trimmed(length + 1, 0);
      for (size_t i = 0; i < length; i++) {
        trimmed[i] = from[i];
      }
      from_ = reinterpret_cast<gchar*>(&trimmed[0]);
#else
      from_ = reinterpret_cast<gchar*>(const_cast<ACE_CDR::WChar*>(from));
#endif

      GError* error = NULL;
      char* utf8 = g_convert(
        from_, length* sizeof(ACE_CDR::WChar),
        "UTF-8", "UTF-16LE", NULL, NULL, &error
      );

      if (utf8 != NULL) {
        result = utf8;
        g_free(utf8);
        return false;
      } else {
        const char* error_msg = "UTF-16 to UTF-8 conversion failed: ";
        ACE_DEBUG((LM_DEBUG, ACE_TEXT("%C%C\n"),
          error_msg,
          error->message
        ));
        result = error_msg;
        result += error->message;
        g_clear_error(&error);
        return true;
      }
    }

    Wireshark_Bundle::Wireshark_Bundle(
      char* data, size_t size, bool swap_bytes, Encoding::Alignment
    ) :
      block(data, size),
      serializer(&block, Encoding::KIND_UNALIGNED_CDR, swap_bytes)
    {
      block.wr_ptr(data + size);
      get_size_only = false;
    }

    Wireshark_Bundle::Wireshark_Bundle(const Wireshark_Bundle& other) :
      block(other.block.rd_ptr(), other.block.size()),
      serializer(&block, other.serializer.encoding())
    {
      block.wr_ptr(other.block.wr_ptr() - other.block.rd_ptr());

      get_size_only = other.get_size_only;
      tvb = other.tvb;
      info = other.info;
      offset = other.offset;
      use_index = other.use_index;
      index = other.index;
#ifndef NO_EXPERT
      warning_ef = other.warning_ef;
#endif
    }

    size_t Wireshark_Bundle::buffer_pos() {
      return reinterpret_cast<size_t>(block.rd_ptr());
    }

    guint8*
    Wireshark_Bundle::get_remainder()
    {
      gint remainder = static_cast<gint>(ws_tvb_length(tvb) - offset);
      return reinterpret_cast<guint8*>(ws_ep_tvb_memdup(tvb, static_cast<gint>(offset), remainder));
    }

    const hf_register_info Field_Context::default_hf_register_info =
      {0, {0, 0, FT_NONE, 0, NULL, 0, NULL, HFILL}};

    Field_Context::Field_Context(
      const std::string& short_name, const std::string& long_name,
      ftenum ft, field_display_e fd
    ) :
      short_name_(short_name),
      long_name_(long_name),
      hf_(-1),
      hf_info_(default_hf_register_info)
    {
      hf_info_.p_id = &hf_;
      hf_info_.hfinfo.name = short_name_.c_str();
      hf_info_.hfinfo.abbrev = long_name_.c_str();
      hf_info_.hfinfo.type = ft;
      hf_info_.hfinfo.display = fd;
    }

    Sample_Base::~Sample_Base() {
      for (
        Field_Contexts::iterator i = field_contexts_.begin();
        i != field_contexts_.end();
        ++i
      ) {
        delete i->second;
      }
    }

    Field_Context* Sample_Base::get_context() {
      if (field_contexts_.count(ns_)) {
        return field_contexts_[ns_];
      }
      return 0;
    }

    // Sample_Base static members
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
        i != ns_stack_.end(); ++i
      ) {
        ss << "." << *i;
      }
      ns_ = ss.str();
    }

    std::string Sample_Base::get_ns() {
      return payload_namespace + ns_;
    }

    void Sample_Base::push_ns(const std::string& name) {
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

    void Sample_Base::push_idl_name(std::string name) {
      Sample_Base::clear_ns();

      // If name begins with "IDL:", remove it
      const std::string idl_prefix("IDL:");
      if (name.find(idl_prefix) == 0) {
          name.erase(0, idl_prefix.size());
      }

      // If name contains ':', remove everything after the last ':'
      size_t l = name.rfind(":");
      if (l != std::string::npos) {
          name.erase(l, name.size() - l);
      }

      // Push namespace when we find a '/'
      name.push_back('/'); // For last namespace
      l = 0;
      for (size_t index = 0; index != name.size(); index++) {
          if (name[index] == '/') {
              push_ns(name.substr(index - l, l));
              l = 0;
          } else {
              l++;
          }
      }
    }

    void Sample_Base::add_protocol_field() {
      Field_Context* fc = get_context();
      if (fc) {
        Sample_Manager::instance().add_protocol_field(fc->hf_info_);
      }
    }

    Field_Context* Sample_Base::create_context(ftenum ft, field_display_e fd) {
      if (field_contexts_.count(get_ns())) {
        throw Sample_Dissector_Error(
          std::string("Could not create context because it already exists for: ") +
          get_ns()
        );
      }

      Field_Context* fc = new Field_Context(get_label(), get_ns(), ft, fd);
      field_contexts_[ns_] = fc;

      return fc;
    }

    Sample_Field::Sample_Field(IDLTypeID id, const std::string &label)
      : label_(label),
        type_id_(id),
        nested_(0),
        next_(0)
    {
    }

    Sample_Field::Sample_Field(Sample_Dissector* n, const std::string &label)
      : label_(label),
        type_id_(Undefined),
        nested_(n),
        next_(0)
    {
    }

    Sample_Field::~Sample_Field()
    {
      /* This doesn't work since a Dissector can be referenced by multiple
       * fields
       */
      // delete nested_;

      delete next_;
    }

    Sample_Field* Sample_Field::chain(Sample_Field* f)
    {
      if (next_ != 0)
        next_->chain (f);
      else
        next_ = f;
      return f;
    }

    Sample_Field* Sample_Field::chain(IDLTypeID ti, const std::string& l)
    {
      return chain (new Sample_Field (ti, l));
    }

    Sample_Field* Sample_Field::chain(Sample_Dissector* n, const std::string& l)
    {
      return chain (new Sample_Field (n, l));
    }

    Sample_Field* Sample_Field::get_link(size_t index)
    {
      if (index == 0)
        return this;

      return (next_ == 0) ? 0 : next_->get_link (index - 1);
    }

    void Sample_Field::to_stream(std::stringstream& s, Wireshark_Bundle& p) {
      size_t location = p.buffer_pos();

      TAO::String_Manager string_value;
      size_t wstring_length;
      TAO::WString_Manager wstring_value;
      std::string converted;

      switch (type_id_) {
      case Char:
        ACE_CDR::Char char_value;
        if (p.serializer >> char_value) {
          s << char_value;
        }
        break;

      case Boolean:
        ACE_CDR::Boolean boolean_value;
        if (p.serializer >> ACE_InputCDR::to_boolean(boolean_value)) {
          s << (boolean_value ? "true" : "false");
        }
        break;

      case Octet:
        ACE_CDR::Octet octet_value;
        if (p.serializer >> ACE_InputCDR::to_octet(octet_value)) {
          s << octet_value;
        }
        break;

      case WChar:
        ACE_CDR::WChar wchar_value;
        if (p.serializer >> ACE_InputCDR::to_wchar(wchar_value)) {
          utf16_to_utf8(converted, &wchar_value);
          s << converted;
        }
        break;

      case Short:
        ACE_CDR::Short short_value;
        if (p.serializer >> short_value) {
          s << short_value;
        }
        break;

      case Long:
        ACE_CDR::Long long_value;
        if (p.serializer >> long_value) {
          s << long_value;
        }
        break;

      case LongLong:
        ACE_CDR::LongLong longlong_value;
        if (p.serializer >> longlong_value) {
          s << longlong_value;
        }
        break;

      case UShort:
        ACE_CDR::UShort ushort_value;
        if (p.serializer >> ushort_value) {
          s << ushort_value;
        }
        break;

      case ULong:
        ACE_CDR::ULong ulong_value;
        if (p.serializer >> ulong_value) {
          s << ulong_value;
        }
        break;

      case ULongLong:
        ACE_CDR::ULongLong ulonglong_value;
        if (p.serializer >> ulonglong_value) {
          s << ulonglong_value;
        }
        break;

      case Float:
        ACE_CDR::Float float_value;
        if (p.serializer >> float_value) {
          s << float_value;
        }
        break;

      case Double:
        ACE_CDR::Double double_value;
        if (p.serializer >> double_value) {
          s << double_value;
        }
        break;

      case LongDouble:
        ACE_CDR::LongDouble longdouble_value;
        if (p.serializer >> longdouble_value) {
          s << longdouble_value;
        }
        break;

      case String:
        p.serializer.read_string(string_value.inout());
        if (p.serializer.good_bit()) {
          s << string_value;
        }
        break;

      case WString:
        wstring_length = p.serializer.read_string(wstring_value.inout());
        if (p.serializer.good_bit()) {
          utf16_to_utf8(converted, wstring_value, wstring_length);
          s << converted;
        }
        break;

      case Enumeration:
        break; // only the label is used, not directly presented

      default:
        throw Sample_Dissector_Error(
          get_ns()  + " does not have a valid type.");
      }

      if (!p.serializer.good_bit()) {
        throw Sample_Dissector_Error(
          "Failed to deserialize " + get_ns() +
          " that has the type " + IDLTypeID_string(type_id_)
        );
      }

      p.offset += (p.buffer_pos() - location);
    }

#define ADD_FIELD_PARAMS params.tree, hf, params.tvb, (gint) params.offset, (gint) len
    size_t Sample_Field::dissect_i(Wireshark_Bundle& params, bool recur)
    {

      size_t len = 0;

      if (!nested_) {

        int hf = -1;
        if (!params.get_size_only) {
           hf = get_hf();
        }

        if (hf == -1 && !params.get_size_only) {
          throw Sample_Dissector_Error(
            get_ns()  + " is not a registered wireshark field."
          );
        } else {

          size_t location = params.buffer_pos();

          TAO::String_Manager string_value;
          size_t wstring_length;
          TAO::WString_Manager wstring_value;

          // Set Field
          switch (type_id_) {

          case Boolean:
            ACE_CDR::Boolean boolean_value;
            params.serializer >> ACE_InputCDR::to_boolean(boolean_value);
            if (!params.serializer.good_bit()) {
              break;
            }
            len = params.buffer_pos() - location;
            if (!params.get_size_only) {
              if (params.use_index) {
                proto_tree_add_boolean_format(
                  ADD_FIELD_PARAMS, boolean_value,
                  "[%u]: %s", params.index, boolean_value ? "true" : "false"
                );
              } else {
                proto_tree_add_boolean(ADD_FIELD_PARAMS, boolean_value);
              }
            }
            break;

          case Char:
            ACE_CDR::Char char_value;
            params.serializer >> char_value;
            if (!params.serializer.good_bit()) {
              break;
            }
            len = params.buffer_pos() - location;
            if (!params.get_size_only) {
              if (params.use_index) {
                proto_tree_add_string_format(
                  ADD_FIELD_PARAMS, &char_value,
                  "[%u]: %c", params.index, char_value
                );
              } else {
                proto_tree_add_string(ADD_FIELD_PARAMS, &char_value);
              }
            }
            break;

          case WChar:
            ACE_CDR::WChar wchar_value;
            params.serializer >> ACE_InputCDR::to_wchar(wchar_value);
            if (!params.serializer.good_bit()) {
              break;
            }
            len = params.buffer_pos() - location;
            if (!params.get_size_only) {
              std::string s;
              bool utf_fail = utf16_to_utf8(s, &wchar_value);
#ifdef NO_EXPERT
              ACE_UNUSED_ARG(utf_fail);
#else
              if (utf_fail) {
                proto_tree_add_expert_format(
                    params.tree, params.info, params.warning_ef, params.tvb,
                    static_cast<gint>(params.offset), static_cast<gint>(len), "%s", s.c_str()
                );
              }
#endif
              if (params.use_index) {
                proto_tree_add_string_format(
                  ADD_FIELD_PARAMS, s.c_str(),
                  "[%u]: %s", params.index, s.c_str()
                );
              } else {
                proto_tree_add_string_format_value(
                  ADD_FIELD_PARAMS, s.c_str(),
                  "%s", s.c_str()
                );
              }
            }
            break;

          case Octet:
            ACE_CDR::Octet octet_value;
            params.serializer >> ACE_InputCDR::to_octet(octet_value);
            if (!params.serializer.good_bit()) {
              break;
            }
            len = params.buffer_pos() - location;
            if (!params.get_size_only) {
              if (params.use_index) {
                proto_tree_add_uint_format(
                  ADD_FIELD_PARAMS, octet_value,
                  "[%u]: %x", params.index, octet_value
                );
              } else {
                proto_tree_add_uint(ADD_FIELD_PARAMS, octet_value);
              }
            }
            break;

          case Short:
            ACE_CDR::Short short_value;
            params.serializer >> short_value;
            if (!params.serializer.good_bit()) {
              break;
            }
            len = params.buffer_pos() - location;
            if (!params.get_size_only) {
              if (params.use_index) {
                proto_tree_add_int_format(
                  ADD_FIELD_PARAMS, short_value,
                  "[%u]: %d", params.index, short_value
                );
              } else {
                proto_tree_add_int(ADD_FIELD_PARAMS, short_value);
              }
            }
            break;

          case Long:
            ACE_CDR::Long long_value;
            params.serializer >> long_value;
            if (!params.serializer.good_bit()) {
              break;
            }
            len = params.buffer_pos() - location;
            if (!params.get_size_only) {
              if (params.use_index) {
                proto_tree_add_int_format(
                  ADD_FIELD_PARAMS, long_value,
                  "[%u]: %d", params.index, long_value
                );
              } else {
                proto_tree_add_int(ADD_FIELD_PARAMS, long_value);
              }
            }
            break;

          case LongLong:
            ACE_CDR::LongLong longlong_value;
            params.serializer >> longlong_value;
            if (!params.serializer.good_bit()) {
              break;
            }
            len = params.buffer_pos() - location;
            if (!params.get_size_only) {
              if (params.use_index) {
                proto_tree_add_int64_format(
                  ADD_FIELD_PARAMS, longlong_value,
                  "[%u]: %ld", params.index, longlong_value
                );
              } else {
                proto_tree_add_int64(ADD_FIELD_PARAMS, longlong_value);
              }
            }
            break;

          case UShort:
            ACE_CDR::UShort ushort_value;
            params.serializer >> ushort_value;
            if (!params.serializer.good_bit()) {
              break;
            }
            len = params.buffer_pos() - location;
            if (!params.get_size_only) {
              if (params.use_index) {
                proto_tree_add_uint_format(
                  ADD_FIELD_PARAMS, ushort_value,
                  "[%u]: %u", params.index, ushort_value
                );
              } else {
                proto_tree_add_uint(ADD_FIELD_PARAMS, ushort_value);
              }
            }
            break;

          case ULong:
            ACE_CDR::ULong ulong_value;
            params.serializer >> ulong_value;
            if (!params.serializer.good_bit()) {
              break;
            }
            len = params.buffer_pos() - location;
            if (!params.get_size_only) {
              if (params.use_index) {
                proto_tree_add_uint_format(
                  ADD_FIELD_PARAMS, ulong_value,
                  "[%u]: %u", params.index, ulong_value
                );
              } else {
                proto_tree_add_uint(ADD_FIELD_PARAMS, ulong_value);
              }
            }
            break;

          case ULongLong:
            ACE_CDR::ULongLong ulonglong_value;
            params.serializer >> ulonglong_value;
            if (!params.serializer.good_bit()) {
              break;
            }
            len = params.buffer_pos() - location;
            if (!params.get_size_only) {
              if (params.use_index) {
                proto_tree_add_uint64_format(
                  ADD_FIELD_PARAMS, ulonglong_value,
                  "[%u]: %lu", params.index, ulonglong_value
                );
              } else {
                proto_tree_add_uint64(ADD_FIELD_PARAMS, ulonglong_value);
              }
            }
            break;

          case Float:
            ACE_CDR::Float float_value;
            params.serializer >> float_value;
            if (!params.serializer.good_bit()) {
              break;
            }
            len = params.buffer_pos() - location;
            if (!params.get_size_only) {
              if (params.use_index) {
                proto_tree_add_float_format(
                  ADD_FIELD_PARAMS, float_value,
                  "[%u]: %f", params.index, float_value
                );
              } else {
                proto_tree_add_float(ADD_FIELD_PARAMS, float_value);
              }
            }
            break;

          case Double:
            ACE_CDR::Double double_value;
            params.serializer >> double_value;
            if (!params.serializer.good_bit()) {
              break;
            }
            len = params.buffer_pos() - location;
            if (!params.get_size_only) {
              if (params.use_index) {
                proto_tree_add_double_format(
                  ADD_FIELD_PARAMS, double_value,
                  "[%u]: %lf", params.index, double_value
                );
              } else {
                proto_tree_add_double(ADD_FIELD_PARAMS, double_value);
              }
            }
            break;

          case LongDouble:
            ACE_CDR::LongDouble longdouble_value;
            params.serializer >> longdouble_value;
            if (!params.serializer.good_bit()) {
              break;
            }
            len = params.buffer_pos() - location;
            if (!params.get_size_only) {
              // Casting to double because ws doesn't support long double
              double casted_value = static_cast<double>(longdouble_value);
              if (params.use_index) {
                proto_tree_add_double_format(
                  ADD_FIELD_PARAMS, casted_value,
                  "[%u]: %lf", params.index, casted_value
                );
              } else {
                proto_tree_add_double(ADD_FIELD_PARAMS, casted_value);
              }
            }
            break;

          case String:
            // Get String
            params.serializer.read_string(string_value.inout());
            if (!params.serializer.good_bit()) {
              break;
            }
            len = params.buffer_pos() - location;

            // Add to Tree
            if (!params.get_size_only) {
              if (params.use_index) {
                proto_tree_add_string_format(
                  ADD_FIELD_PARAMS, string_value,
                  "[%u]: %s", params.index, string_value.in()
                );
              } else {
                proto_tree_add_string(ADD_FIELD_PARAMS, string_value);
              }
            }

            break;

          case WString:
            // Get String
            wstring_length = params.serializer.read_string(wstring_value.inout());
            if (!params.serializer.good_bit()) {
              break;
            }
            len = params.buffer_pos() - location;

            // Add to Tree
            if (!params.get_size_only) {
              // Get UTF8 Version of the String
              std::string s;
              bool utf_fail = utf16_to_utf8(s, wstring_value, wstring_length);
#ifdef NO_EXPERT
              ACE_UNUSED_ARG(utf_fail);
#else
              if (utf_fail) {
                proto_tree_add_expert_format(
                    params.tree, params.info, params.warning_ef, params.tvb,
                    static_cast<gint>(params.offset), static_cast<gint>(len), "%s", s.c_str()
                );
              }
#endif

              if (params.use_index) {
                proto_tree_add_string_format(
                  ADD_FIELD_PARAMS, s.c_str(),
                  "[%u]: %s", params.index, s.c_str()
                );
              } else {
                proto_tree_add_string_format_value(
                  ADD_FIELD_PARAMS, s.c_str(),
                  "%s", s.c_str()
                );
              }
            }

            break;

          default:
            throw Sample_Dissector_Error(
              get_ns()  + " does not have a valid type.");
          }

          if (!params.serializer.good_bit()) {
            throw Sample_Dissector_Error(
              "Failed to deserialize " + get_ns() +
              " that has the type " + IDLTypeID_string(type_id_)
            );
          }

          params.offset += len;
        }
      } else {
        push_ns(this->label_);
        len = nested_->dissect_i(params);
        pop_ns();
      }

      if (next_ != 0 && recur) {
        len += next_->dissect_i(params);
      }
      return len;
    }

    void Sample_Field::init_ws_fields(bool first_pass) {
      if (!first_pass) {
        add_protocol_field();
      }
      if (label_.empty() && first_pass) {

        switch (type_id_) {

        case Sample_Field::Boolean:
          create_context(FT_BOOLEAN);
          break;

        case Sample_Field::Char:
          create_context(FT_STRING);
          break;

        case Sample_Field::Octet:
          create_context(FT_UINT8, BASE_HEX);
          break;

        case Sample_Field::WChar:
          create_context(FT_STRING);
          break;

        case Sample_Field::Short:
          create_context(FT_INT16, BASE_DEC);
          break;

        case Sample_Field::Long:
          create_context(FT_INT32, BASE_DEC);
          break;

        case Sample_Field::LongLong:
          create_context(FT_INT64, BASE_DEC);
          break;

        case Sample_Field::UShort:
          create_context(FT_UINT16, BASE_DEC);
          break;

        case Sample_Field::ULong:
          create_context(FT_UINT32, BASE_DEC);
          break;

        case Sample_Field::ULongLong:
          create_context(FT_UINT64, BASE_DEC);
          break;

        case Sample_Field::Float:
          create_context(FT_FLOAT);
          break;

        case Sample_Field::Double:
          create_context(FT_DOUBLE);
          break;

        case Sample_Field::LongDouble:
          // Long Doubles will be cast to doubles, resulting in possible
          // data loss if long doubles are larger than doubles.
          create_context(FT_DOUBLE);
          break;

        case Sample_Field::String:
          create_context(FT_STRINGZ);
          break;

        case Sample_Field::WString:
          create_context(FT_STRINGZ);
          break;

        default:
          throw Sample_Dissector_Error(
            get_ns()  + " does not have a valid type.");
        }
      } else if (nested_) {
        push_ns(label_);
        nested_->init_ws_fields(first_pass);
        pop_ns();
      }
      if (next_ != NULL) {
        next_->init_ws_fields(first_pass);
      }
    }

    size_t
    Sample_Field::compute_length(const Wireshark_Bundle & p) {
      Wireshark_Bundle size_params(p);
      size_params.get_size_only = true;
      return dissect_i(size_params);
    }

    //------------------------------------------------------------------------

    Sample_Dissector::Sample_Dissector(const std::string& subtree)
      : field_(0),
        ett_(-1),
        subtree_label_(),
        is_struct_(false),
        is_root_(false)
    {
      if (!subtree.empty()) {
        this->init (subtree);
      }
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
    Sample_Dissector::add_field(Sample_Dissector* n,
                                const std::string &label)
    {
      return add_field(new Sample_Field(n, label));
    }

    size_t
    Sample_Dissector::dissect_i(Wireshark_Bundle& params)
    {
      size_t len = 0;
      if (!params.get_size_only) {
        len = field_->compute_length(params);
      }

      // Add Field and Dissect
      if (field_) {
        bool prev_use_index = params.use_index;
        proto_tree* prev_tree = params.tree;

        if (is_struct_ && !is_root_ && !params.get_size_only) {
          std::stringstream outstream;
          if (params.use_index) {
            params.use_index = false;
            outstream << "[" << params.index << "]";
          } else {
            outstream << get_label();
          }
          int hf = get_hf();
          if (hf == -1) {
            throw Sample_Dissector_Error(
              get_ns()  + " is not a registered wireshark field."
            );
          } else {
            proto_item* item = proto_tree_add_none_format(
              ADD_FIELD_PARAMS, "%s", outstream.str().c_str()
            );
            params.tree = proto_item_add_subtree(item, ett_);
          }
        } else {
          is_root_ = false;
        }

        // Dissect Child Field
        len = field_->dissect_i(params);

        params.tree = prev_tree;
        params.use_index = prev_use_index;
      }

      return len;
    }

    size_t Sample_Dissector::compute_length(const Wireshark_Bundle& p)
    {
      Wireshark_Bundle size_params(p);
      size_params.get_size_only = true;
      return dissect_i(size_params);
    }

    gint Sample_Dissector::dissect (Wireshark_Bundle& params)
    {
      params.use_index = false;
      is_root_ = true;
      return static_cast<gint>(params.offset + this->dissect_i(params));
    }

    Sample_Field::IDLTypeID
    Sample_Dissector::get_field_type ()
    {
      if (field_ == 0 || field_->next_ != 0)
        return Sample_Field::Undefined;
      return field_->type_id_;
    }

    std::string Sample_Dissector::stringify(Wireshark_Bundle& p) {
      std::stringstream outstream;
      if (field_) {
        field_->to_stream(outstream, p);
      }
      return outstream.str();
    }

    void Sample_Dissector::init_ws_proto_tree(bool first_pass) {
      if (field_) {
        field_->init_ws_fields(first_pass);
      }
    }

    void Sample_Dissector::init_ws_fields(bool first_pass) {
      if (first_pass) {
        if (is_struct()) {
          create_context(FT_NONE);
          init("struct");
        }
      } else {
        add_protocol_field();
      }
      if (field_) {
        field_->init_ws_fields(first_pass);
      }
    }

    void Sample_Dissector::mark_struct() {
      is_struct_ = true;
    }

    bool Sample_Dissector::is_struct() {
      return is_struct_;
    }

    //----------------------------------------------------------------------
    Sample_Sequence::Sample_Sequence (Sample_Dissector* sub)
      : element_ (sub)
    {
      this->init ("sequence");
    }

    Sample_Dissector* Sample_Sequence::element()
    {
      return element_;
    }

    void Sample_Sequence::dissect_elements(
      Wireshark_Bundle &params, size_t &len, size_t count, size_t count_size
    ) {

      // Add Base Item
      proto_item *item;
      int hf = -1;
      if (!params.get_size_only) {
        hf = get_hf();
        if (hf == -1) {
          throw Sample_Dissector_Error(
            get_ns()  + " is not a registered wireshark field."
          );
        } else {
          std::stringstream outstream;
          if (params.use_index) {
            outstream << "[" << params.index << "]";
          } else {
            outstream << get_label();
          }
          outstream << " (length = " << count << ")";
          item = proto_tree_add_uint_format(
            ADD_FIELD_PARAMS,
            static_cast<guint32>(count), "%s", outstream.str().c_str()
          );
        }
      }

      params.offset += count_size;

      // Push namespace and new tree
      proto_tree* prev_tree = NULL;
      if (!(params.get_size_only || hf == -1)) {
        prev_tree = params.tree;
        params.tree = proto_item_add_subtree(item, ett_);
      }
      push_ns(element_namespace);

      // Dissect Elements

      bool prev_use_index = params.use_index;
      size_t all_elements_size = 0;
      for (guint32 ndx = 0; ndx < count; ndx++) {
        params.index = ndx;
        params.use_index = true;
        size_t element_size = element_->dissect_i(params);
        all_elements_size += element_size;
      }
      if (params.get_size_only) {
        len += all_elements_size;
      }

      // Cleanup
      params.use_index = prev_use_index;
      pop_ns();
      if (prev_tree) {
        params.tree = prev_tree;
      }
    }

    size_t
    Sample_Sequence::dissect_i (Wireshark_Bundle &params)
    {
      size_t len = 0;
      if (!params.get_size_only) {
        len = compute_length(params);
      }

      size_t location = params.buffer_pos();

      // Get Count of Elements
      ACE_CDR::ULong count;
      params.serializer >> count;
      size_t count_size = params.buffer_pos() - location;
      if (params.get_size_only) {
        len = count_size;
      }

      // Create Tree and Dissect Elements
      dissect_elements(params, len, count, count_size);

      return len;
    }

    void Sample_Sequence::init_ws_fields(bool first_pass) {
      if (first_pass) {
        create_context(FT_UINT32, BASE_DEC);
      } else {
        add_protocol_field();
      }
      push_ns(element_namespace);
      element_->init_ws_fields(first_pass);
      pop_ns();
    }

    //----------------------------------------------------------------------

    Sample_Array::Sample_Array(size_t count, Sample_Dissector* sub)
      : Sample_Sequence (sub),
        count_ (count)
    {
    }

    size_t
    Sample_Array::dissect_i(Wireshark_Bundle &params) {
      size_t len = 0;
      if (!params.get_size_only) {
        len = compute_length(params);
      }

      // Create Tree and Dissect Elements
      dissect_elements(params, len, count_, 0);

      return len;
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
      size_t begin_field = params.buffer_pos();

      // Get Enum Value
      ACE_CDR::ULong value;
      params.serializer >> value;
      size_t len = params.buffer_pos() - begin_field;
      params.offset += len;
      if (params.get_size_only) {
        return len;
      }
      Sample_Field* sf = value_->get_link(value);

      // Add to Tree
      std::stringstream outstream;
      std::string enum_label;
      if (params.use_index) {
        outstream << "[" << params.index << "]";
      } else {
        outstream << get_label();
      }
      outstream << ": ";
      if (sf == 0) {
        outstream.str("");
        outstream
          << get_ns()
          << " should be an enum but has an invalid discriminator: "
          << value
        ;
        throw Sample_Dissector_Error(outstream.str());
      } else {
        outstream << sf->label_;
        enum_label = sf->label_;
      }
      int hf = get_hf();
      if (hf == -1) {
        throw Sample_Dissector_Error(
          get_ns()  + " is not a registered wireshark field."
        );
      } else {
        proto_tree_add_string_format(
          ADD_FIELD_PARAMS, enum_label.c_str(),
          "%s", outstream.str().c_str()
        );
      }

      return len;
    }

    bool
    Sample_Enum::index_of (const std::string &value, size_t &result)
    {
      result = 0;
      Sample_Field *iter = value_;
      while (iter != 0) {
        if (iter->label_.compare (value) == 0) {
          return true;
        }
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

    void Sample_Enum::init_ws_fields(bool first_pass) {
      if (first_pass) {
        create_context(FT_STRING);
      } else {
        add_protocol_field();
      }
      if (value_ != NULL) {
        value_->init_ws_fields(first_pass);
      }
    }

    //----------------------------------------------------------------------

    Sample_Union::Sample_Union () :
      discriminator_(0),
      default_(0)
    {
      this->init ("union");
    }

    Sample_Union::~Sample_Union ()
    {
      delete default_;
    }

    void Sample_Union::discriminator(Sample_Dissector* d)
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
      // Get Type
      size_t len = discriminator_->compute_length(params);
      Sample_Field* value = this->default_;
      std::string _d = discriminator_->stringify(params);
      MapType::const_iterator pos = map_.find(_d);
      if (pos != map_.end()) {
        value = pos->second;
      }

      // Add Item
      bool prev_use_index = params.use_index;
      proto_tree* prev_tree = params.tree;
      if (!params.get_size_only) {
        std::stringstream outstream;
        if (params.use_index) {
          outstream << "[" << params.index << "]";
        } else {
          outstream << get_label();
        }
        outstream << " (on " << _d << ")";
        int hf = get_hf();
        if (hf == -1) {
          throw Sample_Dissector_Error(
            get_ns()  + " is not a registered wireshark field."
          );
        } else {
          proto_item* item = proto_tree_add_string_format(
            ADD_FIELD_PARAMS,
            _d.c_str(), "%s", outstream.str().c_str()
          );
          params.tree = proto_item_add_subtree(item, ett_);
        }
      }

      // Dissect Value
      params.use_index = false;
      len += value->dissect_i(params, false);

      // Cleanup
      params.use_index = prev_use_index;
      params.tree = prev_tree;

      return len;
    }

    void Sample_Union::init_ws_fields(bool first_pass) {
      if (first_pass) {
        create_context(FT_STRING);
      } else {
        add_protocol_field();
      }
      if (field_ != NULL) {
        field_->init_ws_fields(first_pass);
      }
      if (default_ != NULL) {
        default_->init_ws_fields(first_pass);
      }
    }

    //-----------------------------------------------------------------------

    Sample_Alias::Sample_Alias(Sample_Dissector* base)
      :base_(base)
    {
      this->init ("alias");
    }

    size_t
    Sample_Alias::dissect_i (Wireshark_Bundle &p)
    {
      return base_->dissect_i(p);
    }

    void Sample_Alias::init_ws_fields(bool first_pass) {
      base_->init_ws_fields(first_pass);
    }

    //-----------------------------------------------------------------------

    Sample_Fixed::Sample_Fixed(unsigned digits, unsigned scale)
    :
      Sample_Dissector(""),
      digits_(digits),
      scale_(scale)
    {
    }

    void Sample_Fixed::init_ws_fields(bool first_pass) {
      if (first_pass) {
        create_context(FT_DOUBLE);
      } else {
        add_protocol_field();
      }
    }

    size_t Sample_Fixed::dissect_i(Wireshark_Bundle &params) {
      // Based on FACE/Fixed.h
      // Fixed_T could not be used here because it is a templated class
      ACE_CDR::UShort len = static_cast<ACE_CDR::UShort>((digits_ + 2) / 2);

      if (params.get_size_only) {
        params.serializer.skip(len);
      } else {
        int hf = get_hf();
        if (hf == -1) {
          throw Sample_Dissector_Error(
            get_ns()  + " is not a registered wireshark field."
          );
        }

#ifdef ACE_HAS_CDR_FIXED
        // Extract a ACE_CDR::Fixed, give it to WS as a double, and display
        // it using Fixed.to_string().
        FACE::Octet raw[(ACE_CDR::Fixed::MAX_DIGITS + 2) / 2];
        if (params.serializer.read_octet_array(raw, len)) {
          ACE_CDR::Fixed fixed_value = ACE_CDR::Fixed::from_octets(
            raw, len, scale_);
          char string_value[ACE_CDR::Fixed::MAX_STRING_SIZE];
          fixed_value.to_string(
            &string_value[0], ACE_CDR::Fixed::MAX_STRING_SIZE);
          double double_value = static_cast<ACE_CDR::LongDouble>(
            fixed_value);
          if (params.use_index) {
            proto_tree_add_double_format(
              ADD_FIELD_PARAMS, double_value,
              "[%u]: %s", params.index, &string_value[0]
            );
          } else {
            proto_tree_add_double_format_value(
              ADD_FIELD_PARAMS, double_value,
              "%s", &string_value[0]
            );
          }
        } else {
          throw Sample_Dissector_Error("Error Reading Fixed Type");
        }
#else // ACE_HAS_CDR_FIXED
        // Place Dummy value and inform user
        const char * missing_fixed = "Fixed Type Support is missing from ACE";
        if (params.use_index) {
          proto_tree_add_double_format(
            ADD_FIELD_PARAMS, 0, "[%u]: %s", params.index, missing_fixed
          );
        } else {
          proto_tree_add_double_format_value(
            ADD_FIELD_PARAMS, 0, "%s", missing_fixed
          );
        }
#ifndef NO_EXPERT
        proto_tree_add_expert(
            params.tree, params.info, params.warning_ef, params.tvb,
            params.offset, len, missing_fixed
        );
#endif // NO_EXPERT

        // and skip it.
        params.serializer.skip(len);

#endif // ACE_HAS_CDR_FIXED

      }

      return len;
    }

  }
}

OPENDDS_END_VERSIONED_NAMESPACE_DECL
