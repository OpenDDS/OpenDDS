/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "idl_mapping.h"
#include "be_extern.h"

#include "utl_identifier.h"
#include "utl_string.h"

#include "ace/OS_NS_sys_stat.h"
#include "ace/OS_NS_string.h"
#include "ace/Version.h"

#include <iostream>
#include <sstream>
#include <fstream>
#include <algorithm>

using namespace std;

string idl_mapping_java::scoped(UTL_ScopedName *name)
{
  return scoped_helper(name, ".");
}

namespace {

enum JavaFileType {JCLASS, JINTERFACE, JABSTRACT_CLASS, JFINAL_CLASS};

struct JavaName {
  vector<string> pkg_;
  string clazz_;

  explicit JavaName(UTL_ScopedName *name) {
    for (UTL_ScopedName *sn(name); sn;
         sn = static_cast<UTL_ScopedName *>(sn->tail())) {
      string sname((sn->head()->escaped()) ? "_" : "");
      sname += sn->head()->get_string();

      if (!sname.empty()) {
        if (sn->tail()) pkg_.push_back(sname);

        else clazz_ = sname;
      }
    }
  }

  JavaName(const JavaName& base, const char *suffix, bool escape = false)
  : pkg_(base.pkg_)
  , clazz_(base.clazz_) {
    clazz_ += suffix;
    size_t zero = 0; //explicit type to avoid SunCC thinking this is a char*

    if (escape) clazz_.insert(zero, 1, '_');
  }
};

struct concat {
  concat(string &str, char sep) : str_(str), sep_(sep) {}
  void operator()(const string &sub) {
    str_ += sub + sep_;
  }
  string &str_;
  char sep_;
};

bool java_class_gen(const JavaName &jn, JavaFileType jft, const char *body,
                    const char *extends = "",
                    const char *implements = "")
{
  string file, pkgdecl;

  for_each(jn.pkg_.begin(), jn.pkg_.end(), concat(file, '/'));
  for_each(jn.pkg_.begin(), jn.pkg_.end(), concat(pkgdecl, '.'));

  //remove trailing dot
  if (jn.pkg_.size() > 0) pkgdecl.resize(pkgdecl.size() - 1);

  //create directories
  string path;

  for (size_t i = 0; i < jn.pkg_.size(); ++i) {
    path += jn.pkg_[i] + '/';

    if (0 != ACE_OS::mkdir(path.c_str()) && errno != EEXIST) {
      cerr << "ERROR - couldn't create directory " << path << "\n";
      return false;
    }
  }

  file += jn.clazz_;
  file += ".java";

  string classkeyw;

  switch (jft) {
  case JCLASS:
    classkeyw = "class";
    break;
  case JINTERFACE:
    classkeyw = "interface";
    break;
  case JABSTRACT_CLASS:
    classkeyw = "abstract class";
    break;
  case JFINAL_CLASS:
    classkeyw = "final class";
    break;
  }

  string nlib = be_global->native_lib_name().c_str();
  string nativeLoader;

  if (nlib != "" && ACE_OS::strstr(body, " native ")) {
    nativeLoader = "\n"
                   "  static {\n"
                   "    String propVal = System.getProperty(\"opendds.native.debug\");\n"
                   "    if (propVal != null && (\"1\".equalsIgnoreCase(propVal) ||\n"
                   "        \"y\".equalsIgnoreCase(propVal) ||\n"
                   "        \"yes\".equalsIgnoreCase(propVal) ||\n"
                   "        \"t\".equalsIgnoreCase(propVal) ||\n"
                   "        \"true\".equalsIgnoreCase(propVal)))\n"
                   "      System.loadLibrary(\"" + nlib + "d\");\n"
                   "    else System.loadLibrary(\"" + nlib +  "\");\n"
                   "  }\n\n";
  }

  ostringstream oss;

  if (jn.pkg_.size() > 0)
    oss <<
    "package " << pkgdecl << ";\n";

  oss <<
  "public " << classkeyw << ' ' << jn.clazz_;

  if (*extends) {
    oss << " extends " << extends;
  }

  if (*implements && jft != JINTERFACE) {
    oss << " implements " << implements;
  }

  oss << " {\n" <<
  body <<
  nativeLoader <<
  "}\n";

  return BE_GlobalData::writeFile(file.c_str(), oss.str());
}

bool gen_helper(const JavaName &jn, const char *repoid, bool narrow = false)
{
  JavaName jn_help(jn, "Helper");

  string body =
    "  // Any and TypeCode operations not currently implemented\n"
    "  public static String id() { return \"" + string(repoid) + "\"; }\n";

  if (narrow) {
    body +=
      "  public static " +  jn.clazz_
      + " narrow(org.omg.CORBA.Object obj) {\n"
      "    if (obj == null)\n"
      "      return null;\n"
      "    else if (obj instanceof " + jn.clazz_ + ")\n"
      "      return (" + jn.clazz_ + ")obj;\n"
      "    else if (!obj._is_a (id ()))\n"
      "      throw new org.omg.CORBA.BAD_PARAM ();\n"
      "    else\n"
      "      return native_unarrow(obj);\n"
      "  }\n\n"
      "  public static " +  jn.clazz_
      + " unchecked_narrow(org.omg.CORBA.Object obj) {\n"
      "    if (obj == null)\n"
      "      return null;\n"
      "    else if (obj instanceof " + jn.clazz_ + ")\n"
      "      return (" + jn.clazz_ + ")obj;\n"
      "    else\n"
      "      return native_unarrow(obj);\n"
      "  }\n\n"
      "  private native static " + jn.clazz_
      + " native_unarrow(org.omg.CORBA.Object obj);\n";
  }

  return java_class_gen(jn_help, JABSTRACT_CLASS, body.c_str());
}

bool gen_holder(const JavaName &jn, const char *held = "")
{
  JavaName jn_hold(jn, "Holder");
  string value_type = *held ? held : jn.clazz_;
  string body =
    "  // TypeCode operations not currently implemented\n"
    "  public " + value_type + " value;\n"
    "  public " + jn.clazz_ + "Holder() {}\n"
    "  public " + jn.clazz_ + "Holder(" + value_type + " initial) {\n"
    "    value = initial;\n"
    "  }\n";
  return java_class_gen(jn_hold, JFINAL_CLASS, body.c_str());
}

std::string param_type(AST_Type *t, AST_Argument::Direction dir)
{
  string type = idl_mapping_java::type(t);

  if (dir == AST_Argument::dir_IN) return type;

  bool capitalize = false, builtin = false;

  switch (t->node_type()) {
  case AST_Decl::NT_pre_defined: {
    AST_PredefinedType *p = AST_PredefinedType::narrow_from_decl(t);

    switch (p->pt()) {
    case AST_PredefinedType::PT_boolean:
    case AST_PredefinedType::PT_char:
    case AST_PredefinedType::PT_wchar:
    case AST_PredefinedType::PT_octet:
    case AST_PredefinedType::PT_short:
    case AST_PredefinedType::PT_ushort:
    case AST_PredefinedType::PT_long:
    case AST_PredefinedType::PT_ulong:
    case AST_PredefinedType::PT_longlong:
    case AST_PredefinedType::PT_ulonglong:
    case AST_PredefinedType::PT_float:
    case AST_PredefinedType::PT_double:
      builtin = true;
      capitalize = true;
    default:
      ;
    }

    break;
  }
  case AST_Decl::NT_string:
    builtin = true;
    break;
  case AST_Decl::NT_typedef: {
    AST_Typedef *td = AST_Typedef::narrow_from_decl(t);
    AST_Type *base = td->base_type();

    if (base->node_type() == AST_Decl::NT_array
        || base->node_type() == AST_Decl::NT_sequence)
      type = idl_mapping_java::scoped(td->name());

    break;
  }
  default:
    ;
  }

  if (capitalize) type[0] = toupper(type[0]);

  if (builtin) type = "org.omg.CORBA." + type;

  type += "Holder";
  return type;
}

} // namespace

std::string idl_mapping_java::type(AST_Type *decl)
{
  switch (decl->node_type()) {
  case AST_Decl::NT_pre_defined: {
    AST_PredefinedType *p = AST_PredefinedType::narrow_from_decl(decl);

    switch (p->pt()) {
    case AST_PredefinedType::PT_boolean:
      return "boolean";
    case AST_PredefinedType::PT_char:
    case AST_PredefinedType::PT_wchar:
      return "char";
    case AST_PredefinedType::PT_octet:
      return "byte";
    case AST_PredefinedType::PT_short:
    case AST_PredefinedType::PT_ushort:
      return "short";
    case AST_PredefinedType::PT_long:
    case AST_PredefinedType::PT_ulong:
      return "int";
    case AST_PredefinedType::PT_longlong:
    case AST_PredefinedType::PT_ulonglong:
      return "long";
    case AST_PredefinedType::PT_float:
      return "float";
    case AST_PredefinedType::PT_double:
      return "double";
    default:                                ;//fall through
    }
  }
  case AST_Decl::NT_string:
    return "String";
  case AST_Decl::NT_enum:
  case AST_Decl::NT_interface:
  case AST_Decl::NT_interface_fwd:
  case AST_Decl::NT_native:
  case AST_Decl::NT_union:
  case AST_Decl::NT_struct:
  case AST_Decl::NT_struct_fwd:
    return scoped(decl->name());
  case AST_Decl::NT_typedef: {
    AST_Typedef *td = AST_Typedef::narrow_from_decl(decl);
    return type(td->primitive_base_type());
  }
  case AST_Decl::NT_sequence: {
    AST_Sequence *seq = AST_Sequence::narrow_from_decl(decl);
    return type(seq->base_type()) + "[]";
  }
  case AST_Decl::NT_array: {
    AST_Array *arr = AST_Array::narrow_from_decl(decl);
    return type(arr->base_type()) + "[]";
  }
  default: ;//fall through
  }

  cerr << "ERROR - unknown Java type " << decl->node_type()
       << " for IDL type: "
       << decl->local_name()->get_string() << endl;
  return "**unknown**";
}

namespace { //"ju" helper functions: Java Unsigned type conversion

ACE_CDR::Short ju_s(ACE_CDR::UShort u)
{
  return (u > 0x7FFF) ? -static_cast<ACE_CDR::Short>(~u+1) : u;
}

ACE_CDR::Long ju_l(ACE_CDR::ULong u)
{
  return (u > 0x7FFFFFFFUL) ? -static_cast<ACE_CDR::Long>(~u+1) : u;
}

ACE_CDR::LongLong ju_ll(ACE_CDR::ULongLong u)
{
  return (u > 0x7FFFFFFFFFFFFFFFULL)
         ? -static_cast<ACE_CDR::LongLong>(~u+1) : u;
}

ostream &operator<< (ostream &o, AST_Expression::AST_ExprValue *ev)
{
  switch (ev->et) {
  case AST_Expression::EV_short:
    o << ev->u.sval;
    break;
  case AST_Expression::EV_ushort:
    o << ju_s(ev->u.usval);
    break;
  case AST_Expression::EV_long:
    o << ev->u.lval;
    break;
  case AST_Expression::EV_ulong:
    o << ju_l(ev->u.ulval);
    break;
  case AST_Expression::EV_longlong:
    o << ev->u.llval << 'L';
    break;
  case AST_Expression::EV_ulonglong:
    o << ju_ll(ev->u.ullval) << 'L';
    break;
  case AST_Expression::EV_float:
    o << ev->u.fval << 'F';
    break;
  case AST_Expression::EV_double:
    o << ev->u.dval;
    break;
  case AST_Expression::EV_char:
    o << '\'' << ev->u.cval << '\'';
    break;
  case AST_Expression::EV_wchar:
    o << ev->u.wcval;
    break;
  case AST_Expression::EV_octet:
    o << static_cast<int>(ev->u.oval);
    break;
  case AST_Expression::EV_bool:
    o << boolalpha << static_cast<bool>(ev->u.bval);
    break;
  case AST_Expression::EV_string:
    o << '"' << ev->u.strval->get_string() << '"';
    break;
  case AST_Expression::EV_wstring:
    o << '"' << ev->u.wstrval << '"';
    break;
  case AST_Expression::EV_enum:
    o << ev->u.eval;
    break;
  case AST_Expression::EV_longdouble:
  case AST_Expression::EV_any:
  case AST_Expression::EV_object:
  case AST_Expression::EV_void:
  case AST_Expression::EV_none:
  default: {
    cerr << "ERROR - Constant of type " << ev->et
         << " is not supported\n";
  }
  }

  return o;
}

bool isEnum(AST_Type *t)
{
  if (t->node_type() == AST_Decl::NT_typedef) {
    AST_Typedef *td = AST_Typedef::narrow_from_decl(t);
    t = td->primitive_base_type();
  }

  return t->node_type() == AST_Decl::NT_enum;
}

} // namespace

bool idl_mapping_java::gen_const(UTL_ScopedName *name, bool nestedInInteface,
                                 AST_Expression::ExprType type, AST_Expression::AST_ExprValue *ev)
{
  //Constants within interfaces have an alternate mapping, which we are not
  //going to implement at this time.
  if (nestedInInteface) {
    cerr << "ERROR - Constants within Interfaces are not supported\n";
    return false;
  }

  string type_str;

  switch (type) {
  case AST_Expression::EV_short:
  case AST_Expression::EV_ushort:
    type_str = "short";
    break;
  case AST_Expression::EV_long:
  case AST_Expression::EV_ulong:
    type_str = "int";
    break;
  case AST_Expression::EV_longlong:
  case AST_Expression::EV_ulonglong:
    type_str = "long";
    break;
  case AST_Expression::EV_float:
    type_str = "float";
    break;
  case AST_Expression::EV_double:
    type_str = "double";
    break;
  case AST_Expression::EV_char:
  case AST_Expression::EV_wchar:
    type_str = "char";
    break;
  case AST_Expression::EV_octet:
    type_str = "byte";
    break;
  case AST_Expression::EV_bool:
    type_str = "boolean";
    break;
  case AST_Expression::EV_string:
  case AST_Expression::EV_wstring:
    type_str = "String";
    break;
  case AST_Expression::EV_longdouble:
  case AST_Expression::EV_enum:
  case AST_Expression::EV_any:
  case AST_Expression::EV_object:
  case AST_Expression::EV_void:
  case AST_Expression::EV_none:
  default: {
    cerr << "ERROR - Constant of type " << type
         << " is not supported\n";
    return false;
  }
  }

  ostringstream oss;
  oss << "  " << type_str << " value = (" << type_str << ") (" << ev << ");\n";

  return java_class_gen(JavaName(name), JINTERFACE,
                        oss.str().c_str());
}

bool idl_mapping_java::gen_enum(UTL_ScopedName *name,
                                const std::vector<AST_EnumVal *> &contents, const char *repoid)
{
  ostringstream oss;
  const char *enum_name = name->last_component()->get_string();
  oss <<
  "  private static " << enum_name << "[] __values = {\n";

  for (size_t i = 0; i < contents.size(); ++i) {
    oss <<
    "    new " << enum_name << '(' << i << ')';

    if (i < contents.size() - 1) oss << ',';

    oss << '\n';
  }

  oss
  << "  };\n\n";

  for (size_t i = 0; i < contents.size(); ++i) {
    const char *enumerator_name = contents[i]->local_name()->get_string();
    oss <<
    "  public static final int _" << enumerator_name
    << " = " << i << ";\n"
    "  public static final " << enum_name << ' '
    << enumerator_name << " = __values[" << i << "];\n\n";
  }

  oss <<
  "  public int value() { return _value; }\n\n"
  "  private int _value;\n\n"
  "  public static " << enum_name << " from_int(int value) {\n"
  "    if (value >= 0 && value < " << contents.size() << ") {\n"
  "      return __values[value];\n"
  "    } else {\n"
  "      return new " << enum_name << "(value);\n"
  "    }\n"
  "  }\n\n"
  "  protected " << enum_name << "(int value) { _value = value; }\n\n"
  "  public Object readResolve()\n"
  "      throws java.io.ObjectStreamException {\n"
  "    return from_int(value());\n"
  "  }\n\n";

  JavaName jn(name);
  return java_class_gen(jn, JCLASS,
                        oss.str().c_str(), "", "java.io.Serializable")
         && gen_helper(jn, repoid) && gen_holder(jn);
}

bool idl_mapping_java::gen_struct(UTL_ScopedName *name,
                                  const std::vector<AST_Field *> &fields, const char *repoid)
{
  string fieldDecl, ctorArgs, ctorBody;

  for (size_t i = 0; i < fields.size(); ++i) {
    string fname = fields[i]->local_name()->get_string();
    string ftype = type(fields[i]->field_type());
    fieldDecl += "  public " + ftype + " " + fname + ";\n";
    ctorArgs += ftype + " _" + fname;

    if (i != fields.size() - 1) ctorArgs += ", ";

    ctorBody += "    " + fname + " = _" + fname + ";\n";
  }

  const char *struct_name = name->last_component()->get_string();

  string body =
    fieldDecl + "\n"
    "  public " + struct_name + "() {}\n\n"
    "  public " + struct_name + "(" + ctorArgs + ") {\n" +
    ctorBody +
    "  }\n";

  JavaName jn(name);

  return java_class_gen(jn, JFINAL_CLASS,
                        body.c_str(), "", "java.io.Serializable")
         && gen_helper(jn, repoid) && gen_holder(jn);
}

bool idl_mapping_java::gen_typedef(UTL_ScopedName *name, AST_Type *base,
                                   const char *repoid)
{
  JavaName jn(name);
  bool ok = gen_helper(jn, repoid);

  if (base->node_type() == AST_Decl::NT_array
      || base->node_type() == AST_Decl::NT_sequence) {
    ok &= gen_holder(jn, type(base).c_str());
  }

  return ok;
}

namespace {
string attr_signature_r(AST_Attribute *attr)
{
  return idl_mapping_java::type(attr->field_type())
         + ' ' + attr->local_name()->get_string() + "()";
}

string attr_signature_w(AST_Attribute *attr)
{
  const char *name = attr->local_name()->get_string();
  return string("void ") + name + '('
         + idl_mapping_java::type(attr->field_type()) + ' ' + name + ')';
}

string op_signature(AST_Operation *op)
{
  string signature =
    (op->void_return_type() ? "void"
  : idl_mapping_java::type(op->return_type()))
    + " " + op->local_name()->get_string() + "(";
  UTL_ScopeActiveIterator it(op, UTL_Scope::IK_decls);
  bool noArgs = true;

  for (; !it.is_done(); it.next()) {
    noArgs = false;
    AST_Decl *item = it.item();

    if (item->node_type() == AST_Decl::NT_argument) {
      AST_Argument *arg = AST_Argument::narrow_from_decl(item);
      signature += param_type(arg->field_type(), arg->direction())
                   + " " + arg->local_name()->get_string() + ", ";
    }
  }

  if (noArgs) signature += ")";

  else signature.replace(signature.size() - 2, 2, ")");

  return signature;
}
}

bool idl_mapping_java::gen_interf(UTL_ScopedName *name, bool local,
                                  const std::vector<AST_Interface *> &inherits,
                                  const std::vector<AST_Interface *> &inherits_flat,
                                  const std::vector<AST_Attribute *> &attrs,
                                  const std::vector<AST_Operation *> &ops, const char *repoid)
{
  JavaName jn(name);
  JavaName jn_ops(jn, "Operations");
  JavaName jn_stub(jn, local ? "TAOPeer" : "Stub", true);

  string extends = jn.clazz_ + "Operations, "
                   + (inherits.size() ? "" : "org.omg.CORBA.Object");
  string extends_ops;

  for (size_t i = 0; i < inherits.size(); ++i) {
    extends += type(inherits[i]);
    extends_ops += type(inherits[i]) + "Operations";

    if (i != inherits.size() - 1) {
      extends += ", ";
      extends_ops += ", ";
    }
  }

  string extends_stub = string("i2jrt.TAO") +
                        (local ? "Local" : "") + "Object";

  string allRepoIds = '"' + string(repoid) + '"',
                      body_ops, body_stub =
                        "  protected " + jn_stub.clazz_ + "(long ptr) {\n"
                        "    super(ptr);\n"
                        "  }\n\n";

  for (size_t i = 0; i < attrs.size(); ++i) {
    AST_Attribute *attr = attrs[i];

    string signature = attr_signature_r(attr);
    body_ops +=
      "  " + signature + ";\n";
    body_stub +=
      "  public native " + signature + ";\n\n";

    if (!attr->readonly()) {
      string signature = attr_signature_w(attr);
      body_ops +=
        "  " + signature + ";\n";
      body_stub +=
        "  public native " + signature + ";\n\n";
    }
  }

  for (size_t i = 0; i < ops.size(); ++i) {
    string signature = op_signature(ops[i]);
    body_ops +=
      "  " + signature + ";\n";
    body_stub +=
      "  public native " + signature + ";\n\n";
  }

  for (size_t i = 0; i < inherits_flat.size(); ++i) {
    AST_Interface *base = inherits_flat[i];
    allRepoIds += ", \"" + string(base->repoID()) + '"';

    UTL_ScopeActiveIterator it(base, UTL_Scope::IK_decls);

    for (; !it.is_done(); it.next()) {
      AST_Decl *item = it.item();

      if (item->node_type() == AST_Decl::NT_attr) {
        AST_Attribute *attr = AST_Attribute::narrow_from_decl(item);

        string signature = attr_signature_r(attr);
        body_stub +=
          "  public native " + signature + ";\n\n";

        if (!attr->readonly()) {
          string signature = attr_signature_w(attr);
          body_stub +=
            "  public native " + signature + ";\n\n";
        }

      } else if (item->node_type() == AST_Decl::NT_op) {
        AST_Operation *op = AST_Operation::narrow_from_decl(item);
        body_stub +=
          "  public native " + op_signature(op) + ";\n\n";
      }
    }
  }

  bool ok(true);

  if (local) {
    JavaName jn_lb(jn, "LocalBase", true);
    string lb =
      "  private String[] _type_ids = {" + allRepoIds + "};\n\n"
      "  public String[] _ids() { return (String[])_type_ids.clone(); }\n";
    ok = java_class_gen(jn_lb, JABSTRACT_CLASS, lb.c_str(),
                        "org.omg.CORBA.LocalObject", jn.clazz_.c_str());
  }

  return ok && java_class_gen(jn, JINTERFACE, "", extends.c_str())
         && java_class_gen(jn_ops, JINTERFACE, body_ops.c_str(),
                           extends_ops.c_str())
         && java_class_gen(jn_stub, JCLASS, body_stub.c_str(),
                           extends_stub.c_str(), jn.clazz_.c_str())
         && gen_helper(jn, repoid, true) && gen_holder(jn);

  //FUTURE: server side
}

bool idl_mapping_java::gen_native(UTL_ScopedName *name, const char *repoid)
{
  JavaName jn(name);

  if (idl_global->dcps_support_zero_copy_read()
      && jn.clazz_.substr(jn.clazz_.size() - 3) == "Seq") {
    string held = jn.clazz_.substr(0, jn.clazz_.size() - 3) + "[]";
    return gen_helper(jn, repoid) && gen_holder(jn, held.c_str());
  }

  return true;
}

namespace {
void writeUnionDefaultValue(ostream &os, AST_Expression::ExprType udisc_type,
                            const AST_Union::DefaultValue &dv)
{
  switch (udisc_type) {
  case AST_Expression::EV_short:
    os << dv.u.short_val;
    break;
  case AST_Expression::EV_ushort:
    os << dv.u.ushort_val;
    break;
  case AST_Expression::EV_long:
    os << dv.u.long_val;
    break;
  case AST_Expression::EV_ulong:
    os << dv.u.ulong_val;
    break;
  case AST_Expression::EV_char:
    os << dv.u.char_val;
    break;
  case AST_Expression::EV_wchar:
    os << dv.u.wchar_val;
    break;
  case AST_Expression::EV_bool:
    os << boolalpha << static_cast<bool>(dv.u.bool_val);
    break;
  case AST_Expression::EV_enum:
    os << dv.u.enum_val;
    break;
  case AST_Expression::EV_longlong:
    os << dv.u.longlong_val;
    break;
  case AST_Expression::EV_ulonglong:
    os << dv.u.ulonglong_val;
    break;
  default:
    cerr << "ERROR: Bad discriminant type (shouldn't happen here)\n";
  }
}
}

bool idl_mapping_java::gen_union(UTL_ScopedName *name,
                                 const std::vector<AST_UnionBranch *> &branches, AST_Type *discriminator,
                                 AST_Expression::ExprType udisc_type,
                                 const AST_Union::DefaultValue &default_value, const char *repoid)
{
  string body_branches,
  disc_ty = type(discriminator),
            disc_val = isEnum(discriminator) ? ".value()" : "",
                       pre_disc_set = isEnum(discriminator) ? (disc_ty + ".from_int(") : "",
                                      post_disc_set = isEnum(discriminator) ? ")" : "",
                                                      default_disc_check;
  bool hasDefault(false);

  for (size_t i = 0; i < branches.size(); ++i) {
    string disc_check, first_label_value;
    unsigned long n_labels = branches[i]->label_list_length();

    for (unsigned long j = 0; j < n_labels; ++j) {
      AST_UnionLabel *ul = branches[i]->label(j);
      ostringstream oss;

      if (ul->label_kind() == AST_UnionLabel::UL_default) {
        hasDefault = true;
        writeUnionDefaultValue(oss, udisc_type, default_value);
        //disc_check is an expression to make sure the discriminant isn't
        //one of the other branch labels, but we don't know all the other
        //values yet, so this special string will be replaced later
        disc_check = "<%default_disc_check%>";

      } else {
        oss << ul->label_val()->ev();
        disc_check += "_discriminator" + disc_val + " != " + oss.str()
                      + ((j == n_labels - 1) ? "" : " && ");
        default_disc_check += (default_disc_check.size() ? " || " : "")
                              + string("_discriminator") + disc_val + " == " + oss.str();
      }

      if (j == 0) first_label_value = oss.str();
    }

    string ty = type(branches[i]->field_type());
    const char *brname = branches[i]->local_name()->get_string();
    body_branches +=
      "  private " + ty + ' ' + brname + ";\n\n"
      "  public " + ty + ' ' + brname + "() {\n"
      "    if (" + disc_check + ") throw new org.omg.CORBA.BAD_OPERATION();\n"
      "    return " + brname + ";\n"
      "  }\n\n"
      "  public void " + brname + '(' + ty + " _value) {\n"
      "    _discriminator = " + pre_disc_set + first_label_value
      + post_disc_set + ";\n"
      "    " + brname + " = _value;\n"
      "  }\n\n";

    if (n_labels > 1 || default_disc_check.size() > 0) {
      body_branches +=
        "  public void " + string(brname) + '(' + disc_ty
        + " _discriminator, " + ty + " _value) {\n"
        "    if (" + disc_check + ") throw new "
        "org.omg.CORBA.BAD_PARAM(34, "
        "org.omg.CORBA.CompletionStatus.COMPLETED_MAYBE);\n"
        "    this._discriminator = _discriminator;\n"
        "    this." + brname + " = _value;\n"
        "  }\n\n";
    }
  }

  size_t idx = body_branches.find("<%default_disc_check%>");
  const size_t n_chars = sizeof("<%default_disc_check%>") - 1 /*no null*/;

  while (idx != string::npos) {
    body_branches.replace(idx, n_chars, default_disc_check);
    idx = body_branches.find("<%default_disc_check%>");
  }

  const char *union_name = name->last_component()->get_string();

  string body =
    "  public " + string(union_name) + "() {}\n\n"
    "  private " + disc_ty + " _discriminator;\n\n"
    "  public " + disc_ty + " discriminator() {\n"
    "    return _discriminator;\n"
    "  }\n\n" +
    body_branches;

  if (!hasDefault
      && default_value.computed_ != 0) { //all possible values aren't covered
    ostringstream dv;
    writeUnionDefaultValue(dv, udisc_type, default_value);

    body +=
      "  public void __default() {\n"
      "    _discriminator = " + pre_disc_set + dv.str() + post_disc_set + ";\n"
      "  }\n\n"
      "  public void __default(" + disc_ty + " discriminator) {\n"
      "    _discriminator = discriminator;\n"
      "  }\n\n";
  }

  JavaName jn(name);
  return java_class_gen(jn, JFINAL_CLASS,
                        body.c_str(), "", "java.io.Serializable")
         && gen_helper(jn, repoid) && gen_holder(jn);
}
