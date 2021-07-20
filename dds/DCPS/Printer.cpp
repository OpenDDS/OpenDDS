/*
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "DCPS/DdsDcps_pch.h" //Only the _pch include should start with DCPS/

#ifndef OPENDDS_SAFETY_PROFILE

#include "Printer.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

Printer::Printer()
: indent_char_(' ')
, indent_char_count_(4)
, initial_indent_level_(0)
, print_field_names_(true)
{
}

char Printer::indent_char() const
{
  return indent_char_;
}

void Printer::indent_char(char new_value)
{
  indent_char_ = new_value;
}

unsigned Printer::indent_char_count() const
{
  return indent_char_count_;
}

void Printer::indent_char_count(unsigned new_value)
{
  indent_char_count_ = new_value;
}

unsigned Printer::initial_indent_level() const
{
  return initial_indent_level_;
}

void Printer::initial_indent_level(unsigned new_value)
{
  initial_indent_level_ = new_value;
}

bool Printer::print_field_names() const
{
  return print_field_names_;
}

void Printer::print_field_names(bool new_value)
{
  print_field_names_ = new_value;
}

Printable operator<<(std::ostream& os, const Printer& printer)
{
  return Printable(os, printer);
}

Printable::Printable(std::ostream& os, const Printer& printer)
: os_(os)
, printer_(printer)
, indent_level_(printer.initial_indent_level())
{
}

std::ostream& Printable::os()
{
  return os_;
}

const Printer& Printable::printer()
{
  return printer_;
}

void Printable::push_indent()
{
  ++indent_level_;
}

void Printable::pop_indent()
{
  if (indent_level_) {
    --indent_level_;
  }
}

void Printable::print_indent()
{
  os_ << std::string(
    printer_.indent_char_count() * indent_level_, printer_.indent_char());
}


} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif // ifndef OPENDDS_SAFETY_PROFILE
