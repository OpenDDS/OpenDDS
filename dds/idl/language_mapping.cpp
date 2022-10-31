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
  return "";
}

std::string LanguageMapping::getBranchStringType(bool wide) const
{
  return "";
}

std::string LanguageMapping::getBranchStringPrefix() const
{
  return "";
}

std::string LanguageMapping::getBranchStringSuffix() const
{
  return ".c_str()";
}

std::string LanguageMapping::getBoundStringSuffix() const
{
  return "";
}

bool LanguageMapping::needSequenceTypeSupportImplHeader() const
{
  return false;
}

bool LanguageMapping::skipTAOSequences() const
{
  return false;
}

GeneratorBase* LanguageMapping::getGeneratorHelper() const
{
  return 0;
}