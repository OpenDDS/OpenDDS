#include "language_mapping.h"
#include "global_extern.h"

#include "be_builtin.h"
#include "dds_visitor.h"
#include "dds/Version.h"

#include <ace/OS_NS_strings.h>
#include <ace/OS_NS_sys_time.h>
#include <ace/OS_NS_unistd.h>

#include <fstream>

LanguageMapping::~LanguageMapping()
{
}

bool LanguageMapping::cxx11() const
{
  return false;
}

bool LanguageMapping::none() const
{
  return true;
}

std::string LanguageMapping::getMinimalHeaders() const
{
  return "";
}

std::string LanguageMapping::getInputCDRToString(bool wide) const
{
  return wide ? "Serializer::ToBoundedString<wchar_t>" : "Serializer::ToBoundedString<char>";
}

std::string LanguageMapping::getBranchStringType(bool wide) const
{
  return wide ? "OPENDDS_WSTRING" : "OPENDDS_STRING";
}

std::string LanguageMapping::getBranchStringPrefix() const
{
  return "CORBA::";
}

std::string LanguageMapping::getBranchStringSuffix() const
{
  return "";
}

std::string LanguageMapping::getBoundStringSuffix() const
{
  return ".c_str()";
}

bool LanguageMapping::needSequenceTypeSupportImplHeader() const
{
  return true;
}

bool LanguageMapping::skipTAOSequences() const
{
  return false;
}

void LanguageMapping::setTS(bool setting)
{
}

void LanguageMapping::produce() const
{
  const char* idl_fn = idl_global->main_filename()->get_string();
  be_builtin_global->filename(idl_fn);

  const BE_BuiltinGlobalData::stream_enum_t out_stream =
    be_builtin_global->language_mapping()->none()
    ? BE_BuiltinGlobalData::STREAM_H : BE_BuiltinGlobalData::STREAM_LANG_H;

  ifstream idl(idl_fn);
  const size_t buffer_sz = 512;
  char buffer[buffer_sz];
  unsigned lineno = 0;

  while (idl) {
    idl.getline(buffer, buffer_sz);
    ++lineno;

    // search for #includes in the IDL, add them as #includes in the stubs/skels
    if (0 == strncmp("#include", buffer, 8)) { //FUTURE: account for comments?
      std::string inc(buffer + 8);
      size_t delim1 = inc.find_first_of("<\"");
      size_t delim2 = inc.find_first_of(">\"", delim1 + 1);
      std::string included(inc, delim1 + 1, delim2 - delim1 - 1);
      size_t len = included.size();
      std::string base_name;

      if (len >= 5 &&
        0 == ACE_OS::strcasecmp(included.c_str() + len - 4, ".idl")) {
        base_name.assign(included.c_str(), len - 4);

      }
      else if (len >= 6 &&
        0 == ACE_OS::strcasecmp(included.c_str() + len - 5, ".pidl")) {
        base_name.assign(included.c_str(), len - 5);

      }
      else {
        continue;
      }

      if (be_builtin_global->language_mapping()->skipTAOSequences() &&
        base_name.substr(0, 4) == "tao/" &&
        base_name.substr(base_name.size() - 3) == "Seq") {
        continue; // with Safety Profile C++, skip include of tao/*SeqC.h
      }

      std::string stb_inc = base_name + "C.h";
      if (stb_inc != "tao/orbC.h") {
        be_builtin_global->add_include(stb_inc.c_str(), out_stream);
        if (stb_inc == "orbC.h" ||
          (stb_inc.size() >= 7
            && stb_inc.substr(stb_inc.size() - 7) == "/orbC.h")) {
          be_builtin_global->warning(
            "Potential inclusion of TAO orbC.h\n"
            "  Include TAO orb.idl with path of tao/orb.idl"
            "  to prevent compilation errors",
            idl_fn, lineno);
        }
      }

    }
  }

  idl.close();

  be_builtin_global->open_streams(idl_fn);

  AST_Decl* d = idl_global->root();
  AST_Root* root = dynamic_cast<AST_Root*>(d);

  if (root == 0) {
    ACE_ERROR((LM_ERROR,
      ACE_TEXT("(%N:%l) BE_produce - ")
      ACE_TEXT("No Root\n")));

    BE_abort();
  }

  be_builtin_global->set_inc_paths(idl_global->idl_flags());

  const bool java_ts_only = be_builtin_global->java_arg().length() > 0;

  dds_visitor visitor(d, java_ts_only);

  if (root->ast_accept(&visitor) == -1) {
    ACE_ERROR((LM_ERROR,
      ACE_TEXT("(%N:%l) BE_produce -")
      ACE_TEXT(" failed to accept adding visitor\n")));
    BE_abort();
  }

  if (!java_ts_only) {
    postprocess(be_builtin_global->header_name_.c_str(),
      be_builtin_global->header_, BE_BuiltinGlobalData::STREAM_H);
    if (!be_builtin_global->suppress_idl()) {
      postprocess(be_builtin_global->idl_name_.c_str(),
        be_builtin_global->idl_, BE_BuiltinGlobalData::STREAM_IDL);
    }
  }

  postprocess(be_builtin_global->impl_name_.c_str(),
    be_builtin_global->impl_, BE_BuiltinGlobalData::STREAM_CPP);

  if (be_builtin_global->itl()) {
    if (!BE_BuiltinGlobalData::writeFile(be_builtin_global->itl_name_.c_str(), be_builtin_global->itl_.str())) {
      BE_abort();  //error message already printed
    }
  }

  produceTS();

  if (!be_builtin_global->language_mapping()->none()) {
    postprocess(be_builtin_global->lang_header_name_.c_str(), be_builtin_global->lang_header_,
      BE_BuiltinGlobalData::STREAM_LANG_H);
  }
}

void LanguageMapping::produceTS() const
{
}

GeneratorBase* LanguageMapping::getGeneratorHelper() const
{
  return 0;
}

LanguageMapping::Includes_t* LanguageMapping::additional_includes(int which)
{
  return 0;
}

void LanguageMapping::reset_includes()
{
}

void LanguageMapping::set_additional_names(const std::string& filebase)
{
}

/// generate a macro name for the #ifndef header-double-include protector
std::string LanguageMapping::to_macro(const char* fn) const
{
  std::string ret = "OPENDDS_IDL_GENERATED_";

  for (size_t i = 0; i < strlen(fn); ++i) {
    if (isalnum(fn[i])) {
      ret += static_cast<char>(toupper(fn[i]));
    }
    else if (ret[ret.size() - 1] != '_') {
      ret += '_';
    }
  }

  // Add some random characters since two files of the same name (in different
  // directories) could be used in the same translation unit.  The algorithm
  // for randomness comes from TAO_IDL's implementation.

  const size_t NUM_CHARS = 6;

  const ACE_Time_Value now = ACE_OS::gettimeofday();
  ACE_UINT64 msec;
  now.msec(msec);

  msec += ACE_OS::getpid() + (size_t)ACE_OS::thr_self();

  unsigned int seed = static_cast<unsigned int>(msec);

  if (ret[ret.size() - 1] != '_') ret += '_';
  static const char alphanum[] =
    "0123456789"
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ";

  for (unsigned int n = 0; n < NUM_CHARS; ++n) {
    ret += alphanum[ACE_OS::rand_r(&seed) % (sizeof(alphanum) - 1)];
  }

  return ret;
}

/// change *.cpp to *.h
std::string LanguageMapping::to_header(const char* cpp_name) const
{
  size_t len = strlen(cpp_name);
  assert(len >= 5 && 0 == ACE_OS::strcasecmp(cpp_name + len - 4, ".cpp"));
  std::string base_name(cpp_name, len - 4);
  return base_name + ".h";
}

void LanguageMapping::emit_tao_header(std::ostringstream& out) const
{
  std::string taoheader = be_builtin_global->header_name_.c_str();
  taoheader.replace(taoheader.find("TypeSupportImpl.h"), 17, "C.h");
  const bool explicit_ints = be_builtin_global->tao_inc_pre_.length()
    && (taoheader == "Int8SeqC.h" || taoheader == "UInt8SeqC.h");
  std::string indent;
  if (explicit_ints) {
    out << "#if OPENDDS_HAS_EXPLICIT_INTS\n";
    indent = "  ";
  }
  out << "#" << indent << "include \"" << be_builtin_global->tao_inc_pre_ << taoheader << "\"\n";
  if (explicit_ints) {
    out << "#endif\n";
  }
}

void LanguageMapping::postprocess(const char* fn, const std::ostringstream& content, int iwhich) const
{
  const BE_BuiltinGlobalData::stream_enum_t which =
    static_cast<BE_BuiltinGlobalData::stream_enum_t>(iwhich);
  std::ostringstream out;

  if (which == BE_BuiltinGlobalData::STREAM_H ||
      which == BE_BuiltinGlobalData::STREAM_LANG_H) {
    out << "/* -*- C++ -*- */\n";
  }

  out << "/* Generated by " << idl_global->prog_name()
    << " version " OPENDDS_VERSION " (ACE version " ACE_VERSION
    << ") running on input file "
    << idl_global->main_filename()->get_string()
    << " */\n";

  //  if .h add #ifndef...#define
  const std::string macrofied = to_macro(fn);
  const std::string ifndef = "#ifndef " + macrofied + "\n#define " + macrofied + "\n";

  switch (which) {
  case BE_BuiltinGlobalData::STREAM_H: {
    out << ifndef
        <<
        "\n"
        "#include <dds/Version.h>\n"
        "#if !OPENDDS_VERSION_EXACTLY(" << OPENDDS_MAJOR_VERSION
        << ", " << OPENDDS_MINOR_VERSION
        << ", " << OPENDDS_MICRO_VERSION << ")\n"
        "#  error \"This file should be regenerated with opendds_idl\"\n"
        "#endif\n"
        "#include <dds/DCPS/Definitions.h>\n"
        "\n"
        "#include <dds/DdsDcpsC.h>\n"
        "\n";
    emit_tao_header(out);
    break;
  }
  case BE_BuiltinGlobalData::STREAM_LANG_H: {
    out << ifndef
        << getMinimalHeaders();
  }
  break;
  case BE_BuiltinGlobalData::STREAM_CPP: {
    const ACE_CString pch = be_builtin_global->pch_include();
    if (pch.length()) {
      out << "#include \"" << pch << "\"\n";
    }
    if (be_builtin_global->java_arg().length() == 0) {
      out << "#include \"" << to_header(fn) << "\"\n\n";
    }
    else {
      out << "#include \"" << be_builtin_global->header_name_.c_str() << "\"\n\n";
    }
  }
  break;
  case BE_BuiltinGlobalData::STREAM_IDL: {
    out << ifndef;

#ifdef ACE_HAS_CDR_FIXED
    out << "#define __OPENDDS_IDL_HAS_FIXED\n";
#endif

    std::string filebase(be_builtin_global->filename());
    const size_t idx = filebase.find_last_of("/\\"); // allow either slash
    if (idx != std::string::npos) {
      filebase = filebase.substr(idx + 1);
    }
    out << "#include \"" << filebase << "\"\n\n";
  }
  break;
  default:
    postprocess_guard_begin(macrofied, out, iwhich);
    break;
  }

  out << be_builtin_global->get_include_block(which);

  out << content.str();

  switch (which) {
  case BE_BuiltinGlobalData::STREAM_H:
  case BE_BuiltinGlobalData::STREAM_IDL:
  case BE_BuiltinGlobalData::STREAM_LANG_H:
    out << "#endif /* " << macrofied << " */\n";
    break;
  default:
    postprocess_guard_end(macrofied, out, iwhich);
    break;
  }

  if (!BE_BuiltinGlobalData::writeFile(fn, out.str())) {
    BE_abort();  //error message already printed
  }
}

void LanguageMapping::postprocess_guard_begin(const std::string& macro, std::ostringstream& content, int which) const
{
}

void LanguageMapping::postprocess_guard_end(const std::string& macro, std::ostringstream& content, int which) const
{
}
