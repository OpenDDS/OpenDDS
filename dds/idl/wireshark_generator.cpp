/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "wireshark_generator.h"
#include "be_extern.h"

#include "utl_identifier.h"
#include "utl_labellist.h"

using namespace AstTypeClassification;

void wireshark_generator::write_common (UTL_ScopedName *name,
                                        const char *kind,
                                        const char *repoid)
{
  const char *ident = name->last_component()->get_string();
  be_global->ws_config_ << "\n[" << scoped_helper(name, "/") << "]\n";
  be_global->ws_config_ << ident << ".kind = \"" << kind << "\"\n";
  be_global->ws_config_ << ident << ".repoid = \"" << repoid << "\"\n";
}

bool wireshark_generator::gen_enum(AST_Enum*, UTL_ScopedName* name,
  const std::vector<AST_EnumVal*>& contents, const char* repoid)
{
  if (!be_global->generate_wireshark())
    return true;

  write_common (name, "enum", repoid);
  be_global->ws_config_ << name->last_component()->get_string()
                        << ".order = \"";

  for (size_t i = 0; i < contents.size(); ++i) {
    be_global->ws_config_ << contents[i]->local_name()->get_string();
    if ((i+1) < contents.size())
      be_global->ws_config_<< " ";
  }
  be_global->ws_config_ << "\"\n";
  return true;
}

void wireshark_generator::gen_array(UTL_ScopedName* name, AST_Array* arr)
{
  std::string elem = scoped(arr->base_type()->name());
  const char *ident = name->last_component()->get_string();

  be_global->ws_config_ << ident << ".element = \"" << elem << "\"\n";
  ACE_CDR::ULong dims = arr->n_dims();
  if (dims > 1)
    {
      be_global->ws_config_ << ident << ".dimensions = " << dims << "\n";
      be_global->ws_config_ << ident << ".sizes = \"";
    }
  else
    {
      be_global->ws_config_ << ident << ".size = ";
    }

  for (size_t i = 0; i < dims; ++i) {
    be_global->ws_config_ << arr->dims()[i]->ev()->u.ulval;
    if ((i+1) < dims)
      be_global->ws_config_ << " ";
  }

  if (dims > 1)
    {
      be_global->ws_config_ << "\"";
    }
  be_global->ws_config_ << "\n";

}


bool wireshark_generator::gen_typedef(AST_Typedef*, UTL_ScopedName* name, AST_Type* base,
                                      const char* repoid)
{
  if (!be_global->generate_wireshark())
    return true;
  switch (base->node_type()) {
  case AST_Decl::NT_sequence:
    {
      AST_Sequence *seq = AST_Sequence::narrow_from_decl(base);
      write_common (name, "sequence", repoid);
      be_global->ws_config_ << name->last_component()->get_string()
                            << ".element = \""
                            << scoped(seq->base_type()->name())
                            << "\"\n";
      break;
    }
  case AST_Decl::NT_array:
    {
      write_common (name, "array", repoid);
      gen_array(name, AST_Array::narrow_from_decl(base));
      break;
    }
  default:
    {
      write_common (name, "alias", repoid);
      be_global->ws_config_ << name->last_component()->get_string()
                            << ".base = \""
                            << scoped(base->name())
                            << "\"\n";

      return true;
    }
  }
  return true;
}

namespace {
  static std::stringstream orderstrm;

  void write_field (AST_Field *field)
  {
    orderstrm << field->local_name()->get_string() << " ";
    be_global->ws_config_ << field->local_name()->get_string() << " = \"";
    be_global->ws_config_ << scoped(field->field_type()->name()) << "\"\n";
  }

  void write_branch (AST_UnionBranch *branch)
  {
    const char *label_name = 0;
    unsigned long count = branch->label_list_length();
    for (unsigned long i = 0; i < count; i++)
      {
        AST_UnionLabel *label = branch->label(i);
        if (label->label_kind() == AST_UnionLabel::UL_default)
          {
            label_name = "default";
            break;
          }
        label_name = label->label_val()->n()->last_component()->get_string();
        orderstrm << label_name << " ";
      }

    be_global->ws_config_ << label_name << ".type = \""
                          << scoped(branch->field_type()->name()) << "\"\n";
    be_global->ws_config_ << label_name << ".name = \""
                          << branch->local_name()->get_string() << "\"\n";
  }

}


bool wireshark_generator::gen_struct(AST_Structure*, UTL_ScopedName* name,
                                     const std::vector<AST_Field*>& fields,
                                     AST_Type::SIZE_TYPE, const char* repoid)
{
  if (!be_global->generate_wireshark())
    return true;

  write_common (name, "struct", repoid);

  std::for_each (fields.begin(), fields.end(), write_field);

  be_global->ws_config_ << name->last_component()->get_string()
                        << ".order = \""
                        << orderstrm.str() << "\"\n";
  orderstrm.str("");
  orderstrm.clear();

  return true;
}


bool wireshark_generator::gen_union(AST_Union*, UTL_ScopedName* name,
                                    const std::vector<AST_UnionBranch*>& cases,
                                    AST_Type* _d,
                                    const char* repoid)
{
  if (!be_global->generate_wireshark())
    return true;

  write_common (name, "union", repoid);
  const char *ident = name->last_component()->get_string();

  be_global->ws_config_ << ident << ".discriminator = \""
                        << scoped(_d->name()) << "\"\n";

  std::for_each (cases.begin(), cases.end(), write_branch);

  be_global->ws_config_ << ident << ".order = \""
                        << orderstrm.str() << "\"\n";
  orderstrm.str("");
  orderstrm.clear();

  return true;
}
