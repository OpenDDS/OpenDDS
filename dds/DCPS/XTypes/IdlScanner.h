/*
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_XTYPES_IDL_SCANNER_H
#define OPENDDS_DCPS_XTYPES_IDL_SCANNER_H

#ifndef OPENDDS_SAFETY_PROFILE

#  include <dds/DCPS/dcps_export.h>

#  include <dds/DCPS/Definitions.h>
#  include <dds/DdsDynamicDataC.h>

#  include <string>

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL
namespace OpenDDS {
namespace XTypes {

class OpenDDS_Dcps_Export CharacterScanner {
public:
  CharacterScanner(const std::string& str)
    : str_(str)
    , idx_(0)
  {}

  char peek() const
  {
    OPENDDS_ASSERT(!eoi());
    return str_[idx_];
  }

  bool eoi() const
  {
    return idx_ == str_.size();
  }

  void consume()
  {
    OPENDDS_ASSERT(!eoi());
    ++idx_;
  }

  bool match(char c)
  {
    if (eoi() || peek() != c) {
      return false;
    }

    consume();
    return true;
  }

private:
  const std::string str_;
  size_t idx_;
};

class OpenDDS_Dcps_Export IdlToken {
public:
  enum Kind {
    Error,
    EOI,
    BooleanLiteral,
    IntegerLiteral,
    FloatLiteral,
    StringLiteral,
    CharacterLiteral,
    Identifier
  };

  IdlToken()
  : kind_(Error)
  , signed_(false)
  , integer_value_(0)
  , exponent_signed_(false)
  , exponent_value_(0)
  {}

  static IdlToken make_error()
  {
    IdlToken token;
    token.kind_ = Error;
    return token;
  }

  static IdlToken make_eoi()
  {
    IdlToken token;
    token.kind_ = EOI;
    return token;
  }

  static IdlToken make_boolean(bool value)
  {
    IdlToken token;
    token.kind_ = BooleanLiteral;
    token.boolean_value_ = value;
    return token;
  }

  static IdlToken make_character(char value)
  {
    IdlToken token;
    token.kind_ = CharacterLiteral;
    token.character_value_ = value;
    return token;
  }

  static IdlToken make_string(const std::string& value)
  {
    IdlToken token;
    token.kind_ = StringLiteral;
    token.string_value_ = value;
    return token;
  }

  static IdlToken make_integer(bool is_signed, ACE_UINT64 value)
  {
    IdlToken token;
    token.kind_ = IntegerLiteral;
    token.signed_ = is_signed;
    token.integer_value_ = value;
    return token;
  }

  static IdlToken make_float(bool is_signed, ACE_UINT64 value, bool exponent_is_signed, ACE_UINT64 exponent_value)
  {
    IdlToken token;
    token.kind_ = FloatLiteral;
    token.signed_ = is_signed;
    token.integer_value_ = value;
    token.exponent_signed_ = exponent_is_signed;
    token.exponent_value_ = exponent_value;
    return token;
  }

  static IdlToken make_identifier(const std::string& value)
  {
    IdlToken token;
    if (value == "TRUE") {
      token.kind_ = BooleanLiteral;
      token.boolean_value_ = true;
    } else if (value == "FALSE") {
      token.kind_ = BooleanLiteral;
      token.boolean_value_ = false;
    } else {
      token.kind_ = Identifier;
      token.identifier_value_ = value;
    }
    return token;
  }

  bool operator==(const IdlToken& other) const
  {
    if (kind_ != other.kind_) {
      return false;
    }

    switch (kind_) {
    case Error:
    case EOI:
      return true;
    case BooleanLiteral:
      return boolean_value_ == other.boolean_value_;
    case IntegerLiteral:
      return signed_ == other.signed_ && integer_value_ == other.integer_value_;
    case FloatLiteral:
      return signed_ == other.signed_ && integer_value_ == other.integer_value_
        && exponent_signed_ == other.exponent_signed_ && exponent_value_ == other.exponent_value_;
    case StringLiteral:
      return string_value_ == other.string_value_;
    case CharacterLiteral:
      return character_value_ == other.character_value_;
    case Identifier:
      return identifier_value_ == other.identifier_value_;
    default:
      return false;
    }
  }

  bool is_error() const
  {
    return kind_ == Error;
  }

  bool is_boolean() const
  {
    return kind_ == BooleanLiteral;
  }

  bool is_integer() const
  {
    return kind_ == IntegerLiteral;
  }

  bool is_float() const
  {
    return kind_ == FloatLiteral;
  }

  bool is_identifier() const
  {
    return kind_ == Identifier;
  }

  bool is_signed() const
  {
    return signed_;
  }

  ACE_UINT64 integer_value() const
  {
    return integer_value_;
  }

  bool exponent_is_signed() const
  {
    return exponent_signed_;
  }

  ACE_UINT64 exponent_value() const
  {
    return exponent_value_;
  }

  const std::string& identifier_value() const
  {
    return identifier_value_;
  }

private:
  Kind kind_;
  union {
    bool boolean_value_;
    char character_value_;
    struct {
      bool signed_;  // Integer or Float is signed.
      ACE_UINT64 integer_value_; // Integer or Float.
      bool exponent_signed_; // Exponent is signed for Float.
      ACE_UINT64 exponent_value_; // Exponent value for Float.
    };
  };
  std::string string_value_;
  std::string identifier_value_;
};

class OpenDDS_Dcps_Export IdlScanner {
public:
  IdlScanner(CharacterScanner& scanner)
    : scanner_(scanner)
  {}

  IdlToken scan_token()
  {
    if (scanner_.eoi()) {
      return IdlToken::make_eoi();
    }

    const char c = scanner_.peek();
    if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z')) {
      return scan_identifier();
    }

    switch (c) {
    case '\'':
      if (scan_character_literal()) {
        return IdlToken::make_character(character_literal_);
      } else {
        return IdlToken::make_error();
      }
    case '"':
      if (scan_string_literal()) {
        return IdlToken::make_string(string_literal_);
      } else {
        return IdlToken::make_error();
      }
    case '-':
    case '0':
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
    case '8':
    case '9':
    case '.':
      return scan_numeric_literal();
    default:
      return IdlToken::make_error();
    }
  }

  // Method for scanning default_values.
  IdlToken scan_token(DDS::DynamicType_ptr type);

  bool eoi() const
  {
    return scanner_.eoi();
  }

private:

  IdlToken scan_identifier()
  {
    identifier_.clear();

    if (scan_identifier_alpha() && scan_identifier_optional_characters()) {
      return IdlToken::make_identifier(identifier_);
    }

    return IdlToken::make_error();
  }

  bool scan_identifier_alpha()
  {
    if (scanner_.eoi()) {
      return false;
    }

    const char c = scanner_.peek();
    if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z')) {
      return scanner_.match(c) && append_to_identifier(c);
    }

    return false;
  }

  bool scan_identifier_optional_characters()
  {
    if (scanner_.eoi()) {
      return true;
    }

    const char c = scanner_.peek();
    if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '_') {
      return scanner_.match(c) && append_to_identifier(c) && scan_identifier_optional_characters();
    }

    return true;
  }

  bool append_to_identifier(char c)
  {
    identifier_ += c;
    return true;
  }

  bool scan_character_literal()
  {
    return scanner_.match('\'') && scan_character_value(true) && scanner_.match('\'');
  }

  bool scan_character_value(bool allow_null)
  {
    character_literal_ = 0;

    if (scanner_.eoi()) {
      return false;
    }

    const char c = scanner_.peek();
    if (c == '\\') {
      return scanner_.match(c) && scan_character_escape() && character_literal_okay(allow_null);
    } else {
      character_literal_ = c;
      return scanner_.match(c) && character_literal_okay(allow_null);
    }
  }

  bool character_literal_okay(bool allow_null) {
    return allow_null || character_literal_ != 0;
  }

  bool scan_character_escape()
  {
    if (scanner_.eoi()) {
      return false;
    }

    const char c = scanner_.peek();
    switch (c) {
    case 'n':
      character_literal_ = '\n';
      return scanner_.match(c);
    case 't':
      character_literal_ = '\t';
      return scanner_.match(c);
    case 'v':
      character_literal_ = '\v';
      return scanner_.match(c);
    case 'b':
      character_literal_ = '\b';
      return scanner_.match(c);
    case 'r':
      character_literal_ = '\r';
      return scanner_.match(c);
    case 'f':
      character_literal_ = '\f';
      return scanner_.match(c);
    case 'a':
      character_literal_ = '\a';
      return scanner_.match(c);
    case '\\':
      character_literal_ = '\\';
      return scanner_.match(c);
    case '?':
      character_literal_ = '\?';
      return scanner_.match(c);
    case '\'':
      character_literal_ = '\'';
      return scanner_.match(c);
    case '"':
      character_literal_ = '\"';
      return scanner_.match(c);
    case '0':
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
      return scan_character_octal_escape();
    case 'x':
      return scanner_.match(c) && scan_character_hex_escape();
    default:
      return false;
    }
  }

  bool scan_character_octal_escape()
  {
    return scan_character_octal_digit() && scan_character_octal_optional_digit() && scan_character_octal_optional_digit();
  }

  bool scan_character_octal_digit()
  {
    if (scanner_.eoi()) {
      return false;
    }

    const char c = scanner_.peek();
    if (c >= '0' && c <= '7') {
      character_literal_ = (character_literal_ << 3) | ((c - '0') & 0x7);
      return scanner_.match(c);
    } else {
      return false;
    }
  }

  bool scan_character_octal_optional_digit()
  {
    if (scanner_.eoi()) {
      return true;
    }

    const char c = scanner_.peek();
    if (c >= '0' && c <= '7') {
      character_literal_ = (character_literal_ << 3) | ((c - '0') & 0x7);
      return scanner_.match(c);
    } else {
      return true;
    }
  }

  bool scan_character_hex_escape()
  {
    return scan_character_hex_digit() && scan_character_hex_optional_digit();
  }

  bool scan_character_hex_digit()
  {
    if (scanner_.eoi()) {
      return false;
    }

    const char c = scanner_.peek();
    if (c >= '0' && c <= '9') {
      character_literal_ = (character_literal_ << 4) | ((c - '0') & 0xF);
      return scanner_.match(c);
    } else if (c >= 'A' && c <= 'F') {
      character_literal_ = (character_literal_ << 4) | ((c - 'A' + 10) & 0xF);
      return scanner_.match(c);
    } else if (c >= 'a' && c <= 'f') {
      character_literal_ = (character_literal_ << 4) | ((c - 'a' + 10) & 0xF);
      return scanner_.match(c);
    } else {
      return false;
    }
  }

  bool scan_character_hex_optional_digit()
  {
    if (scanner_.eoi()) {
      return true;
    }

    const char c = scanner_.peek();
    if (c >= '0' && c <= '9') {
      character_literal_ = (character_literal_ << 4) | ((c - '0') & 0xF);
      return scanner_.match(c);
    } else if (c >= 'A' && c <= 'F') {
      character_literal_ = (character_literal_ << 4) | ((c - 'A' + 10) & 0xF);
      return scanner_.match(c);
    } else if (c >= 'a' && c <= 'f') {
      character_literal_ = (character_literal_ << 4) | ((c - 'a' + 10) & 0xF);
      return scanner_.match(c);
    } else {
      return true;
    }
  }

  bool scan_string_literal()
  {
    return scanner_.match('"') && scan_string_value(true) && scanner_.match('"');
  }

  bool scan_string_value(bool expect_quote)
  {
    string_literal_.clear();
    return scan_string_optional_characters(expect_quote);
  }

  bool scan_string_optional_characters(bool expect_quote)
  {
    if (scanner_.eoi()) {
      // Expecting ending quote.
      return !expect_quote;
    }

    const char c = scanner_.peek();
    if (c == '"') {
      return true;
    }

    return scan_character_value(false) && append_character_to_string() && scan_string_optional_characters(expect_quote);
  }

  bool append_character_to_string()
  {
    string_literal_ += character_literal_;
    return true;
  }

  IdlToken scan_numeric_literal()
  {
    signed_ = false;
    integer_value_ = 0;
    digits_after_decimal_ = 0;
    exponent_signed_ = false;
    exponent_value_ = 0;

    if (scanner_.eoi()) {
      return IdlToken::make_error();
    }

    const char c = scanner_.peek();
    if (c == '-') {
      scanner_.match(c);
      signed_ = true;
    }

    if (scanner_.eoi()) {
      return IdlToken::make_error();
    }

    const char d = scanner_.peek();
    if (d == '0') {
      scanner_.match(d);
      if (scanner_.eoi()) {
        // A single zero.
        return IdlToken::make_integer(false, 0);
      }

      const char e = scanner_.peek();
      if (e >= '0' && e <= '7') {
        // Octal
        if (scan_numeric_octal()) {
          return IdlToken::make_integer(signed_, integer_value_);
        } else {
          return IdlToken::make_error();
        }
      } else if (e == 'x') {
        // Hex.
        scanner_.match(e);
        if (scan_numeric_hex()) {
          return IdlToken::make_integer(signed_, integer_value_);
        } else {
          return IdlToken::make_error();
        }
      } else if (e == '.') {
        // Float.
        scanner_.match(e);
        if (scan_numeric_after_decimal(true)) {
          return make_float();
        } else {
          return IdlToken::make_error();
        }
      } else {
        return IdlToken::make_error();
      }
    } else if (d == '.') {
      if (scanner_.match(d) && scan_numeric_after_decimal(false)) {
        return make_float();
      } else {
        return IdlToken::make_error();
      }
    } else {
      // 1-9
      return scan_numeric_before_decimal();
    }
  }

  bool scan_numeric_octal()
  {
    return scan_numeric_octal_digit() && scan_numeric_octal_optional_digits();
  }

  bool scan_numeric_octal_digit()
  {
    if (scanner_.eoi()) {
      return false;
    }

    const char c = scanner_.peek();
    if (c >= '0' && c <= '7') {
      return scanner_.match(c) && scale_integer(8, c - '0');
    }

    return false;
  }

  bool scan_numeric_octal_optional_digits()
  {
    if (scanner_.eoi()) {
      return true;
    }

    const char c = scanner_.peek();
    if (c >= '0' && c <= '7') {
      return scanner_.match(c) && scale_integer(8, c - '0') && scan_numeric_octal_optional_digits();
    }

    return true;
  }

  bool scan_numeric_hex()
  {
    return scan_numeric_hex_digit() && scan_numeric_hex_optional_digits();
  }

  bool scan_numeric_hex_digit()
  {
    if (scanner_.eoi()) {
      return false;
    }

    const char c = scanner_.peek();
    if (c >= '0' && c <= '9') {
      return scanner_.match(c) && scale_integer(16, c - '0');
    } else if (c >= 'a' && c <= 'f') {
      return scanner_.match(c) && scale_integer(16, c - 'a' + 10);
    } else if (c >= 'A' && c <= 'F') {
      return scanner_.match(c) && scale_integer(16, c - 'A' + 10);
    }

    return false;
  }

  bool scan_numeric_hex_optional_digits()
  {
    if (scanner_.eoi()) {
      return true;
    }

    const char c = scanner_.peek();
    if (c >= '0' && c <= '9') {
      return scanner_.match(c) && scale_integer(16, c - '0') && scan_numeric_hex_optional_digits();
    } else if (c >= 'a' && c <= 'f') {
      return scanner_.match(c) && scale_integer(16, c - 'a' + 10) && scan_numeric_hex_optional_digits();
    } else if (c >= 'A' && c <= 'F') {
      return scanner_.match(c) && scale_integer(16, c - 'A' + 10) && scan_numeric_hex_optional_digits();
    }

    return true;
  }

  bool scan_numeric_after_decimal(bool digits_before_decimal)
  {
    if (digits_before_decimal) {
      return scan_numeric_fraction_optional_digits() && scan_numeric_optional_exponent();
    } else {
      return scan_numeric_fractional_digit() && scan_numeric_fraction_optional_digits() && scan_numeric_optional_exponent();
    }
  }

  bool scan_numeric_fractional_digit()
  {
    if (scanner_.eoi()) {
      return false;
    }

    const char c = scanner_.peek();
    if (c >= '0' && c <= '9') {
      ++digits_after_decimal_;
      return scanner_.match(c) && scale_integer(10, c - '0');
    } else {
      return false;
    }
  }

  bool scan_numeric_fraction_optional_digits()
  {
    if (scanner_.eoi()) {
      return true;
    }

    const char c = scanner_.peek();
    if (c >= '0' && c <= '9') {
      ++digits_after_decimal_;
      return scanner_.match(c) && scale_integer(10, c - '0') && scan_numeric_fraction_optional_digits();
    } else {
      return true;
    }
  }

  bool scan_numeric_optional_exponent()
  {
    if (scanner_.eoi()) {
      return true;
    }

    const char c = scanner_.peek();
    if (c == 'e' || c == 'E') {
      return scanner_.match(c) && scan_numeric_exponent();
    }

    return true;
  }

  bool scan_numeric_exponent()
  {
    if (scanner_.eoi()) {
      return false;
    }

    const char c = scanner_.peek();
    if (c == '-') {
      scanner_.match(c);
      exponent_signed_ = true;
    }

    return scan_numeric_exponent_digit() && scan_numeric_exponent_optional_digits();
  }

  bool scan_numeric_exponent_digit()
  {
    if (scanner_.eoi()) {
      return false;
    }

    const char c = scanner_.peek();
    if (c >= '0' && c <= '9') {
      return scanner_.match(c) && scale_exponent(c - '0');
    } else {
      return false;
    }
  }

  bool scan_numeric_exponent_optional_digits()
  {
    if (scanner_.eoi()) {
      return true;
    }

    const char c = scanner_.peek();
    if (c >= '0' && c <= '9') {
      return scanner_.match(c) && scale_exponent(c - '0') && scan_numeric_exponent_optional_digits();
    } else {
      return true;
    }
  }

  IdlToken scan_numeric_before_decimal()
  {
    if (scan_numeric_integer_digit() && scan_numeric_integer_optional_digits()) {
      if (scanner_.eoi()) {
        return IdlToken::make_integer(signed_, integer_value_);
      }

      const char c = scanner_.peek();
      if (c == '.') {
        if (scanner_.match(c) && scan_numeric_after_decimal(true)) {
          return make_float();
        } else {
          return IdlToken::make_error();
        }
      } else if (c == 'e' || c == 'E') {
        if (scanner_.match(c) && scan_numeric_exponent()) {
          return make_float();
        } else {
          return IdlToken::make_error();
        }
      } else {
        return IdlToken::make_integer(signed_, integer_value_);
      }
    } else {
      return IdlToken::make_error();
    }
  }

  bool scan_numeric_integer_digit()
  {
    if (scanner_.eoi()) {
      return false;
    }

    const char c = scanner_.peek();
    if (c >= '0' && c <= '9') {
      return scanner_.match(c) && scale_integer(10, c - '0');
    } else {
      return false;
    }
  }

  bool scan_numeric_integer_optional_digits()
  {
    if (scanner_.eoi()) {
      return true;
    }

    const char c = scanner_.peek();
    if (c >= '0' && c <= '9') {
      return scanner_.match(c) && scale_integer(10, c - '0') && scan_numeric_integer_optional_digits();
    } else {
      return true;
    }
  }

  IdlToken make_float()
  {
    while (digits_after_decimal_) {
      if (exponent_signed_) {
        ++exponent_value_;
        if (exponent_value_ == 0) {
          return IdlToken::make_error();
        }
      } else {
        if (exponent_value_ > 0) {
          --exponent_value_;
        } else {
          exponent_signed_ = true;
          exponent_value_ = 1;
        }
      }
      --digits_after_decimal_;
    }
    return IdlToken::make_float(signed_, integer_value_, exponent_signed_, exponent_value_);
  }

  bool scale_integer(ACE_UINT64 base, ACE_UINT64 value)
  {
    const ACE_UINT64 next = integer_value_ * base + value;
    if (integer_value_ && next < integer_value_) {
      return false;
    }
    integer_value_ = next;
    return true;
  }

  bool scale_exponent(ACE_UINT64 value)
  {
    const ACE_UINT64 next = exponent_value_ * 10 + value;
    if (exponent_value_ && next < exponent_value_) {
      return false;
    }
    exponent_value_ = next;
    return true;
  }

  CharacterScanner& scanner_;
  union {
    bool boolean_literal_;
    char character_literal_;
    struct {
      bool signed_;
      ACE_UINT64 integer_value_;
      ACE_UINT64 digits_after_decimal_;
      bool exponent_signed_;
      ACE_UINT64 exponent_value_; // TODO:  Handle overflow.
    };
  };
  std::string string_literal_;
  std::string identifier_;
};

} // namespace XTypes
} // namespace OpenDDS
OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif // OPENDDS_SAFETY_PROFILE
#endif // OPENDDS_DCPS_XTYPES_IDL_SCANNER_H
