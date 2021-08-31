/*
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "itl_generator.h"

#include "be_extern.h"
#include "global_extern.h"

#include <dds/DCPS/Definitions.h>

#include <utl_identifier.h>
#include <utl_labellist.h>
#include <ast_fixed.h>

using namespace AstTypeClassification;

std::ostream&
operator<<(std::ostream& out,
           const itl_generator::Indent& i)
{
  out << std::string(static_cast<size_t> (i.generator->level_ * 2), ' ');
  return out;
}

std::ostream&
operator<<(std::ostream& out,
           const itl_generator::Open& i)
{
  i.generator->level_ += 1;
  return out;
}

std::ostream&
operator<<(std::ostream& out,
           const itl_generator::Close& i)
{
  i.generator->level_ -= 1;
  return out;
}

struct InlineType {
  AST_Type* type;

  explicit InlineType(AST_Type* t)
    : type(t)
  { }
};

std::ostream&
operator<<(std::ostream& out,
           const InlineType& it)
{
  AST_Typedef* td = dynamic_cast<AST_Typedef*>(it.type);
  if (td) {
    be_global->itl_ << '"' << it.type->repoID() << '"';
    return out;
  }

  Classification c = classify(it.type);
  if (c & CL_STRING) {
    // TODO:  Support bounded strings.
    if (c & CL_WIDE) {
      be_global->itl_ << "{ \"kind\" : \"string\", \"note\" : { \"idl\" : { \"type\" : \"wstring\" } } }";
    }
    else {
      be_global->itl_ << "{ \"kind\" : \"string\" }";
    }
  }
  else if (c & CL_PRIMITIVE) {
    switch (dynamic_cast<AST_PredefinedType*>(it.type)->pt()) {
    case AST_PredefinedType::PT_long:
      be_global->itl_ << "{ \"kind\" : \"int\", \"bits\" : 32 }";
      break;
    case AST_PredefinedType::PT_ulong:
      be_global->itl_ << "{ \"kind\" : \"int\", \"bits\" : 32, \"unsigned\" : true}";
      break;
    case AST_PredefinedType::PT_longlong:
      be_global->itl_ << "{ \"kind\" : \"int\", \"bits\" : 64 }";
      break;
    case AST_PredefinedType::PT_ulonglong:
      be_global->itl_ << "{ \"kind\" : \"int\", \"bits\" : 64, \"unsigned\" : true}";
      break;
    case AST_PredefinedType::PT_short:
      be_global->itl_ << "{ \"kind\" : \"int\", \"bits\" : 16 }";
      break;
    case AST_PredefinedType::PT_ushort:
      be_global->itl_ << "{ \"kind\" : \"int\", \"bits\" : 16, \"unsigned\" : true}";
      break;
    case AST_PredefinedType::PT_float:
      be_global->itl_ << "{ \"kind\" : \"float\", \"model\" : \"binary32\" }";
      break;
    case AST_PredefinedType::PT_double:
      be_global->itl_ << "{ \"kind\" : \"float\", \"model\" : \"binary64\" }";
      break;
    case AST_PredefinedType::PT_longdouble:
      be_global->itl_ << "{ \"kind\" : \"float\", \"model\" : \"binary128\" }";
      break;
    case AST_PredefinedType::PT_char:
      be_global->itl_ << "{ \"kind\" : \"int\", \"bits\" : 8, \"note\" : { \"presentation\" : { \"type\" : \"char\" } } }";
      break;
    case AST_PredefinedType::PT_wchar:
      be_global->itl_ << "{ \"kind\" : \"int\", \"note\" : { \"presentation\" : { \"type\" : \"char\" }, \"idl\" : { \"type\" : \"wchar\" } } }";
      break;
    case AST_PredefinedType::PT_boolean:
      be_global->itl_ << "{ \"kind\" : \"int\", \"bits\" : 1, \"note\" : { \"presentation\" : { \"type\" : \"bool\" } } }";
      break;
    case AST_PredefinedType::PT_octet:
      be_global->itl_ << "{ \"kind\" : \"int\", \"bits\" : 8, \"unsigned\" : true, "
        "\"note\" : { \"presentation\" : { \"type\" : \"byte\" } }  }";
      break;
#if OPENDDS_HAS_EXPLICIT_INTS
    case AST_PredefinedType::PT_uint8:
      be_global->itl_ << "{ \"kind\" : \"int\", \"bits\" : 8, \"unsigned\" : true }";
      break;
    case AST_PredefinedType::PT_int8:
      be_global->itl_ << "{ \"kind\" : \"int\", \"bits\" : 8 }";
      break;
#endif
    case AST_PredefinedType::PT_any:
    case AST_PredefinedType::PT_object:
    case AST_PredefinedType::PT_value:
    case AST_PredefinedType::PT_abstract:
    case AST_PredefinedType::PT_void:
    case AST_PredefinedType::PT_pseudo:
      // TODO
      break;
    }
  }
  else {
    be_global->itl_ << '"' << it.type->repoID() << '"';
  }

  return out;
}

void itl_generator::gen_prologue()
{
  be_global->itl_ << Indent(this) << "{\n"
                  << Open(this)
                  << Indent(this) << "\"types\" :\n"
                  << Open(this)
                  << Indent(this) << "[\n";
}

void itl_generator::gen_epilogue()
{
  be_global->itl_ << Indent(this) << "]\n"
                  << Close(this)
                  << Close(this)
                  << Indent(this) << "}\n";
}

void itl_generator::new_type()
{
  if (count_ > 0)
    be_global->itl_ << Indent(this) << ",\n";
  ++count_;
}

bool itl_generator::gen_enum(AST_Enum*, UTL_ScopedName* /*name*/,
                             const std::vector<AST_EnumVal*>& contents, const char* repoid)
{
  new_type();

  be_global->itl_ << Open(this)
                  << Indent(this) << "{\n"
                  << Open(this)
                  << Indent(this) << "\"kind\" : \"alias\",\n"
                  << Indent(this) << "\"name\" : \"" << repoid << "\",\n"
                  << Indent(this) << "\"type\" :\n"
                  << Open(this)
                  << Indent(this) << "{\n"
                  << Open(this)
                  << Indent(this) << "\"kind\" : \"int\",\n"
                  << Indent(this) << "\"bits\" : 32,\n"
                  << Indent(this) << "\"unsigned\" : true,\n"
                  << Indent(this) << "\"constrained\" : true,\n"
                  << Indent(this) << "\"values\" : {";

  for (size_t i = 0; i < contents.size(); ++i) {
    if (i > 0)
      be_global->itl_ << ", ";
    be_global->itl_ << '"' << contents[i]->local_name()->get_string() << '"'
                    << " : "
                    << '"' << i << '"';
  }

  be_global->itl_ << "}\n";
  be_global->itl_ << Close(this)
                  << Indent(this) << "}\n"
                  << Close(this);

  be_global->itl_ << Close(this)
                  << Indent(this) << "}\n"
                  << Close(this);

  return true;
}

bool itl_generator::gen_typedef(AST_Typedef*, UTL_ScopedName* /*name*/,
                                AST_Type* base,
                                const char* repoid)
{
  new_type();

  switch (base->node_type()) {
  case AST_Decl::NT_sequence:
    {
      AST_Sequence *seq = dynamic_cast<AST_Sequence*>(base);
      be_global->itl_ << Open(this)
                      << Indent(this) << "{\n"
                      << Open(this)
                      << Indent(this) << "\"kind\" : \"alias\",\n"
                      << Indent(this) << "\"name\" : \"" << repoid << "\",\n"
                      << Indent(this) << "\"type\" :\n"
                      << Open(this)
                      << Indent(this) << "{\n"
                      << Open(this)
                      << Indent(this) << "\"kind\" : \"sequence\",\n";
      if (!seq->unbounded()) {
        be_global->itl_ << Indent(this) << "\"capacity\" : " << seq->max_size()->ev()->u.ulval << ",\n";
      }
      be_global->itl_ << Indent(this) << "\"type\" : " << InlineType(seq->base_type()) << "\n"
                      << Close(this)
                      << Indent(this) << "}\n"
                      << Close(this)
                      << Close(this)
                      << Indent(this) << "}\n"
                      << Close(this);
      break;
    }
  case AST_Decl::NT_array:
    {
      AST_Array* arr = dynamic_cast<AST_Array*>(base);
      be_global->itl_ << Open(this)
                      << Indent(this) << "{\n"
                      << Open(this)
                      << Indent(this) << "\"kind\" : \"alias\",\n"
                      << Indent(this) << "\"name\" : \"" << repoid << "\",\n"
                      << Indent(this) << "\"type\" :\n"
                      << Open(this)
                      << Indent(this) << "{\n"
                      << Open(this)
                      << Indent(this) << "\"kind\" : \"sequence\",\n"
                      << Indent(this) << "\"type\" : " << InlineType(arr->base_type()) << ",\n"
                      << Indent(this) << "\"size\" : [";
      ACE_CDR::ULong dims = arr->n_dims();
      for (size_t i = 0; i < dims; ++i) {
        if (i > 0)
          be_global->itl_ << ", ";
        be_global->itl_ << arr->dims()[i]->ev()->u.ulval;
      }
      be_global->itl_ << "]\n"
                      << Close(this)
                      << Indent(this) << "}\n"
                      << Close(this)
                      << Close(this)
                      << Indent(this) << "}\n"
                      << Close(this);
      break;
    }
  case AST_Decl::NT_fixed:
    {
      AST_Fixed* fixed = dynamic_cast<AST_Fixed*>(base);
      unsigned digits = fixed->digits()->ev()->u.ulval;
      unsigned scale = fixed->scale()->ev()->u.ulval;
      be_global->itl_
        << Open(this) << Indent(this) << "{\n" << Open(this)
        << Indent(this) << "\"kind\" : \"alias\",\n"
        << Indent(this) << "\"name\" : \"" << repoid << "\",\n"
        << Indent(this) << "\"type\" : { "
          << "\"kind\" : \"fixed\", "
          << "\"digits\" : " << digits << ", "
          << "\"scale\" : " << scale << ", "
          << "\"base\" : 10 }\n"
        << Close(this) << Indent(this) << "}\n" << Close(this);
      break;
    }
  default:
    {
      be_global->itl_ << Open(this)
                      << Indent(this) << "{\n"
                      << Open(this)
                      << Indent(this) << "\"kind\" : \"alias\",\n"
                      << Indent(this) << "\"name\" : \"" << repoid << "\",\n"
                      << Indent(this) << "\"type\" : " << InlineType(base) << "\n"
                      << Close(this)
                      << Indent(this) << "}\n"
                      << Close(this);

      return true;
    }
  }
  return true;
}

bool itl_generator::gen_struct(AST_Structure* node, UTL_ScopedName*,
                               const std::vector<AST_Field*>& fields,
                               AST_Type::SIZE_TYPE, const char* repoid)
{
  if (!be_global->itl())
    return true;

  const bool is_topic_type =
    idl_global->is_dcps_type(node->name()) || be_global->is_topic_type(node);

  new_type();

  be_global->itl_ << Open(this)
                  << Indent(this) << "{\n"
                  << Open(this)
                  << Indent(this) << "\"kind\" : \"alias\",\n"
                  << Indent(this) << "\"name\" : \"" << repoid << "\",\n"

  // Check if this is defined as a primary data type
                  << Indent(this) << "\"note\" : { \"is_dcps_data_type\" : "
                  << (is_topic_type ? "true" : "false")
                  << " },\n"

                  << Indent(this) << "\"type\" :\n"
                  << Open(this)
                  << Indent(this) << "{\n"
                  << Open(this)
                  << Indent(this) << "\"kind\" : \"record\",\n"
                  << Indent(this) << "\"fields\" :\n"
                  << Open(this)
                  << Indent(this) << "[\n";

  bool comma_flag = false;
  for (std::vector<AST_Field*>::const_iterator pos = fields.begin(), limit = fields.end();
       pos != limit;
       ++pos) {
    AST_Field* field = *pos;
    if (comma_flag) {
      be_global->itl_ << Indent(this) << ",\n";
    }
    be_global->itl_ << Open(this)
                    << Indent(this) << "{\n"
                    << Open(this)
                    << Indent(this) << "\"name\" : \"" << field->local_name()->get_string() << "\",\n"
                    << Indent(this) << "\"type\" : " << InlineType(field->field_type()) << "\n"
                    << Close(this)
                    << Indent(this) << "}\n"
                    << Close(this);
    comma_flag = true;
  }

  be_global->itl_ << Indent(this) << "]\n"
                  << Close(this)
                  << Close(this)
                  << Indent(this) << "}\n"
                  << Close(this)
                  << Close(this)
                  << Indent(this) << "}\n"
                  << Close(this);


  return true;
}


bool itl_generator::gen_union(AST_Union* node, UTL_ScopedName* /*name*/,
                              const std::vector<AST_UnionBranch*>& cases,
                              AST_Type* _d,
                              const char* repoid)
{
  new_type();

  be_global->itl_ << Open(this)
                  << Indent(this) << "{\n"
                  << Open(this)
                  << Indent(this) << "\"kind\" : \"alias\",\n"
                  << Indent(this) << "\"name\" : \"" << repoid << "\",\n"
                  << Indent(this) << "\"type\" :\n"
                  << Open(this)
                  << Indent(this) << "{\n"
                  << Open(this)
                  << Indent(this) << "\"kind\" : \"union\",\n"
                  << Indent(this) << "\"discriminator\" : " << InlineType(_d) << ",\n"
                  << Indent(this) << "\"note\" : { \"is_dcps_data_type\" : "
                  << (be_global->is_topic_type(node) ? "true" : "false")
                  << " },\n"
                  << Indent(this) << "\"fields\" :\n"
                  << Open(this)
                  << Indent(this) << "[\n";

  for (std::vector<AST_UnionBranch*>::const_iterator pos = cases.begin(), limit = cases.end();
       pos != limit;
       ++pos) {
    if (pos != cases.begin())
      be_global->itl_ << Indent(this) << ",\n";

    AST_UnionBranch *branch = *pos;

    be_global->itl_ << Open(this)
                    << Indent(this) << "{\n"
                    << Open(this)
                    << Indent(this) << "\"name\" : \"" << branch->local_name()->get_string() << "\",\n"
                    << Indent(this) << "\"type\" : " << InlineType(branch->field_type()) << ",\n"

                    << Indent(this) << "\"labels\" : [";


    unsigned long count = branch->label_list_length();
    for (unsigned long i = 0; i < count; i++)
      {
        if (i > 0)
          be_global->itl_ << ", ";
        AST_UnionLabel *label = branch->label(i);
        if (label->label_kind() == AST_UnionLabel::UL_default)
          {
            continue;
          }
        be_global->itl_ << "\"" << label->label_val()->n()->last_component()->get_string() << "\"";
      }

    be_global->itl_ << "]\n"
                    << Close(this)
                    << Indent(this) << "}\n"
                    << Close(this);
  }

  be_global->itl_ << Indent(this) << "]\n"
                  << Close(this)
                  << Close(this)
                  << Indent(this) << "}\n"
                  << Close(this)
                  << Close(this)
                  << Indent(this) << "}\n"
                  << Close(this);

  return true;
}
