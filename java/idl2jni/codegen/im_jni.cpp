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
#include "fe_private.h"

#include <iostream>
#include <fstream>
#include <sstream>
#include <set>

using namespace std;

string idl_mapping_jni::scoped(UTL_ScopedName *name)
{
  return scoped_helper(name, "::");
}

string idl_mapping_jni::taoType(AST_Type *decl)
{
  switch (decl->node_type()) {
  case AST_Decl::NT_pre_defined: {
    AST_PredefinedType *p = AST_PredefinedType::narrow_from_decl(decl);

    switch (p->pt()) {
    case AST_PredefinedType::PT_boolean:
      return "CORBA::Boolean";
    case AST_PredefinedType::PT_char:
      return "CORBA::Char";
    case AST_PredefinedType::PT_wchar:
      return "CORBA::WChar";
    case AST_PredefinedType::PT_octet:
      return "CORBA::Octet";
    case AST_PredefinedType::PT_short:
      return "CORBA::Short";
    case AST_PredefinedType::PT_ushort:
      return "CORBA::UShort";
    case AST_PredefinedType::PT_long:
      return "CORBA::Long";
    case AST_PredefinedType::PT_ulong:
      return "CORBA::ULong";
    case AST_PredefinedType::PT_longlong:
      return "CORBA::LongLong";
    case AST_PredefinedType::PT_ulonglong:
      return "CORBA::ULongLong";
    case AST_PredefinedType::PT_float:
      return "CORBA::Float";
    case AST_PredefinedType::PT_double:
      return "CORBA::Double";
    default:
      ;
    }
  }
  case AST_Decl::NT_string:
    return "CORBA::String_var";
  case AST_Decl::NT_enum:
    return scoped(decl->name());
  case AST_Decl::NT_interface:
  case AST_Decl::NT_interface_fwd:
    return scoped(decl->name()) + "_var";
  default:
    ;
  }

  return scoped(decl->name());
}

string idl_mapping_jni::taoParam(AST_Type *decl, AST_Argument::Direction dir,
                                 bool return_type)
{
  string param = taoType(decl), name = scoped(decl->name());
  bool addRef = !return_type && ((dir == AST_Argument::dir_INOUT)
                                 || (dir == AST_Argument::dir_OUT)),
                addConst = !return_type && dir == AST_Argument::dir_IN,
                           variableOut = dir == AST_Argument::dir_OUT
                                         && decl->size_type() == AST_Type::VARIABLE;
  AST_Decl::NodeType effectiveType = decl->node_type();

  if (effectiveType == AST_Decl::NT_typedef) {
    AST_Typedef *td = AST_Typedef::narrow_from_decl(decl);
    effectiveType = td->primitive_base_type()->node_type();
  }

  switch (effectiveType) {
  case AST_Decl::NT_interface:
  case AST_Decl::NT_interface_fwd:

    if (dir == AST_Argument::dir_OUT) {
      addRef = false;
      param = name + "_out";

    } else
      param = name + "_ptr";

    //fall-through
  case AST_Decl::NT_pre_defined:
    addConst = false;
    break;
  case AST_Decl::NT_string:
    param = "char *";
    break;
  case AST_Decl::NT_wstring:
    param = "CORBA::WChar *";
    break;
  case AST_Decl::NT_sequence:

    if (return_type) {
      param += '*';

    } else if (dir == AST_Argument::dir_OUT) {
      param += "_out";
      addRef = false;

    } else addRef = true;

    break;
  case AST_Decl::NT_union:
  case AST_Decl::NT_struct:
    addRef = !return_type;

    if (decl->size_type() == AST_Type::VARIABLE) {
      if (return_type) param += '*';

      else if (dir == AST_Argument::dir_OUT) {
        addRef = false;
        param += "_out";
      }
    }

    break;
  case AST_Decl::NT_array:

    if (return_type || variableOut) param += "_slice*";

    addRef = variableOut;
    break;
  default:
    ;
  }

  if (addRef) param += '&';

  if (addConst) param = "const " + param;

  return param;
}

string idl_mapping_jni::type(AST_Type *decl)
{
  switch (decl->node_type()) {
  case AST_Decl::NT_pre_defined: {
    AST_PredefinedType *p = AST_PredefinedType::narrow_from_decl(decl);

    switch (p->pt()) {
    case AST_PredefinedType::PT_boolean:
      return "jboolean";
    case AST_PredefinedType::PT_char:
    case AST_PredefinedType::PT_wchar:
      return "jchar";
    case AST_PredefinedType::PT_octet:
      return "jbyte";
    case AST_PredefinedType::PT_short:
    case AST_PredefinedType::PT_ushort:
      return "jshort";
    case AST_PredefinedType::PT_long:
    case AST_PredefinedType::PT_ulong:
      return "jint";
    case AST_PredefinedType::PT_longlong:
    case AST_PredefinedType::PT_ulonglong:
      return "jlong";
    case AST_PredefinedType::PT_float:
      return "jfloat";
    case AST_PredefinedType::PT_double:
      return "jdouble";
    default:
      return "jobject";
    }
  }
  case AST_Decl::NT_string:
  case AST_Decl::NT_enum:
  case AST_Decl::NT_struct:
    return "jobject";
  case AST_Decl::NT_typedef: {
    AST_Typedef *td = AST_Typedef::narrow_from_decl(decl);
    return type(td->primitive_base_type());
  }
  case AST_Decl::NT_sequence: {
    AST_Sequence *seq = AST_Sequence::narrow_from_decl(decl);
    string base = type(seq->base_type());

    if (base.find("Array") == string::npos)
      return base + "Array";

    else return "jobjectArray";
  }
  case AST_Decl::NT_array: {
    AST_Array *arr = AST_Array::narrow_from_decl(decl);
    string base = type(arr->base_type());

    if (base.find("Array") == string::npos)
      return base + "Array";

    else return "jobjectArray";
  }
  case AST_Decl::NT_native:
    return "jobjectArray";
  default:
    return "jobject";
  }
}

namespace {
enum native_type {
  NATIVE_UNKNOWN,
  NATIVE_DATA_SEQUENCE,
  NATIVE_INFO_SEQUENCE
};

native_type get_native_type(UTL_ScopedName *name, string &elem)
{
  string seq(idl_mapping_jni::scoped_helper(name, "/"));

  if (idl_global->dcps_support_zero_copy_read() && seq.size() > 3
      && seq.substr(seq.size() - 3) == "Seq") {
    elem = seq.substr(0, seq.size() - 3);

    if (seq == "DDS/SampleInfoSeq") return NATIVE_INFO_SEQUENCE;

    return NATIVE_DATA_SEQUENCE;
  }

  return NATIVE_UNKNOWN;
}
}

string idl_mapping_jni::jvmSignature(AST_Type *decl)
{
  switch (decl->node_type()) {
  case AST_Decl::NT_pre_defined: {
    AST_PredefinedType *p = AST_PredefinedType::narrow_from_decl(decl);

    switch (p->pt()) {
    case AST_PredefinedType::PT_boolean:
      return "Z";
    case AST_PredefinedType::PT_char:
    case AST_PredefinedType::PT_wchar:
      return "C";
    case AST_PredefinedType::PT_octet:
      return "B";
    case AST_PredefinedType::PT_short:
    case AST_PredefinedType::PT_ushort:
      return "S";
    case AST_PredefinedType::PT_long:
    case AST_PredefinedType::PT_ulong:
      return "I";
    case AST_PredefinedType::PT_longlong:
    case AST_PredefinedType::PT_ulonglong:
      return "J";
    case AST_PredefinedType::PT_float:
      return "F";
    case AST_PredefinedType::PT_double:
      return "D";
    default:                                ;//fall through
    }
  }
  case AST_Decl::NT_string:
    return "Ljava/lang/String;";
  case AST_Decl::NT_enum:
  case AST_Decl::NT_struct:
  case AST_Decl::NT_struct_fwd:
  case AST_Decl::NT_union:
  case AST_Decl::NT_interface:
  case AST_Decl::NT_interface_fwd:
    return "L" + scoped_helper(decl->name(), "/") + ";";
  case AST_Decl::NT_typedef: {
    AST_Typedef *td = AST_Typedef::narrow_from_decl(decl);
    return jvmSignature(td->primitive_base_type());
  }
  case AST_Decl::NT_sequence: {
    AST_Sequence *seq = AST_Sequence::narrow_from_decl(decl);
    return "[" + jvmSignature(seq->base_type());
  }
  case AST_Decl::NT_array: {
    AST_Array *arr = AST_Array::narrow_from_decl(decl);
    return "[" + jvmSignature(arr->base_type());
  }
  case AST_Decl::NT_native: {
    string elem;

    if (get_native_type(decl->name(), elem) != NATIVE_UNKNOWN) {
      return "[L" + elem + ";";
    }
  }
  default: ;//fall through
  }

  cerr << "ERROR - unknown jvmSignature for IDL type: " <<
       decl->local_name()->get_string() << endl;
  return "**unknown**";
}

string idl_mapping_jni::jniFnName(AST_Type *decl)
{
  switch (decl->node_type()) {
  case AST_Decl::NT_pre_defined: {
    AST_PredefinedType *p = AST_PredefinedType::narrow_from_decl(decl);

    switch (p->pt()) {
    case AST_PredefinedType::PT_boolean:
      return "Boolean";
    case AST_PredefinedType::PT_char:
    case AST_PredefinedType::PT_wchar:
      return "Char";
    case AST_PredefinedType::PT_octet:
      return "Byte";
    case AST_PredefinedType::PT_short:
    case AST_PredefinedType::PT_ushort:
      return "Short";
    case AST_PredefinedType::PT_long:
    case AST_PredefinedType::PT_ulong:
      return "Int";
    case AST_PredefinedType::PT_longlong:
    case AST_PredefinedType::PT_ulonglong:
      return "Long";
    case AST_PredefinedType::PT_float:
      return "Float";
    case AST_PredefinedType::PT_double:
      return "Double";
    default:
      return "Object";
    }
  }
  case AST_Decl::NT_string:
  case AST_Decl::NT_enum:
  case AST_Decl::NT_struct:
    return "Object";
  case AST_Decl::NT_typedef: {
    AST_Typedef *td = AST_Typedef::narrow_from_decl(decl);
    return jniFnName(td->primitive_base_type());
  }
  default:
    return "Object";
  }
}

namespace {
struct commonSetup {
  ostream &hfile;
  ostream &cppfile;
  string sigToCxx, sigToJava, cxx, exporter;

  explicit commonSetup(UTL_ScopedName *name, const char *java = "jobject",
                       bool useVar = false, bool skipDefault = false, bool useCxxRef = true)
  : hfile(be_global->stub_header_)
  , cppfile(be_global->stub_impl_)
  , cxx(idl_mapping_jni::scoped(name) + (useVar ? "_var" : "")) {
    be_global->add_include("idl2jni_jni.h");
    be_global->add_include("idl2jni_runtime.h");
    ACE_CString ace_exporter = be_global->stub_export_macro();
    bool use_exp = ace_exporter != "";
    exporter = use_exp ? (string(" ") + ace_exporter.c_str()) : "";
    sigToCxx =
      "void copyToCxx (JNIEnv *jni, " + cxx + (useCxxRef ? " &" : " ")
      + "target, " + java + " source)";
    sigToJava =
      "void copyToJava (JNIEnv *jni, " + string(java) + " &target, const "
      + cxx + (useCxxRef ? " &" : " ") + "source, bool createNewObject";
    hfile <<
    ace_exporter << (use_exp ? "\n" : "") << sigToCxx << ";\n" <<
    ace_exporter << (use_exp ? "\n" : "") << sigToJava <<
    (skipDefault ? "" : " = false") << ");\n";
    sigToJava += ')';
  }
};

bool isPrimitive(AST_Type *element)
{
  if (element->node_type() == AST_Decl::NT_typedef) {
    AST_Typedef *td = AST_Typedef::narrow_from_decl(element);
    element = td->primitive_base_type();
  }

  if (element->node_type() == AST_Decl::NT_pre_defined) {
    AST_PredefinedType *p = AST_PredefinedType::narrow_from_decl(element);
    return (p->pt() != AST_PredefinedType::PT_any)
           && (p->pt() != AST_PredefinedType::PT_object);

  } else return false;
}

bool isArray(AST_Type *t)
{
  if (t->node_type() == AST_Decl::NT_typedef) {
    AST_Typedef *td = AST_Typedef::narrow_from_decl(t);
    t = td->primitive_base_type();
  }

  return t->node_type() == AST_Decl::NT_array;
}

bool isObjref(AST_Type *t)
{
  if (t->node_type() == AST_Decl::NT_typedef) {
    AST_Typedef *td = AST_Typedef::narrow_from_decl(t);
    t = td->primitive_base_type();
  }

  return t->node_type() == AST_Decl::NT_interface
    || t->node_type() == AST_Decl::NT_interface_fwd;
}

bool isSSU(AST_Type *t)  //sequence, struct, union
{
  if (t->node_type() == AST_Decl::NT_typedef) {
    AST_Typedef *td = AST_Typedef::narrow_from_decl(t);
    t = td->primitive_base_type();
  }

  switch (t->node_type()) {
  case AST_Decl::NT_sequence:
  case AST_Decl::NT_struct:
  case AST_Decl::NT_union:
    return true;
  default:
    return false;
  }
}
}

bool idl_mapping_jni::gen_enum(UTL_ScopedName *name,
                               const std::vector<AST_EnumVal *> &, const char *)
{
  commonSetup c(name);
  string enumJVMsig = scoped_helper(name, "/");
  c.cppfile <<
  c.sigToCxx << "\n"
  "{\n"
  "  jclass clazz = jni->GetObjectClass (source);\n"
  "  jfieldID fid = jni->GetFieldID (clazz, \"_value\", \"I\");\n"
  "  target = static_cast<" << c.cxx
  << "> (jni->GetIntField (source, fid));\n"
  "}\n\n" <<
  c.sigToJava << "\n"
  "{\n"
  "  ACE_UNUSED_ARG (createNewObject);\n"
  "  jclass clazz = findClass (jni, \"" << enumJVMsig << "\");\n"
  "  jmethodID factory = jni->GetStaticMethodID (clazz, \"from_int\", "
  "\"(I)L" << enumJVMsig << ";\");\n"
  "  target = jni->CallStaticObjectMethod (clazz, factory, source);\n"
  "}\n\n";
  return true;
}

bool idl_mapping_jni::gen_struct(UTL_ScopedName *name,
                                 const std::vector<AST_Field*> &fields, const char *)
{
  commonSetup c(name);
  string fieldsToCxx, fieldsToJava;

  for (size_t i = 0; i < fields.size(); ++i) {
    string fname = fields[i]->local_name()->get_string(),
                   jvmSig = jvmSignature(fields[i]->field_type()),   // "I"
                            jniFn = jniFnName(fields[i]->field_type()),   // "Int"
                                    fieldID =
                                      "    jfieldID fid = jni->GetFieldID (clazz, \"" + fname + "\", \""
                                      + jvmSig + "\");\n";
    fieldsToCxx += "  {\n" + fieldID;
    fieldsToJava += "  {\n" + fieldID;

    if (isPrimitive(fields[i]->field_type())) {
      fieldsToCxx +=
        "    target." + fname + " = jni->Get" + jniFn
        + "Field (source, fid);\n";
      fieldsToJava +=
        "    jni->Set" + jniFn + "Field (target, fid, source." + fname
        + ");\n";

    } else if (jvmSig[0] == '[') {
      string t = type(fields[i]->field_type());
      fieldsToCxx +=
        "    " + t + " obj = static_cast<" + t + "> (jni->GetObjectField "
        "(source, fid));\n"
        "    copyToCxx (jni, target." + fname + ", obj);\n"
        "    jni->DeleteLocalRef (obj);\n";
      fieldsToJava +=
        "    " + t + " obj = createNewObject ? 0 : "
        "static_cast<" + t + "> (jni->GetObjectField (target, fid));\n"
        "    copyToJava (jni, obj, source." + fname
        + ", createNewObject);\n"
        "    jni->SetObjectField (target, fid, obj);\n"
        "    jni->DeleteLocalRef (obj);\n";

    } else {
      fieldsToCxx +=
        "    jobject obj = jni->GetObjectField (source, fid);\n"
        "    copyToCxx (jni, target." + fname + ", obj);\n"
        "    jni->DeleteLocalRef (obj);\n";
      fieldsToJava +=
        "    jobject obj = createNewObject ? 0 : "
        "jni->GetObjectField (target, fid);\n"
        "    copyToJava (jni, obj, source." + fname
        + ", createNewObject);\n"
        "    jni->SetObjectField (target, fid, obj);\n"
        "    jni->DeleteLocalRef (obj);\n";
    }

    fieldsToCxx += "  }\n";
    fieldsToJava += "  }\n";
  }

  string structJVMsig = scoped_helper(name, "/");

  c.cppfile <<
  c.sigToCxx << "\n"
  "{\n"
  "  jclass clazz = jni->GetObjectClass (source);\n" <<
  fieldsToCxx <<
  "}\n\n" <<
  c.sigToJava << "\n"
  "{\n"
  "  jclass clazz;\n"
  "  if (createNewObject)\n"
  "    {\n"
  "      clazz = findClass (jni, \"" << structJVMsig << "\");\n"
  "      jmethodID ctor = jni->GetMethodID (clazz, \"<init>\", \"()V\");\n"
  "      target = jni->NewObject (clazz, ctor);\n"
  "    }\n"
  "  else\n"
  "    {\n"
  "      clazz = jni->GetObjectClass (target);\n"
  "    }\n" <<
  fieldsToJava <<
  "}\n\n";
  return true;
}

bool idl_mapping_jni::gen_typedef(UTL_ScopedName *name, AST_Type *base,
                                  const char *)
{
  AST_Type *element = 0;
  bool sequence = false;
  size_t arrayLength = 0;

  switch (base->node_type()) {
  case AST_Decl::NT_sequence: {
    AST_Sequence *seq = AST_Sequence::narrow_from_decl(base);
    element = seq->base_type();
    sequence = true;
    break;
  }
  case AST_Decl::NT_array: {
    AST_Array *arr = AST_Array::narrow_from_decl(base);
    element = arr->base_type();

    if (arr->n_dims() != 1) {
      cerr << "ERROR - Multidimensional arrays are not supported\n";
      return false;
    }

    AST_Expression *dim = arr->dims()[0];

    if (dim->ev()->et != AST_Expression::EV_ulong) {
      cerr << "ERROR - Array's dimension type must be ulong\n";
      return false;
    }

    arrayLength = dim->ev()->u.ulval;
    break;
  }
  default: ;//fall through
  }

  if (!element) return true;//nothing needed if it's not an array or a sequence

  string length = "source.length ()";

  if (!sequence) {
    ostringstream oss;
    oss << arrayLength;
    length = oss.str();
  }

  return gen_jarray_copies(name, jvmSignature(element), jniFnName(element),
                           type(element), type(base), taoType(element),
                           sequence, length,
                           element->node_type() == AST_Decl::NT_interface ||
                           element->node_type() == AST_Decl::NT_interface_fwd);
}

bool idl_mapping_jni::gen_jarray_copies(UTL_ScopedName *name,
                                        const string &jvmSig, const string &jniFn, const string &jniType,
                                        const string &jniArrayType, const string &taoTypeName, bool sequence,
                                        const string &length, bool elementIsObjref /* = false */)
{
  commonSetup c(name, jniArrayType.c_str(), false, false);
  string preLoop, postLoopCxx, postLoopJava, preNewArray, newArrayExtra,
  loopCxx, loopJava, actualJniType = jniType,
                                     resizeCxx = sequence ? "  target.length (len);\n" : "";

  if (jvmSig.size() == 1) { //primitive type
    preLoop =
      "  " + jniType + " *buf = jni->Get" + jniFn
      + "ArrayElements (arr, 0);\n";
    string postLoop =
      "  jni->Release" + jniFn + "ArrayElements (arr, buf, ";
    postLoopCxx = postLoop + "JNI_ABORT);\n";
    postLoopJava = postLoop + "0);\n";
    loopCxx = (jvmSig == "C") ?
              "      target[i] = static_cast<char> (buf[i]);\n" :
              "      target[i] = buf[i];\n";
    loopJava =
      "      buf[i] = source[i];\n";

  } else {
    bool useVar = false;

    if (jvmSig[0] == '[' && jvmSig.find(';') != string::npos)
      //nested object array
    {
      actualJniType = "jobject";
      // array-of-array-of-object is a special case because we can't call
      // FindClass("[actual/class/name;") to get the element type
      //for each level, create a dummy array of 0 then GetObjectClass
      const char *begin = jvmSig.c_str();
      const char *iter = begin + jvmSig.find_last_of('[') + 1;
      string basic(iter + 1 /* skip initial L */,
                   strlen(iter + 1) - 1 /* skip final ; */);
      ostringstream oss;
      oss <<
      "      jclass c0 = findClass (jni, \"" << basic << "\");\n";
      size_t ctr = 1;

      for (; iter > begin; --iter, ++ctr) {
        oss <<
        "      jobjectArray a" << ctr << " = jni->NewObjectArray "
        << "(0, c" << (ctr - 1) << ", 0);\n"
        "      jclass c" << ctr << " = jni->GetObjectClass (a" << ctr
        << ");\n"
        "      jni->DeleteLocalRef (a" << ctr << ");\n";
      }

      oss <<
      "      jclass clazz = c" << (ctr - 1) << ";\n";
      loopCxx =
        "      jobjectArray obj = static_cast<jobjectArray> "
        "(jni->GetObjectArrayElement (arr, i));\n";
      loopJava =
        "      jobjectArray obj;\n";
      preNewArray = oss.str();

    } else if (jvmSig[0] == '[') { //nested primitive array
      loopCxx =
        "      " + actualJniType + " obj = static_cast<" + actualJniType
        + "> (jni->GetObjectArrayElement (arr, i));\n";
      loopJava =
        "      " + actualJniType + " obj;\n";
      preNewArray =
        "      jclass clazz = findClass (jni, \"" + jvmSig + "\");\n";
      actualJniType = "jobject";

    } else { //non-nested array of objects
      const char *begin = jvmSig.c_str();
      string taoTypeNoVar, jvmClass(begin + 1 /* skip initial L */,
                                    strlen(begin + 1) - 1 /* skip final ; */);

      if (jvmClass != "java/lang/String") {
        useVar = true;
        taoTypeNoVar.assign(taoTypeName.c_str(),
                            taoTypeName.size() - 4);  //trim "_var"
      }

      loopCxx =
        "      jobject obj = jni->GetObjectArrayElement (arr, i);\n";
      loopJava =
        "      jobject obj = createNewObject ? 0 : "
        "jni->GetObjectArrayElement (arr, i);\n";

      if (useVar) {
        loopJava +=
          "      " + taoTypeName + " var = ";

        if (elementIsObjref) loopJava += taoTypeNoVar + "::_duplicate (";

        loopJava += "source[i]";

        if (elementIsObjref) loopJava += ")";

        loopJava += ";\n";
      }

      preNewArray =
        "      jclass clazz = findClass (jni, \"" + jvmClass + "\");\n";
    }

    newArrayExtra = ", clazz, 0";
    loopCxx +=
      "      copyToCxx (jni, target[i], obj);\n"
      "      jni->DeleteLocalRef (obj);\n";
    loopJava +=
      "      copyToJava (jni, obj, " + string(useVar ? "var" : "source[i]")
      + ", createNewObject);\n"
      "      jni->SetObjectArrayElement (arr, i, obj);\n"
      "      jni->DeleteLocalRef (obj);\n";
  }

  ostringstream toJavaBody;
  toJavaBody <<
  "  jsize len = " << length << ";\n"
  "  " << actualJniType << "Array arr;\n"
  "  if (!createNewObject && jni->GetArrayLength (target) != len) "
  "createNewObject = true;\n"
  "  if (createNewObject)\n"
  "    {\n"
  << preNewArray <<
  "      arr = jni->New" << jniFn << "Array (len" << newArrayExtra << ");\n"
  "    }\n"
  "  else\n"
  "    {\n"
  "      arr = target;\n"
  "    }\n"
  << preLoop <<
  "  for (CORBA::ULong i = 0; i < static_cast<CORBA::ULong> (len); ++i)\n"
  "    {\n"
  << loopJava <<
  "    }\n"
  << postLoopJava <<
  "  target = arr;\n";

  ostringstream toCxxBody;
  toCxxBody <<
  "  " << actualJniType << "Array arr = source;\n"
  "  jsize len = jni->GetArrayLength (arr);\n"
  << resizeCxx
  << preLoop <<
  "  for (CORBA::ULong i = 0; i < static_cast<CORBA::ULong> (len); ++i)\n"
  "    {\n"
  << loopCxx <<
  "    }\n"
  << postLoopCxx;

  c.cppfile <<
  c.sigToCxx << "\n"
  "{\n" <<
  toCxxBody.str() <<
  "}\n\n" <<
  c.sigToJava << "\n"
  "{\n" <<
  toJavaBody.str() <<
  "}\n\n";

  //extra overloads of copyTo{Java,Cxx} to deal with the array C++ _var mapping
  if (!sequence) {
    ACE_CString exporter = be_global->stub_export_macro();
    string altsig_c =
      "void copyToCxx (JNIEnv *jni, " + c.cxx + "_var &target, "
      + jniArrayType + " source)";
    string altsig_j =
      "void copyToJava (JNIEnv *jni, " + jniArrayType + " &target, const "
      + c.cxx + "_var &source, bool createNewObject";
    c.hfile <<
    exporter << (exporter == "" ? "" : "\n") << altsig_c << ";\n" <<
    exporter << (exporter == "" ? "" : "\n") << altsig_j << " = false);\n";
    altsig_j += ')';
    c.cppfile <<
    altsig_c << "\n"
    "{\n" <<
    toCxxBody.str() <<
    "}\n\n" <<
    altsig_j << "\n"
    "{\n" <<
    toJavaBody.str() <<
    "}\n\n";
  }

  return true;
}

namespace {
TAO_IDL_CPP_Keyword_Table cpp_key_tbl_;

bool isCxxKeyword(const char *word)
{
  return cpp_key_tbl_.lookup(word,
                             static_cast<unsigned int>(ACE_OS::strlen(word)));
}

static const char name_sep = '_';

string &escape_name(string &s)
{
  for (size_t i = 0; i < s.length(); ++i) {
    switch (s[i]) {
    case '/':
      s[i] = name_sep;
      break;
    case '_':
      s.replace(i, 1, "_1");
      break;
    case ';':
      s.replace(i, 1, "_2");
      break;
    case '[':
      s.replace(i, 1, "_3");
      break;
    }
  }

  return s;
}

string jni_function_name(const char *jvmClass, const char *method,
                         vector<string> *params = 0)
{
  string classname(jvmClass);
  string methodname(method);

  // According to the JNI spec, a native method name consists of:
  // the prefix Java_
  string name("Java_");
  // a mangled fully-qualified class name
  name += escape_name(classname);
  // an underscore ("_") separator
  name += name_sep;
  // a mangled method name
  name += escape_name(methodname);

  if (params != 0) {
    name += name_sep;
    vector<string>::iterator it = params->begin();

    for (; it != params->end(); ++it) {
      name += escape_name(*it);
    }
  }

  return name;
}

void write_native_unarrow(const char *cxx, const char *javaInterf)
{
  string helper = string(javaInterf) + "Helper",
                  fnName = jni_function_name(helper.c_str(), "native_unarrow");
  be_global->stub_impl_ <<
  "extern \"C\" JNIEXPORT jobject JNICALL\n" <<
  fnName << " (JNIEnv *jni, jclass, jobject obj)\n"
  "{\n"
  "  " << cxx << "_var v;\n"
  "  copyToCxx (jni, v, obj);\n"
  "  jobject result;\n"
  "  copyToJava (jni, result, v);\n"
  "  return result;\n"
  "}\n\n";
}

//add in "_var" if it's not already there
string varify(const string &str)
{
  if (str.size() <= 4 || str.substr(str.size() - 4) != "_var") {
    return str + "_var";
  }

  return str;
}

string arg_conversion(const char *name, AST_Type *type,
                      AST_Argument::Direction direction, string &argconv_in, string &argconv_out,
                      string &tao_argconv_in, string &tao_argconv_out)
{
  string tao = idl_mapping_jni::taoType(type),
               tao_var = varify(tao),
                         jni = idl_mapping_jni::type(type),
                               jvmSig = idl_mapping_jni::jvmSignature(type),
                                        holderSig = idl_mapping::scoped_helper(type->name(), "/") + "Holder",
                                                    non_var = idl_mapping::scoped_helper(type->name(), "::"),
                                                              suffix;

  bool always_var(tao == tao_var);

  if (direction == AST_Argument::dir_IN) {
    argconv_in +=
      "      " + tao + " _c_" + name + ";\n"
      "      copyToCxx (_jni, _c_" + name + ", " + name + ");\n";
    tao_argconv_in +=
      "  " + jni + " _j_" + name + " = 0;\n";

    if (always_var) suffix = ".in ()";

    if (isObjref(type))
      tao_argconv_in +=
        "  " + tao + " _c_" + name + " = " + non_var + "::_duplicate ("
        + name + ");\n"
        "  copyToJava (_jni, _j_" + name + ", _c_" + name + ", true);\n";

    else
      tao_argconv_in +=
        "  copyToJava (_jni, _j_" + string(name) + ", " + name
        + ", true);\n";

  } else if (direction == AST_Argument::dir_INOUT) {
    argconv_in +=
      "      " + jni + " _j_" + name + " = deholderize<" + jni
      + "> (_jni, " + name + ", \"" + jvmSig + "\");\n"
      "      " + tao + " _c_" + name;
    tao_argconv_out +=
      "  " + jni + " _o_" + name + " = deholderize<" + jni + "> (_jni, _j_"
      + name + ", \"" + jvmSig + "\");\n"
      "  _jni->DeleteLocalRef (_j_" + name + ");\n";

    if (isPrimitive(type)) {
      argconv_in += string(" = _j_") + name + ";\n";
      argconv_out +=
        "      _j_" + string(name) + " = _c_" + name + ";\n";
      tao_argconv_in +=
        "  " + jni + " _n_" + name + " = " + name + ";\n";
      tao_argconv_out +=
        "  " + string(name) + " = _o_" + name + ";\n";

    } else {
      argconv_in += ";\n"
                    "      copyToCxx (_jni, _c_" + string(name) + ", _j_" + name
                    + ");\n";
      argconv_out +=
        "      copyToJava (_jni, _j_" + string(name) + ", _c_"
        + name + ");\n";
      tao_argconv_in +=
        "  " + jni + " _n_" + name + " = 0;\n"
        "  copyToJava (_jni, _n_" + name + ", " + name + ", true);\n";
      string pre, post;

      if (isObjref(type)) {
        tao_argconv_out +=
          "  " + tao_var + " _c_" + name + ";\n";
        pre = "_c_";
        post =
          "  " + string(name) + " = _c_" + name + ".inout ();\n";
      }

      tao_argconv_out +=
        "  copyToCxx (_jni, " + pre + name + ", _o_" + name + ");\n"
        "  _jni->DeleteLocalRef (_o_" + name + ");\n" + post;
    }

    argconv_out +=
      "      holderize (_jni, " + string(name) + ", _j_" + name + ", \""
      + jvmSig + "\");\n";
    tao_argconv_in +=
      "  jclass _hc_" + string(name) + " = findClass (_jni, \""
      + holderSig + "\");\n"
      "  jmethodID _hm_" + name + " = _jni->GetMethodID (_hc_" + name
      + ", \"<init>\", \"()V\");\n"
      "  jobject _j_" + name + " = _jni->NewObject (_hc_" + name
      + ", _hm_" + name + ");\n"
      "  holderize (_jni, _j_" + name + ", _n_" + name + ", \"" + jvmSig
      + "\");\n";

    if (always_var) suffix = ".inout ()";

  } else if (direction == AST_Argument::dir_OUT) {
    bool var = type->size_type() != AST_Type::FIXED;
    argconv_in +=
      "      " + (var ? tao_var : tao) + " _c_" + name + ";\n";
    argconv_out +=
      "      " + jni + " _j_" + name;
    tao_argconv_in +=
      "  jclass _hc_" + string(name) + " = findClass (_jni, \""
      + holderSig + "\");\n"
      "  jmethodID _hm_" + name + " = _jni->GetMethodID (_hc_" + name
      + ", \"<init>\", \"()V\");\n"
      "  jobject _j_" + name + " = _jni->NewObject (_hc_" + name
      + ", _hm_" + name + ");\n";
    tao_argconv_out +=
      "  " + jni + " _o_" + name + " = deholderize<" + jni + "> (_jni, _j_"
      + name + ", \"" + jvmSig + "\");\n"
      "  _jni->DeleteLocalRef (_j_" + name + ");\n";

    if (isPrimitive(type)) {
      argconv_out += string(" = _c_") + name + ";\n";
      string jniCast = jni == "jchar" ? "static_cast<CORBA::Char> ("
                       : "";
      tao_argconv_out += "  " + string(name) + " = " + jniCast + "_o_"
                         + name + (jniCast.size() == 0 ? "" : ")") + ";\n";

    } else {
      argconv_out += " = 0;\n"
                     "      copyToJava (_jni, _j_" + string(name) + ", _c_" + name
                     + ((var && !isObjref(type)) ? ".in ()" : "") + ", true);\n";

      if (var) {
        string init = isSSU(type) ? (" = new " + tao) : "";
        tao_argconv_out +=
          "  " + tao_var + " _c_" + name + init + ";\n"
          "  copyToCxx (_jni, _c_" + name + ", _o_" + name + ");\n"
          "  " + name + " = _c_" + name + ".out ();\n";

      } else
        tao_argconv_out +=
          "  copyToCxx (_jni, " + string(name) + ", _o_" + name
          + ");\n";

      tao_argconv_out +=
        "  _jni->DeleteLocalRef (_o_" + string(name) + ");\n";
    }

    argconv_out +=
      "      holderize (_jni, " + string(name) + ", _j_" + name + ", \""
      + jvmSig + "\");\n";

    if (var) suffix = ".out ()";
  }

  return suffix;
}

void write_native_attribute_r(UTL_ScopedName *name, const char *javaStub,
                              AST_Attribute *attr, bool local)
{
  const char *attrname = attr->local_name()->get_string();
  string cxx = idl_mapping_jni::scoped(name);
  string fnName = jni_function_name(javaStub, attrname);
  string retconv, ret_exception, tao_retconv, array_cast;
  string ret = idl_mapping_jni::type(attr->field_type());
  string retval = idl_mapping_jni::taoType(attr->field_type());
  string tao_ret = idl_mapping_jni::taoParam(attr->field_type(),
                                             AST_Argument::dir_IN /*ignored*/, true);
  string tao_retval = ret;
  string jniFn = idl_mapping_jni::jniFnName(attr->field_type());
  string ret_jsig = idl_mapping_jni::jvmSignature(attr->field_type());
  string suffix, extra_type, extra_init, extra_retn;
  bool is_array = isArray(attr->field_type()),
                  is_objref = isObjref(attr->field_type());

  if (is_array || attr->field_type()->size_type() != AST_Type::FIXED) {
    if (!is_array && !is_objref) suffix = ".in ()";

    if (is_array || isSSU(attr->field_type())) {
      extra_type = "_var";
      extra_init = " = new " + retval;
      retval = varify(retval);
    }

    extra_retn = "._retn ()";
  }

  if (ret.size() > 5 && ret.substr(ret.size() - 5) == "Array")
    array_cast = "static_cast<" + ret + "> (";

  retval += " _c_ret = ";
  tao_retval += " _j_ret = ";
  retconv = "      return _c_ret;\n";
  tao_retconv = "  return _j_ret;\n";
  ret_exception = "  return 0;\n";

  if (!isPrimitive(attr->field_type())) {
    retconv =
      "      " + ret + " _j_ret = 0;\n"
      "      copyToJava (_jni, _j_ret, _c_ret" + suffix + ", true);\n"
      "      return _j_ret;\n";
    tao_retconv =
      "  " + idl_mapping_jni::taoType(attr->field_type()) + extra_type
      + " _c_ret" + extra_init + ";\n"
      "  copyToCxx (_jni, _c_ret, _j_ret);\n"
      "  return _c_ret" + extra_retn + ";\n";
  }

  //for the JavaPeer (local interfaces only)
  string tao_args, java_args, args_jsig, tao_argconv_in, tao_argconv_out;

  //for the Stub/TAOPeer (all interfaces)
  string args, cxx_args, argconv_in, argconv_out;

  if (local) {
    string attrname_cxx = isCxxKeyword(attrname) ? (string("_cxx_")
                                                    + attrname) : attrname;
    //FUTURE: support user exceptions
    be_global->stub_header_ <<
      "  " << tao_ret << ' ' << attrname_cxx << " (" << tao_args << ");\n\n";
    be_global->stub_impl_ <<
    tao_ret << ' ' << idl_mapping_jni::scoped_helper(name, "_")
            << "JavaPeer::" << attrname_cxx << " (" << tao_args << ")\n"
    "{\n"
    "  JNIThreadAttacher _jta (jvm_, cl_);\n"
    "  JNIEnv *_jni = _jta.getJNI ();\n"
    << tao_argconv_in <<
    "  jclass _clazz = _jni->GetObjectClass (globalCallback_);\n"
    "  jmethodID _mid = _jni->GetMethodID (_clazz, \"" << attrname
    << "\", \"(" << args_jsig << ")" << ret_jsig << "\");\n"
    "  " << tao_retval << array_cast << "_jni->Call" << jniFn
    << "Method (globalCallback_, _mid" << java_args
    << ((array_cast == "") ? "" : ")") << ");\n"
    "  jthrowable _excep = _jni->ExceptionOccurred ();\n"
    "  if (_excep) throw_cxx_exception (_jni, _excep);\n"
    << tao_argconv_out
    << tao_retconv <<
    "}\n\n";
  }

  be_global->stub_impl_ <<
  "extern \"C\" JNIEXPORT " << ret << " JNICALL\n" <<
  fnName << " (JNIEnv *_jni, jobject _jthis" << args << ")\n"
  "{\n"
  "  CORBA::Object_ptr _this_obj = recoverTaoObject (_jni, _jthis);\n"
  "  try\n"
  "    {\n"
  "      " << cxx << "_var _this = " << cxx << "::_narrow (_this_obj);\n"
  << argconv_in <<
  "      " << retval << "_this->" << (isCxxKeyword(attrname)
                                      ? "_cxx_" : "")
  << attrname << " (" << cxx_args << ");\n"
  << argconv_out
  << retconv <<
  //FUTURE: catch declared user exceptions
  "    }\n"
  "  catch (const CORBA::SystemException &_se)\n"
  "    {\n"
  "      throw_java_exception (_jni, _se);\n"
  "    }\n"
  << ret_exception <<
  "}\n\n";
}

void write_native_attribute_w(UTL_ScopedName *name, const char *javaStub,
                              AST_Attribute *attr, bool local)
{
  const char *attrname = attr->local_name()->get_string();
  string ret = "void";
  string cxx = idl_mapping_jni::scoped(name);
  string retval, retconv, ret_exception;
  //for the JavaPeer (local interfaces only)
  string ret_jsig = "V", jniFn = "Void", tao_ret = "void",
                                                   tao_retval, tao_retconv, array_cast;

  //for the JavaPeer (local interfaces only)
  string java_args, tao_argconv_in, tao_argconv_out;

  //for the Stub/TAOPeer (all interfaces)
  string cxx_args, argconv_in, argconv_out;

  string argname = string(attrname) + '_';
  string args = ", " + idl_mapping_jni::type(attr->field_type())
                + ' ' + argname;
  string tao_args = idl_mapping_jni::taoParam(attr->field_type(),
                                              AST_Argument::dir_IN /*ignored*/) + ' ' + argname;

  if (isPrimitive(attr->field_type())) {
    cxx_args = argname + string(", ");
    java_args = string(", ") + argname;

  } else {
    string suffix = arg_conversion(attrname, attr->field_type(),
                                   AST_Argument::dir_IN /*ignored*/, argconv_in,
                                   argconv_out, tao_argconv_in, tao_argconv_out);
    cxx_args = string("_c_") + attrname + suffix + ", ";
    java_args = ", " + string("_j_") + attrname;
  }

  cxx_args.resize(cxx_args.size() - 2);

  // NOTE: mutating attribute functions should always be overloaded:
  vector<string> params;
  params.push_back(idl_mapping_jni::jvmSignature(attr->field_type()));

  string fnName = jni_function_name(javaStub, attrname, &params);

  if (local) {
    string attrname_cxx = isCxxKeyword(attrname) ? (string("_cxx_")
                                                    + attrname) : attrname;
    //FUTURE: support user exceptions
    be_global->stub_header_ <<
      "  " << tao_ret << ' ' << attrname_cxx << " (" << tao_args << ");\n\n";
    be_global->stub_impl_ <<
    tao_ret << ' ' << idl_mapping_jni::scoped_helper(name, "_")
            << "JavaPeer::" << attrname_cxx << " (" << tao_args << ")\n"
    "{\n"
    "  JNIThreadAttacher _jta (jvm_, cl_);\n"
    "  JNIEnv *_jni = _jta.getJNI ();\n"
    << tao_argconv_in <<
    "  jclass _clazz = _jni->GetObjectClass (globalCallback_);\n"
    "  jmethodID _mid = _jni->GetMethodID (_clazz, \"" << attrname
    << "\", \"(" << params[0] << ")" << ret_jsig << "\");\n"
    "  " << tao_retval << array_cast << "_jni->Call" << jniFn
    << "Method (globalCallback_, _mid" << java_args
    << ((array_cast == "") ? "" : ")") << ");\n"
    "  jthrowable _excep = _jni->ExceptionOccurred ();\n"
    "  if (_excep) throw_cxx_exception (_jni, _excep);\n"
    << tao_argconv_out
    << tao_retconv <<
    "}\n\n";
  }

  be_global->stub_impl_ <<
  "extern \"C\" JNIEXPORT " << ret << " JNICALL\n" <<
  fnName << " (JNIEnv *_jni, jobject _jthis" << args << ")\n"
  "{\n"
  "  CORBA::Object_ptr _this_obj = recoverTaoObject (_jni, _jthis);\n"
  "  try\n"
  "    {\n"
  "      " << cxx << "_var _this = " << cxx << "::_narrow (_this_obj);\n"
  << argconv_in <<
  "      " << retval << "_this->" << (isCxxKeyword(attrname)
                                      ? "_cxx_" : "")
  << attrname << " (" << cxx_args << ");\n"
  << argconv_out
  << retconv <<
  //FUTURE: catch declared user exceptions
  "    }\n"
  "  catch (const CORBA::SystemException &_se)\n"
  "    {\n"
  "      throw_java_exception (_jni, _se);\n"
  "    }\n"
  << ret_exception <<
  "}\n\n";
}

void write_native_operation(UTL_ScopedName *name, const char *javaStub,
                            AST_Operation *op, bool local)
{
  const char *opname = op->local_name()->get_string();
  string ret = "void",
               cxx = idl_mapping_jni::scoped(name),
                     fnName = jni_function_name(javaStub, opname),
                              retval, retconv, ret_exception;
  //for the JavaPeer (local interfaces only)
  string ret_jsig = "V", jniFn = "Void", tao_ret = "void",
                                                   tao_retval, tao_retconv, array_cast;

  if (!op->void_return_type()) {
    ret = idl_mapping_jni::type(op->return_type());
    retval = idl_mapping_jni::taoType(op->return_type());
    tao_ret = idl_mapping_jni::taoParam(op->return_type(),
                                        AST_Argument::dir_IN /*ignored*/, true);
    tao_retval = ret;
    jniFn = idl_mapping_jni::jniFnName(op->return_type());
    ret_jsig = idl_mapping_jni::jvmSignature(op->return_type());
    string suffix, extra_type, extra_init, extra_retn;
    bool is_array = isArray(op->return_type()),
                    is_objref = isObjref(op->return_type());

    if (is_array || op->return_type()->size_type() != AST_Type::FIXED) {
      if (!is_array && !is_objref) suffix = ".in ()";

      if (is_array || isSSU(op->return_type())) {
        extra_type = "_var";
        extra_init = " = new " + retval;
        retval = varify(retval);
      }

      extra_retn = "._retn ()";
    }

    if (ret.size() > 5 && ret.substr(ret.size() - 5) == "Array")
      array_cast = "static_cast<" + ret + "> (";

    retval += " _c_ret = ";
    tao_retval += " _j_ret = ";
    retconv = "      return _c_ret;\n";
    tao_retconv = "  return _j_ret;\n";
    ret_exception = "  return 0;\n";

    if (!isPrimitive(op->return_type())) {
      retconv =
        "      " + ret + " _j_ret = 0;\n"
        "      copyToJava (_jni, _j_ret, _c_ret" + suffix + ", true);\n"
        "      return _j_ret;\n";
      tao_retconv =
        "  " + idl_mapping_jni::taoType(op->return_type()) + extra_type
        + " _c_ret" + extra_init + ";\n"
        "  copyToCxx (_jni, _c_ret, _j_ret);\n"
        "  return _c_ret" + extra_retn + ";\n";
    }
  }

  //for the JavaPeer (local interfaces only)
  string tao_args, java_args, args_jsig, tao_argconv_in, tao_argconv_out;

  //for the Stub/TAOPeer (all interfaces)
  string args, cxx_args, argconv_in, argconv_out;

  for (UTL_ScopeActiveIterator it(op, UTL_Scope::IK_decls); !it.is_done();
       it.next()) {
    AST_Decl *item = it.item();

    if (item->node_type() == AST_Decl::NT_argument) {
      AST_Argument *arg = AST_Argument::narrow_from_decl(item);
      const char *argname = arg->local_name()->get_string();
      bool in = arg->direction() == AST_Argument::dir_IN;
      args += ", " + (in ? idl_mapping_jni::type(arg->field_type())
                      : "jobject")
              + ' ' + argname;

      if (tao_args != "") tao_args += ", ";

      tao_args += idl_mapping_jni::taoParam(arg->field_type(),
                                            arg->direction()) + ' ' + argname;
      args_jsig += in
                   ? idl_mapping_jni::jvmSignature(arg->field_type())
                   : "L" + idl_mapping::scoped_helper(arg->field_type()->name(),
                                                      "/") + "Holder;";

      if (in && isPrimitive(arg->field_type())) {
        cxx_args += argname + string(", ");
        java_args += string(", ") + argname;

      } else {
        string suffix = arg_conversion(argname, arg->field_type(),
                                       arg->direction(), argconv_in, argconv_out, tao_argconv_in,
                                       tao_argconv_out);
        cxx_args += string("_c_") + argname + suffix + ", ";
        java_args += ", " + string("_j_") + argname;
      }
    }
  }

  if (cxx_args.size()) cxx_args.resize(cxx_args.size() - 2);

  if (local) {
    string opname_cxx = isCxxKeyword(opname) ? (string("_cxx_") + opname)
                        : opname;
    //FUTURE: support user exceptions
    be_global->stub_header_ <<
      "  " << tao_ret << ' ' << opname_cxx << " (" << tao_args << ");\n\n";
    be_global->stub_impl_ <<
    tao_ret << ' ' << idl_mapping_jni::scoped_helper(name, "_")
            << "JavaPeer::" << opname_cxx << " (" << tao_args << ")\n"
    "{\n"
    "  JNIThreadAttacher _jta (jvm_, cl_);\n"
    "  JNIEnv *_jni = _jta.getJNI ();\n"
    << tao_argconv_in <<
    "  jclass _clazz = _jni->GetObjectClass (globalCallback_);\n"
    "  jmethodID _mid = _jni->GetMethodID (_clazz, \"" << opname
    << "\", \"(" << args_jsig << ")" << ret_jsig << "\");\n"
    "  " << tao_retval << array_cast << "_jni->Call" << jniFn
    << "Method (globalCallback_, _mid" << java_args
    << ((array_cast == "") ? "" : ")") << ");\n"
    "  jthrowable _excep = _jni->ExceptionOccurred ();\n"
    "  if (_excep) throw_cxx_exception (_jni, _excep);\n"
    << tao_argconv_out
    << tao_retconv <<
    "}\n\n";
  }

  be_global->stub_impl_ <<
  "extern \"C\" JNIEXPORT " << ret << " JNICALL\n" <<
  fnName << " (JNIEnv *_jni, jobject _jthis" << args << ")\n"
  "{\n"
  "  CORBA::Object_ptr _this_obj = recoverTaoObject (_jni, _jthis);\n"
  "  try\n"
  "    {\n"
  "      " << cxx << "_var _this = " << cxx << "::_narrow (_this_obj);\n"
  << argconv_in <<
  "      " << retval << "_this->" << (isCxxKeyword(opname) ? "_cxx_" : "")
  << opname << " (" << cxx_args << ");\n"
  << argconv_out
  << retconv <<
  //FUTURE: catch declared user exceptions
  "    }\n"
  "  catch (const CORBA::SystemException &_se)\n"
  "    {\n"
  "      throw_java_exception (_jni, _se);\n"
  "    }\n"
  << ret_exception <<
  "}\n\n";
}

void recursive_bases(AST_Interface *interf, set<string> &bases,
                     string &ctors)
{
  string name = idl_mapping::scoped_helper(interf->name(), "_");

  if (bases.insert(name).second) {
    for (int i = 0; i < interf->n_inherits(); ++i) {
      AST_Interface *base = dynamic_cast<AST_Interface*>(interf->inherits()[i]);
      recursive_bases(base, bases, ctors);
    }

    ctors += "    , " + name + "JavaPeer (jni, java)\n";
  }
}
}

bool idl_mapping_jni::gen_interf(UTL_ScopedName *name, bool local,
                                 const std::vector<AST_Interface *> &inherits,
                                 const std::vector<AST_Interface *> &inherits_flat,
                                 const std::vector<AST_Attribute *> &attrs,
                                 const std::vector<AST_Operation *> &ops, const char *)
{
  string javaInterf = scoped_helper(name, "/"),
                      javaStub = javaInterf + (local ? "TAOPeer" : "Stub"),
                                 cxx = scoped(name),
                                       name_underscores = scoped_helper(name, "_");
  size_t lastSlash = javaStub.find_last_of('/');
  javaStub.insert(lastSlash == string::npos ? 0 : lastSlash + 1, 1, '_');

  // copyToXXX methods "copy" the object reference across the JNI boundary
  commonSetup c(name, "jobject", true);

  if (local) c.cppfile <<
    c.sigToCxx << "\n"
    "{\n"
    "  if (!source) return;\n"
    "  jclass taoObjClazz = findClass (jni, \"i2jrt/TAOObject\");\n"
    "  if (jni->IsInstanceOf (source, taoObjClazz))\n"
    "    {\n"
    "      CORBA::Object_ptr c = recoverTaoObject (jni, source);\n"
    "      target = " << cxx << "::_unchecked_narrow (c);\n"
    "    }\n"
    "  else\n"
    "    {\n"
    "      target = new " << name_underscores << "JavaPeer (jni, source);\n"
    "    }\n"
    "}\n\n";

  else /*!local*/ c.cppfile <<
    c.sigToCxx << "\n"
    "{\n"
    "  if (!source) return;\n"
    "  CORBA::Object_ptr c = recoverTaoObject (jni, source);\n"
    "  target = " << cxx << "::_unchecked_narrow (c);\n"
    "}\n\n";

  c.cppfile <<
  c.sigToJava << "\n"
  "{\n"
  "  ACE_UNUSED_ARG (createNewObject);\n"
  "  if (CORBA::is_nil (source.in ()))\n"
  "    {\n"
  "      target = 0;\n"
  "      return;\n"
  "    }\n"
  "  jclass stubClazz = findClass (jni, \"" << javaStub << "\");\n"
  "  jmethodID ctor = jni->GetMethodID (stubClazz, \"<init>\", \"(J)V\");\n"
  "  target = jni->NewObject (stubClazz, ctor, reinterpret_cast<jlong> (\n"
  "    CORBA::Object::_duplicate (source.in ())));\n"
  "}\n\n";

  // implement native_unarrow native method for the Helper class
  write_native_unarrow(cxx.c_str(), javaInterf.c_str());

  if (local) {
    be_global->add_include("idl2jni_BaseJavaPeer.h");
    c.hfile << "\n"
    "class" << c.exporter << ' ' << name_underscores << "JavaPeer\n";

    if (inherits.size() == 0) {
      c.hfile << "  : public virtual IDL2JNI_BaseJavaPeer";
    }

    set<string> all_bases;
    string base_ctors;

    for (size_t i = 0; i < inherits.size(); ++i) {
      AST_Interface *base = inherits[i];
      c.hfile << ((i == 0) ? "  : " : ", ") << "public virtual "
      << scoped_helper(base->name(), "_") << "JavaPeer";
      recursive_bases(base, all_bases, base_ctors);
    }

    c.hfile << "\n"
    "  , public virtual " << cxx << "\n"
    "{\n"
    "public:\n"
    "  " << name_underscores << "JavaPeer (JNIEnv *jni, jobject java)\n"
    "    : IDL2JNI_BaseJavaPeer (jni, java)\n" <<
    base_ctors <<
    "  {}\n\n";

    if (inherits.size() > 1) c.hfile <<
      "  CORBA::Boolean _is_a (const char *type_id)\n"
      "  {\n"
      "    return " << cxx << "::_is_a (type_id);\n"
      "  }\n\n"
      "  const char* _interface_repository_id () const\n"
      "  {\n"
      "    return " << cxx << "::_interface_repository_id ();\n"
      "  }\n\n"
      "  CORBA::Boolean marshal (TAO_OutputCDR &cdr)\n"
      "  {\n"
      "    return " << cxx << "::marshal (cdr);\n"
      "  }\n\n";
  }

  // implement native methods for the stub (w/ inherits_flat)
  for (size_t i = 0; i < attrs.size(); ++i) {
    AST_Attribute *attr = attrs[i];

    write_native_attribute_r(name, javaStub.c_str(), attr, local);

    if (!attr->readonly()) {
      write_native_attribute_w(name, javaStub.c_str(), attr, local);
    }
  }

  for (size_t i = 0; i < ops.size(); ++i) {
    write_native_operation(name, javaStub.c_str(), ops[i], local);
  }

  if (local) c.hfile << "};\n\n";

  // LOCAL SHOULD NOT BE GENERATED BEYOND THIS POINT!

  for (size_t i = 0; i < inherits_flat.size(); ++i) {
    AST_Interface *base = inherits_flat[i];
    UTL_ScopeActiveIterator it(base, UTL_Scope::IK_decls);

    for (; !it.is_done(); it.next()) {
      AST_Decl *item = it.item();

      if (item->node_type() == AST_Decl::NT_attr) {
        AST_Attribute *attr = AST_Attribute::narrow_from_decl(item);

        write_native_attribute_r(name, javaStub.c_str(), attr, false);

        if (!attr->readonly()) {
          write_native_attribute_w(name, javaStub.c_str(),
                                   attr, false);
        }

      } else if (item->node_type() == AST_Decl::NT_op) {
        AST_Operation *op = AST_Operation::narrow_from_decl(item);
        write_native_operation(name, javaStub.c_str(), op, false);
      }
    }
  }

  //FUTURE: server side
  return true;
}

bool idl_mapping_jni::gen_interf_fwd(UTL_ScopedName *name)
{
  commonSetup c(name, "jobject", true, true);
  return true;
}

bool idl_mapping_jni::gen_native(UTL_ScopedName *name, const char *)
{
  string seq(scoped_helper(name, "/")), seq_cxx(scoped(name));
  string elem;
  bool info(false);

  switch (get_native_type(name, elem)) {
  case NATIVE_INFO_SEQUENCE:
    info = true;
    // fall through

  case NATIVE_DATA_SEQUENCE: {
    string elem_cxx(elem);

    // $elem_cxx =~ s/\//::/g;
    for (size_t iter = elem_cxx.find('/'); iter != string::npos;
         iter = elem_cxx.find('/', iter + 1)) {
      elem_cxx.replace(iter, 1, "::");
    }

    if (info) elem_cxx += "_var";

    return gen_jarray_copies(name, "L" + elem + ";", "Object", "jobject",
                             "jobjectArray", elem_cxx, true, "source.length ()");
  }
  default:
    ; // fall through
  }

  return true;
}

namespace {
ostream &operator<< (ostream &o, AST_Expression::AST_ExprValue *ev)
{
  switch (ev->et) {
  case AST_Expression::EV_short:
    o << ev->u.sval;
    break;
  case AST_Expression::EV_ushort:
    o << ev->u.usval;
    break;
  case AST_Expression::EV_long:
    o << ev->u.lval;
    break;
  case AST_Expression::EV_ulong:
    o << ev->u.ulval;
    break;
  case AST_Expression::EV_longlong:
    o << ev->u.llval << "LL";
    break;
  case AST_Expression::EV_ulonglong:
    o << ev->u.ullval << "ULL";
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
    o << "L\'" << ev->u.wcval << '\'';
    break;
  case AST_Expression::EV_octet:
    o << ev->u.oval;
    break;
  case AST_Expression::EV_bool:
    o << boolalpha << static_cast<bool>(ev->u.bval);
    break;
  case AST_Expression::EV_string:
    o << '"' << ev->u.strval->get_string() << '"';
    break;
  case AST_Expression::EV_wstring:
    o << "L\"" << ev->u.wstrval << '"';
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
}

bool idl_mapping_jni::gen_union(UTL_ScopedName *name,
                                const std::vector<AST_UnionBranch *> &branches, AST_Type *discriminator,
                                AST_Expression::ExprType, const AST_Union::DefaultValue &, const char *)
{
  string disc_ty = type(discriminator),
                   disc_sig = jvmSignature(discriminator),
                              disc_meth = jniFnName(discriminator),
                                          branchesToCxx, branchesToJava;
  bool someBranchUsesExplicitDisc(false);

  for (size_t i = 0; i < branches.size(); ++i) {
    unsigned long n_labels = branches[i]->label_list_length();
    bool useExplicitDisc(n_labels > 1);

    for (unsigned long j = 0; j < n_labels; ++j) {
      AST_UnionLabel *ul = branches[i]->label(j);
      ostringstream oss;

      if (ul->label_kind() == AST_UnionLabel::UL_default) {
        useExplicitDisc = true;
        branchesToCxx  += "    default:\n";
        branchesToJava += "    default:\n";

      } else {
        ostringstream oss;
        oss << ul->label_val()->ev();
        string ccasename(oss.str());

        if (ccasename == "true") ccasename = "JNI_TRUE";

        if (ccasename == "false") ccasename = "JNI_FALSE";

        branchesToCxx += "    case " + ccasename + ":\n";

        //for the "toJava" side, use actual enumerators instead of ints
        if (ul->label_val()->ev()->et == AST_Expression::EV_enum) {
          string enum_val = scoped(ul->label_val()->n()),
                            enum_type = scoped(discriminator->name());
          size_t idx = enum_type.rfind("::");

          if (idx != string::npos) enum_type.resize(idx);

          branchesToJava +=
            "    case " + enum_type + "::" + enum_val + ":\n";

        } else {
          branchesToJava += "    case " + oss.str() + ":\n";
        }
      }
    }

    string br_type = type(branches[i]->field_type()),
                     br_sig = jvmSignature(branches[i]->field_type()),
                              br_fn = jniFnName(branches[i]->field_type()),
                                      br_tao = taoType(branches[i]->field_type()),
                                               copyToCxx, jniCast, cxxCast, copyToJava, edisc;

    if (useExplicitDisc) {
      someBranchUsesExplicitDisc = true;
      edisc = "jdisc, ";
    }

    const char *br_name = branches[i]->local_name()->get_string();

    if (isPrimitive(branches[i]->field_type())) {
      if (br_sig[0] == 'C') cxxCast = "static_cast<CORBA::Char> (";

      copyToCxx =
        "        target." + string(br_name) + " (" + cxxCast + "value"
        + (cxxCast.size() ? ")" : "") + ");\n";
      copyToJava =
        "        jni->CallVoidMethod (target, mid, " + edisc + "source."
        + string(br_name) + " ());\n";

    } else {
      if (br_sig[0] == '[') {
        jniCast = "static_cast<" + br_type + "> (";
      }

      if (branches[i]->field_type()->node_type() == AST_Decl::NT_typedef) {
        AST_Typedef *td = AST_Typedef::narrow_from_decl(branches[i]->field_type());
        if (td->primitive_base_type()->node_type() == AST_Decl::NT_string) {
          br_tao = "CORBA::String_var";
        }
      }

      copyToCxx =
        "        " + br_tao + " taoVal;\n"
        "        copyToCxx (jni, taoVal, value);\n"
        "        target." + br_name + " (taoVal);\n";
      copyToJava =
        "        jfieldID fid = jni->GetFieldID (clazz, \""
        + string(br_name) + "\", \"" + br_sig + "\");\n"
        "        " + br_type + " obj = createNewObject ? 0 : " + jniCast +
        "jni->GetObjectField (target, fid)" + (jniCast.size() ? ")" : "")
        + ";\n"
        "        copyToJava (jni, obj, source." + br_name + " (), !obj);\n"
        "        jni->CallVoidMethod (target, mid, " + edisc + "obj);\n"
        "        jni->DeleteLocalRef (obj);\n";
    }

    branchesToCxx +=
      "      {\n"
      "        jmethodID mid = jni->GetMethodID (clazz, \""
      + string(br_name) + "\", \"()" + br_sig + "\");\n"
      "        " + br_type + " value = " + jniCast + "jni->Call" + br_fn
      + "Method (source, mid)" + (jniCast.size() ? ")" : "") + ";\n" +
      copyToCxx +
      "        break;\n"
      "      }\n";
    string extraArg = useExplicitDisc ? disc_sig : "";
    branchesToJava +=
      "      {\n"
      "        jmethodID mid = jni->GetMethodID (clazz, \""
      + string(br_name) + "\", \"(" + extraArg + br_sig + ")V\");\n" +
      copyToJava +
      "        break;\n"
      "      }\n";
  }

  commonSetup c(name);
  string unionJVMsig = scoped_helper(name, "/");
  bool disc_is_enum(disc_meth == "Object");
  string disc_name = disc_is_enum ? "disc_val" : "disc",
                     extra_enum1 = disc_is_enum ?
                                   "  jmethodID mid_disc_val = jni->GetMethodID (jni->GetObjectClass "
                                   "(disc), \"value\", \"()I\");\n"
                                   "  jint disc_val = jni->CallIntMethod (disc, mid_disc_val);\n"
                                   "  jni->DeleteLocalRef (disc);\n"
                                   : "",
                                   extra_enum2 = disc_is_enum ?
                                                 "static_cast<" + taoType(discriminator) + "> (disc_val)"
                                                 : "disc",
                                                 explicitDiscSetup, explicitDiscCleanup;

  if (someBranchUsesExplicitDisc && disc_is_enum) {
    string enum_sig = scoped_helper(discriminator->name(), "/");
    explicitDiscSetup =
      "  jclass dclazz = findClass (jni, \"" + enum_sig + "\");\n"
      "  jmethodID from_int = jni->GetStaticMethodID (dclazz, \"from_int\", "
      "\"(I)" + disc_sig + "\");\n"
      "  jobject jdisc = jni->CallStaticObjectMethod (dclazz, from_int, "
      "static_cast<jint> (source._d ()));\n";
    explicitDiscCleanup =
      "  jni->DeleteLocalRef (jdisc);\n";

  } else if (someBranchUsesExplicitDisc) {
    explicitDiscSetup =
      "  " + disc_ty + " jdisc = source._d ();\n";
  }

  c.cppfile <<
  c.sigToCxx << "\n"
  "{\n"
  "  jclass clazz = jni->GetObjectClass (source);\n" <<
  "  jmethodID mid_disc = jni->GetMethodID (clazz, \"discriminator\", \"()"
  << disc_sig << "\");\n"
  "  " << disc_ty << " disc = jni->Call" << disc_meth << "Method (source, "
  "mid_disc);\n" <<
  extra_enum1 <<
  "  switch (" << disc_name << ")\n"
  "    {\n" <<
  branchesToCxx <<
  "    }\n"
  "  target._d (" << extra_enum2 << ");\n" //in case the Java side had one of
  "}\n\n" <<                               //the "alternate" labels for 1 br.
  c.sigToJava << "\n"
  "{\n"
  "  jclass clazz;\n"
  "  if (createNewObject)\n"
  "    {\n"
  "      clazz = findClass (jni, \"" << unionJVMsig << "\");\n"
  "      jmethodID ctor = jni->GetMethodID (clazz, \"<init>\", \"()V\");\n"
  "      target = jni->NewObject (clazz, ctor);\n"
  "    }\n"
  "  else\n"
  "    {\n"
  "      clazz = jni->GetObjectClass (target);\n"
  "    }\n" <<
  explicitDiscSetup <<
  "  switch (source._d ())\n"
  "    {\n" <<
  branchesToJava <<
  "    }\n" <<
  explicitDiscCleanup <<
  "}\n\n";
  return true;
}
