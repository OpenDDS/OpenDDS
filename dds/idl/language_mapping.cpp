#include "language_mapping.h"

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

GeneratorBase* LanguageMapping::getGeneratorHelper() const
{
  return 0;
}