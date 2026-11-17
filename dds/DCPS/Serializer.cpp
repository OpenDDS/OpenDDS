/*
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include <DCPS/DdsDcps_pch.h> // Only the _pch include should start with DCPS/

#include "Serializer.h"

#if !defined (__ACE_INLINE__)
# include "Serializer.inl"
#endif /* !__ACE_INLINE__ */

#include "SafetyProfileStreams.h"

#ifndef OPENDDS_UTIL_BUILD
#  include <tao/String_Alloc.h>
#endif

#include <ace/OS_NS_string.h>
#include <ace/OS_Memory.h>
#include <ace/Log_Msg.h>

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <limits>

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

#ifdef OPENDDS_UTIL_BUILD
namespace {

ACE_CDR::Char* string_alloc(ACE_CDR::ULong size)
{
  return static_cast<ACE_CDR::Char*>(std::malloc(size + 1));
}

void string_free(ACE_CDR::Char* ptr)
{
  std::free(ptr);
}

ACE_CDR::WChar* wstring_alloc(ACE_CDR::ULong size)
{
  return static_cast<ACE_CDR::WChar*>(std::malloc(sizeof(ACE_CDR::WChar) * (size + 1)));
}

void wstring_free(ACE_CDR::WChar* ptr)
{
  std::free(ptr);
}

}
#endif

namespace {

const ACE_UINT64 sign_bit = ACE_UINT64_LITERAL(1) << 63;
const ACE_UINT64 low48 = (ACE_UINT64_LITERAL(1) << 48) - 1;
const ACE_UINT64 low52 = (ACE_UINT64_LITERAL(1) << 52) - 1;
const ACE_UINT64 double_exp_mask = ACE_UINT64_LITERAL(0x7ff) << 52;

// binary128 as two host-order 64-bit halves: sign, exponent, and 48 fraction bits, then 64 more
void store_binary128(ACE_UINT64 hi, ACE_UINT64 lo, char* bytes)
{
#ifdef ACE_BIG_ENDIAN
  std::memcpy(bytes, &hi, 8);
  std::memcpy(bytes + 8, &lo, 8);
#else
  std::memcpy(bytes, &lo, 8);
  std::memcpy(bytes + 8, &hi, 8);
#endif
}

void load_binary128(const char* bytes, ACE_UINT64& hi, ACE_UINT64& lo)
{
#ifdef ACE_BIG_ENDIAN
  std::memcpy(&hi, bytes, 8);
  std::memcpy(&lo, bytes + 8, 8);
#else
  std::memcpy(&lo, bytes, 8);
  std::memcpy(&hi, bytes + 8, 8);
#endif
}

struct U128 {
  ACE_UINT64 hi, lo;
  U128(ACE_UINT64 h, ACE_UINT64 l) : hi(h), lo(l) {}
  bool operator==(const U128& o) const { return hi == o.hi && lo == o.lo; }
  bool operator<(const U128& o) const { return hi < o.hi || (hi == o.hi && lo < o.lo); }
};

int top_bit(ACE_UINT64 x)
{
  int n = -1;
  for (; x; x >>= 1) {
    ++n;
  }
  return n;
}

int top_bit(const U128& x)
{
  return x.hi ? 64 + top_bit(x.hi) : top_bit(x.lo);
}

U128 shl(const U128& x, int n)
{
  if (n >= 64) {
    return U128(x.lo << (n - 64), 0);
  }
  return n ? U128((x.hi << n) | (x.lo >> (64 - n)), x.lo << n) : x;
}

U128 shr(const U128& x, int n)
{
  if (n >= 128) {
    return U128(0, 0);
  }
  if (n >= 64) {
    return U128(0, x.hi >> (n - 64));
  }
  return n ? U128(x.hi >> n, (x.lo >> n) | (x.hi << (64 - n))) : x;
}

U128 low_bits(const U128& x, int n)
{
  if (n >= 128) {
    return x;
  }
  return n ? shr(shl(x, 128 - n), 128 - n) : U128(0, 0);
}

U128 add(const U128& a, const U128& b)
{
  const ACE_UINT64 lo = a.lo + b.lo;
  return U128(a.hi + b.hi + (lo < a.lo), lo);
}

U128 sub(const U128& a, const U128& b)
{
  return U128(a.hi - b.hi - (a.lo < b.lo), a.lo - b.lo);
}

ACE_UINT64 double_bits(double d)
{
  ACE_UINT64 bits;
  std::memcpy(&bits, &d, 8);
  return bits;
}

double bits_double(ACE_UINT64 bits)
{
  double d;
  std::memcpy(&d, &bits, 8);
  return d;
}

// A finite double as (-1)^neg * m * 2^scale
struct Finite {
  bool neg;
  int scale;
  ACE_UINT64 m;
  explicit Finite(double d)
  {
    const ACE_UINT64 bits = double_bits(d);
    const int exp = static_cast<int>((bits >> 52) & 0x7ff);
    neg = (bits >> 63) != 0;
    m = bits & low52;
    if (exp) {
      m |= ACE_UINT64_LITERAL(1) << 52;
    }
    scale = (exp ? exp : 1) - 1075;
  }
};

// Round m * 2^scale to the nearest double (ties to even), setting rem to what's left over
double round_to_double(const U128& m, int scale, U128& rem, bool& rem_neg)
{
  rem = U128(0, 0);
  rem_neg = false;
  const int p = top_bit(m);
  if (p < 0) {
    return 0.0;
  }
  // Significant bits available: 53, or fewer for subnormals
  const int e = scale + p;
  const int n = e < -1022 ? 53 - (-1022 - e) : 53;
  const int k = p + 1 - n;
  if (k <= 0) {
    return std::ldexp(static_cast<double>(m.lo), scale);
  }
  U128 q = shr(m, k);
  const U128 dropped = low_bits(m, k);
  if (k <= 128) {
    const U128 half = shl(U128(0, 1), k - 1);
    if (half < dropped || (dropped == half && (q.lo & 1))) {
      q = add(q, U128(0, 1));
      rem = sub(shl(U128(0, 1), k), dropped);
      rem_neg = true;
      return std::ldexp(static_cast<double>(q.lo), scale + k);
    }
  }
  rem = dropped;
  return std::ldexp(static_cast<double>(q.lo), scale + k);
}

#ifdef OPENDDS_X87_LONG_DOUBLE
const ACE_UINT64 x87_integer_bit = ACE_UINT64_LITERAL(1) << 63;
const ACE_UINT64 x87_quiet_bit = ACE_UINT64_LITERAL(1) << 62;

// x87 extended precision and binary128 share the sign, exponent, and bias;
// x87 has an explicit integer bit, set for normal numbers, infinity, and NaN.
void x87_to_binary128(const ACE_CDR::LongDouble& value, char* bytes)
{
  const unsigned char* const src = reinterpret_cast<const unsigned char*>(&value);
  ACE_UINT64 sig;
  std::memcpy(&sig, src, 8);
  const ACE_UINT64 sign = src[9] >> 7;
  ACE_UINT64 exp = src[8] | ((src[9] & 0x7f) << 8);
  if (exp == 0 && (sig & x87_integer_bit)) {
    exp = 1; // Pseudo-denormal, same value as with the minimum normal exponent
  } else if (exp != 0 && !(sig & x87_integer_bit)) {
    // Unnormal, pseudo-infinity, or pseudo-NaN: x87 treats these as invalid
    exp = 0x7fff;
    sig = x87_quiet_bit;
  }
  const ACE_UINT64 frac = sig & ~x87_integer_bit;
  store_binary128((sign << 63) | (exp << 48) | (frac >> 15), frac << 49, bytes);
}

void binary128_to_x87(const char* bytes, ACE_CDR::LongDouble& value)
{
  ACE_UINT64 hi, lo;
  load_binary128(bytes, hi, lo);
  const unsigned int sign = static_cast<unsigned int>(hi >> 63);
  unsigned int exp = static_cast<unsigned int>(hi >> 48) & 0x7fff;
  // The top 63 of the 112 fraction bits, and the 49 below them
  ACE_UINT64 sig = ((hi & low48) << 15) | (lo >> 49);
  const ACE_UINT64 rest = lo & ((ACE_UINT64_LITERAL(1) << 49) - 1);
  const ACE_UINT64 half = ACE_UINT64_LITERAL(1) << 48;

  if (exp == 0x7fff) {
    // Inf/NaN: truncate the payload and quiet any NaN
    if (sig || rest) {
      sig |= x87_quiet_bit;
    }
    sig |= x87_integer_bit;
  } else {
    // Round to x87's 63 fraction bits, ties to even
    if (rest > half || (rest == half && (sig & 1))) {
      ++sig;
    }
    if (sig == x87_integer_bit) {
      ++exp; // Rounding carried into the exponent
    } else if (exp != 0) {
      sig |= x87_integer_bit;
    }
  }

  unsigned char* const dst = reinterpret_cast<unsigned char*>(&value);
  std::memset(dst, 0, sizeof value);
  std::memcpy(dst, &sig, 8);
  dst[8] = static_cast<unsigned char>(exp & 0xff);
  dst[9] = static_cast<unsigned char>((sign << 7) | ((exp >> 8) & 0x7f));
}
#endif

}

void double_double_to_binary128(double hi, double lo, char* bytes)
{
  // Inf or NaN in either part is the result, with the payload's top 52 bits
  ACE_UINT64 special = double_bits(hi);
  if ((special & double_exp_mask) != double_exp_mask) {
    special = double_bits(lo);
  }
  if ((special & double_exp_mask) == double_exp_mask) {
    const ACE_UINT64 frac = special & low52;
    store_binary128((special & sign_bit) | (ACE_UINT64_LITERAL(0x7fff) << 48) | (frac >> 4),
                    frac << 60, bytes);
    return;
  }

  // Add the exact parts with the larger one's leading bit at bit 125 of r,
  // which leaves room for a carry
  Finite a(hi), b(lo);
  if (!a.m && !b.m) {
    store_binary128(a.neg ? sign_bit : 0, 0, bytes);
    return;
  }
  if (!a.m || (b.m && b.scale + top_bit(b.m) > a.scale + top_bit(a.m))) {
    std::swap(a, b);
  }
  const int shift_a = 125 - top_bit(a.m);
  U128 r = shl(U128(0, a.m), shift_a);
  int scale = a.scale - shift_a;
  bool sticky = false;
  if (b.m) {
    U128 rb(0, b.m);
    const int shift_b = b.scale - scale;
    if (shift_b >= 0) {
      rb = shl(rb, shift_b);
    } else {
      sticky = !(low_bits(rb, -shift_b) == U128(0, 0));
      rb = shr(rb, -shift_b);
    }
    if (a.neg == b.neg) {
      r = add(r, rb);
    } else if (rb < r) {
      // b was truncated toward zero, so a sticky b takes one more from r
      r = sub(sub(r, rb), U128(0, sticky));
    } else {
      // Only possible when b wasn't truncated
      r = sub(rb, r);
      a.neg = b.neg;
    }
  }
  if (r == U128(0, 0)) {
    store_binary128(0, 0, bytes);
    return;
  }

  // Normalize to 113 significant bits, rounding to nearest, ties to even
  const int p = top_bit(r);
  if (p > 112) {
    const int k = p - 112;
    const U128 dropped = low_bits(r, k);
    const U128 half = shl(U128(0, 1), k - 1);
    r = shr(r, k);
    scale += k;
    if (half < dropped || (dropped == half && (sticky || (r.lo & 1)))) {
      r = add(r, U128(0, 1));
      if (top_bit(r) > 112) {
        r = shr(r, 1);
        ++scale;
      }
    }
  } else {
    r = shl(r, 112 - p);
    scale -= 112 - p;
  }
  const ACE_UINT64 exp = static_cast<ACE_UINT64>(scale + 112 + 16383);
  store_binary128((a.neg ? sign_bit : 0) | (exp << 48) | (r.hi & low48), r.lo, bytes);
}

void binary128_to_double_double(const char* bytes, double& hi, double& lo)
{
  ACE_UINT64 bhi, blo;
  load_binary128(bytes, bhi, blo);
  const bool neg = (bhi >> 63) != 0;
  const int exp = static_cast<int>(bhi >> 48) & 0x7fff;
  U128 m(bhi & low48, blo);
  lo = 0.0;

  if (exp == 0x7fff) {
    // Inf/NaN: truncate the payload and quiet any NaN
    ACE_UINT64 bits = (neg ? sign_bit : 0) | (ACE_UINT64_LITERAL(0x7ff) << 52);
    if (!(m == U128(0, 0))) {
      bits |= (ACE_UINT64_LITERAL(1) << 51) | (m.hi << 4) | (m.lo >> 60);
    }
    hi = bits_double(bits);
    return;
  }

  // hi is the nearest double and lo is the nearest double to the rest
  if (exp) {
    m.hi |= ACE_UINT64_LITERAL(1) << 48;
  }
  const int scale = (exp ? exp : 1) - 16383 - 112;
  U128 rem(0, 0);
  bool rem_neg = false;
  hi = round_to_double(m, scale, rem, rem_neg);
  if ((double_bits(hi) & double_exp_mask) != double_exp_mask) {
    U128 unused(0, 0);
    bool unused_neg = false;
    lo = round_to_double(rem, scale, unused, unused_neg);
    if (rem_neg) {
      lo = -lo;
    }
  }
  if (neg) {
    hi = -hi;
    lo = -lo;
  }
}

void longdouble_to_binary128(const ACE_CDR::LongDouble& value, char* bytes)
{
#if defined OPENDDS_X87_LONG_DOUBLE
  x87_to_binary128(value, bytes);
#elif defined OPENDDS_DOUBLE_DOUBLE_LONG_DOUBLE
  double parts[2];
  std::memcpy(parts, &value, sizeof parts);
  double_double_to_binary128(parts[0], parts[1], bytes);
#else
  std::memcpy(bytes, &value, float128_cdr_size);
#endif
}

void binary128_to_longdouble(const char* bytes, ACE_CDR::LongDouble& value)
{
#if defined OPENDDS_X87_LONG_DOUBLE
  binary128_to_x87(bytes, value);
#elif defined OPENDDS_DOUBLE_DOUBLE_LONG_DOUBLE
  double parts[2];
  binary128_to_double_double(bytes, parts[0], parts[1]);
  std::memcpy(&value, parts, sizeof parts);
#else
  std::memcpy(&value, bytes, float128_cdr_size);
#endif
}

String endianness_to_string(Endianness endianness)
{
  switch (endianness) {
  case ENDIAN_BIG:
    return "big-endian ("
#ifdef ACE_LITTLE_ENDIAN
      "non-"
#endif
      "native)";
  case ENDIAN_LITTLE:
    return "little-endian ("
#ifndef ACE_LITTLE_ENDIAN
      "non-"
#endif
      "native)";
  default:
    return "invalid endianness";
  }
}

Encoding::Encoding()
  : endianness_(ENDIAN_NATIVE)
  , skip_sequence_dheader_(false)
{
  kind(KIND_XCDR1);
}

Encoding::Encoding(Kind k, Endianness endianness)
  : endianness_(endianness)
  , skip_sequence_dheader_(false)
{
  kind(k);
}

Encoding::Encoding(Kind k, bool swap_bytes)
  : endianness_(swap_bytes ? ENDIAN_NONNATIVE : ENDIAN_NATIVE)
  , skip_sequence_dheader_(false)
{
  kind(k);
}

String Encoding::kind_to_string(Kind value)
{
  switch (value) {
  case KIND_XCDR1:
    return "CDR/XCDR1";
  case KIND_XCDR2:
    return "XCDR2";
  case KIND_UNALIGNED_CDR:
    return "Unaligned CDR";
  default:
    return "Unknown: 0x" + to_dds_string(static_cast<unsigned>(value), true);
  }
}

String Encoding::to_string() const
{
  String rv = Encoding::kind_to_string(kind_) + ", " +
    endianness_to_string(endianness_);
  if (!zero_init_padding_) {
    rv += ", non-initialized padding";
  }
  return rv;
}

const char Serializer::ALIGN_PAD[] = {0};

Serializer::Serializer(ACE_Message_Block* chain, const Encoding& enc)
  : current_(chain)
  , good_bit_(true)
  , construction_status_(ConstructionSuccessful)
  , align_rshift_(0)
  , align_wshift_(0)
  , rpos_(0)
  , read_limit_((std::numeric_limits<size_t>::max)())
  , wpos_(0)
{
  encoding(enc);
  reset_alignment();
}

Serializer::Serializer(ACE_Message_Block* chain, Encoding::Kind kind,
  Endianness endianness)
  : current_(chain)
  , good_bit_(true)
  , construction_status_(ConstructionSuccessful)
  , align_rshift_(0)
  , align_wshift_(0)
  , rpos_(0)
  , read_limit_((std::numeric_limits<size_t>::max)())
  , wpos_(0)
{
  encoding(Encoding(kind, endianness));
  reset_alignment();
}

Serializer::Serializer(ACE_Message_Block* chain,
  Encoding::Kind kind, bool swap_bytes)
  : current_(chain)
  , good_bit_(true)
  , construction_status_(ConstructionSuccessful)
  , align_rshift_(0)
  , align_wshift_(0)
  , rpos_(0)
  , read_limit_((std::numeric_limits<size_t>::max)())
  , wpos_(0)
{
  encoding(Encoding(kind, swap_bytes));
  reset_alignment();
}

Serializer::~Serializer()
{
}

Serializer::ScopedReadLimit::ScopedReadLimit(Serializer& ser, size_t size,
                                             bool enabled, bool skip_remainder)
  : ser_(ser)
  , previous_limit_(ser.read_limit_)
  , end_(ser.rpos_)
  , enabled_(enabled)
  , skip_remainder_(skip_remainder)
  , valid_(false)
  , finished_(false)
{
  if (!enabled_) {
    valid_ = ser.good_bit_;
    return;
  }
  install(size);
}

Serializer::ScopedReadLimit::ScopedReadLimit(Serializer& ser)
  : ser_(ser)
  , previous_limit_(ser.read_limit_)
  , end_(ser.rpos_)
  , enabled_(true)
  , skip_remainder_(true)
  , valid_(false)
  , finished_(false)
{
  size_t size = 0;
  if (ser.read_delimiter(size)) {
    install(size);
  }
}

void Serializer::ScopedReadLimit::install(size_t size)
{
  const size_t max = (std::numeric_limits<size_t>::max)();
  if (ser_.good_bit_ && size <= ser_.length() && size <= max - ser_.rpos_) {
    end_ = ser_.rpos_ + size;
    ser_.read_limit_ = end_;
    valid_ = true;
  } else {
    ser_.good_bit_ = false;
  }
}

Serializer::ScopedReadLimit::~ScopedReadLimit()
{
  if (!finished_ && enabled_ && skip_remainder_ && valid_ && ser_.good_bit_) {
    skip_to_end();
  }
  ser_.read_limit_ = previous_limit_;
}

size_t Serializer::ScopedReadLimit::remaining() const
{
  return enabled_ && valid_ && ser_.rpos_ <= end_ ? end_ - ser_.rpos_ : 0;
}

bool Serializer::ScopedReadLimit::skip_to_end()
{
  return enabled_ && valid_ && ser_.good_bit_ && ser_.skip(remaining());
}

bool Serializer::ScopedReadLimit::finish()
{
  if (finished_) {
    return valid_ && ser_.good_bit_;
  }
  const bool result = valid_ && ser_.good_bit_ &&
    (!enabled_ || !skip_remainder_ || skip_to_end());
  ser_.read_limit_ = previous_limit_;
  finished_ = true;
  return result;
}

bool Serializer::ScopedReadLimit::reset(size_t size)
{
  if (finished_) {
    return false;
  }
  ser_.read_limit_ = previous_limit_;
  enabled_ = true;
  end_ = ser_.rpos_;
  valid_ = false;
  install(size);
  return valid_;
}

Serializer::ScopedAlignmentContext::ScopedAlignmentContext(Serializer& ser, size_t min_read)
  : ser_(ser)
  , max_align_(ser.encoding().max_align())
  , start_rpos_(ser.rpos())
  , rblock_((max_align_ && ser.current_) ? (reinterpret_cast<size_t>(ser.current_->rd_ptr()) - ser.align_rshift_) % max_align_ : 0)
  , min_read_(min_read)
  , start_wpos_(ser.wpos())
  , wblock_((max_align_ && ser.current_) ? (reinterpret_cast<size_t>(ser.current_->wr_ptr()) - ser.align_wshift_) % max_align_ : 0)
{
  ser_.reset_alignment();
}

void
Serializer::ScopedAlignmentContext::restore(Serializer& ser) const
{
  if (min_read_ != 0 && (ser.rpos() - start_rpos_) < min_read_) {
    ser.skip(min_read_ - (ser.rpos() - start_rpos_));
  }

  if (ser.current_ && max_align_) {
    ser.align_rshift_ = offset(ser.current_->rd_ptr(), ser.rpos() - start_rpos_ + rblock_, max_align_);
    ser.align_wshift_ = offset(ser.current_->wr_ptr(), ser.wpos() - start_wpos_ + wblock_, max_align_);
  }
}

bool
Serializer::peek(ACE_CDR::ULong& t)
{
  // save
  const size_t rpos = rpos_;
  const unsigned char align_rshift = align_rshift_;
  ACE_Message_Block* const current = current_;

  // read
  if (!peek_helper(current_, 2 * uint32_cdr_size, t)) {
    return false;
  }

  // reset
  current_ = current;
  align_rshift_ = align_rshift;
  rpos_ = rpos;
  return true;
}

void
Serializer::reset_alignment()
{
  const size_t align = encoding().max_align();
  if (current_ && align) {
    align_rshift_ = offset(current_->rd_ptr(), 0, align);
    align_wshift_ = offset(current_->wr_ptr(), 0, align);
  }
}

void
Serializer::smemcpy(char* to, const char* from, size_t n)
{
  OPENDDS_ASSERT(from);
  (void) ACE_OS::memcpy(to, from, n);
}

void
Serializer::swapcpy(char* to, const char* from, size_t n)
{
  // Unroll the loop...
  switch (n) {               // 2   4   8   16
  case 16:
    to[ 15] = from[ n - 16]; // x   x   x    0
    // fallthrough
  case 15:
    to[ 14] = from[ n - 15]; // x   x   x    1
    // fallthrough
  case 14:
    to[ 13] = from[ n - 14]; // x   x   x    2
    // fallthrough
  case 13:
    to[ 12] = from[ n - 13]; // x   x   x    3
    // fallthrough
  case 12:
    to[ 11] = from[ n - 12]; // x   x   x    4
    // fallthrough
  case 11:
    to[ 10] = from[ n - 11]; // x   x   x    5
    // fallthrough
  case 10:
    to[  9] = from[ n - 10]; // x   x   x    6
    // fallthrough
  case  9:
    to[  8] = from[ n -  9]; // x   x   x    7
    // fallthrough
  case  8:
    to[  7] = from[ n -  8]; // x   x   0    8
    // fallthrough
  case  7:
    to[  6] = from[ n -  7]; // x   x   1    9
    // fallthrough
  case  6:
    to[  5] = from[ n -  6]; // x   x   2   10
    // fallthrough
  case  5:
    to[  4] = from[ n -  5]; // x   x   3   11
    // fallthrough
  case  4:
    to[  3] = from[ n -  4]; // x   0   4   12
    // fallthrough
  case  3:
    to[  2] = from[ n -  3]; // x   1   5   13
    // fallthrough
  case  2:
    to[  1] = from[ n -  2]; // 0   2   6   14
    // fallthrough
  case  1:
    to[  0] = from[ n -  1]; // 1   3   7   15
    // fallthrough
  case  0:
    return;
  default:
    good_bit_ = false;
  }
}

size_t
Serializer::read_string(ACE_CDR::Char*& dest,
                        StrAllocate str_alloc,
                        StrFree str_free)
{
  if (str_alloc == 0) {
#ifdef OPENDDS_UTIL_BUILD
    str_alloc = string_alloc;
#else
    str_alloc = CORBA::string_alloc;
#endif
  }

  //
  // Ensure no bad values leave the routine.
  //
  free_string(dest, str_free);
  dest = 0;

  //
  // Extract the string length.
  //
  ACE_CDR::ULong length; // includes the null
  if (!(*this >> length)) {
    return 0;
  }

  if (length == 0) {
    // not legal CDR, but we need to accept it since other implementations may generate this
    dest = str_alloc(0);
    return 0;
  }

  //
  // NOTE: Maintain the ACE implementation where the length check is
  //       done here before the allocation even though it will be
  //       checked during the actual read as well.  Bound by length()
  //       (not total_length()) so an oversized length inside a
  //       read-limited region is rejected before allocating.
  //
  if (current_ && length <= this->length()) {

    dest = str_alloc(length - 1);

    if (dest == 0) {
      good_bit_ = false;

    } else {
      //
      // Extract the string.
      //
      read_char_array(dest, length);

      if (good_bit_ && dest[length - 1]) {
        // If the last byte was not a 0 it's not a valid CDR string
        good_bit_ = false;
      }
    }

    if (!good_bit_) {
      free_string(dest, str_free);
      dest = 0;
    }

  } else {
    good_bit_ = false;
  }

  return length - 1;
}

void
Serializer::free_string(ACE_CDR::Char* str,
                        StrFree str_free)
{
  if (str_free == 0) {
#ifdef OPENDDS_UTIL_BUILD
    str_free = string_free;
#else
    str_free = CORBA::string_free;
#endif
  }
  str_free(str);
}

size_t
Serializer::read_string(ACE_CDR::WChar*& dest,
                        WStrAllocate str_alloc,
                        WStrFree str_free)
{
  if (str_alloc == 0) {
#ifdef OPENDDS_UTIL_BUILD
    str_alloc = wstring_alloc;
#else
    str_alloc = CORBA::wstring_alloc;
#endif
  }

  //
  // Ensure no bad values leave the routine.
  //
  free_string(dest, str_free);
  dest = 0;

  //
  // Extract the string size.
  //
  ACE_CDR::ULong bytecount;
  if (!(*this >> bytecount)) {
    return 0;
  }

  //
  // NOTE: Maintain the ACE implementation where the length check is
  //       done here before the allocation even though it will be
  //       checked during the actual read as well.
  //
  ACE_CDR::ULong length = 0;
  if (current_ && bytecount <= this->length()) {
    if (bytecount % char16_cdr_size) {
      good_bit_ = false;
      return 0;
    }
    length = bytecount / char16_cdr_size;
    dest = str_alloc(length);

    if (dest == 0) {
      good_bit_ = false;
      return 0;
    }

#if ACE_SIZEOF_WCHAR == 2
    read_array(reinterpret_cast<char*>(dest), char16_cdr_size, length, swap_bytes());
#else
    for (size_t i = 0; i < length && good_bit_; ++i) {
      ACE_UINT16 as_utf16;
      buffer_read(reinterpret_cast<char*>(&as_utf16), char16_cdr_size, swap_bytes());
      if (good_bit_) {
        dest[i] = as_utf16;
      }
    }
#endif

    if (good_bit_) {
      //
      // Null terminate the string.
      //
      dest[length] = L'\0';
    } else {
      free_string(dest, str_free);
      dest = 0;
      length = 0;
    }

  } else {
    good_bit_ = false;
  }

  return length;
}

void
Serializer::free_string(ACE_CDR::WChar* str,
                        WStrFree str_free)
{
  if (str_free == 0) {
#ifdef OPENDDS_UTIL_BUILD
    str_free = wstring_free;
#else
    str_free = CORBA::wstring_free;
#endif
  }
  str_free(str);
}

ACE_Message_Block* Serializer::trim(size_t n) const
{
  if (!good_bit() || !current_ || n > length()) {
    return 0;
  }
  Message_Block_Ptr dup(current_->duplicate());
  ACE_Message_Block* i = dup.get();
  for (size_t remain = n; i && remain; i = i->cont()) {
    if (i->length() >= remain) {
      i->wr_ptr(i->rd_ptr() + remain);
      ACE_Message_Block::release(i->cont());
      i->cont(0);
      break;
    }
    remain -= i->length();
  }
  return dup.release();
}

bool Serializer::read_parameter_id(unsigned& id, size_t& size, bool& must_understand)
{
  id = 0;
  size = 0;
  must_understand = false;
  const Encoding::XcdrVersion xcdr = encoding().xcdr_version();
  if (xcdr == Encoding::XCDR_VERSION_1) {
    // Get the "short" id and size
    if (!align_r(xcdr1_pid_alignment)) {
      return false;
    }
    ACE_CDR::UShort pid;
    if (!(*this >> pid)) {
      return false;
    }
    const ACE_CDR::UShort short_id = pid & 0x3fff;
    ACE_CDR::UShort short_size;
    if (!(*this >> short_size)) {
      return false;
    }

    // If extended, get the "long" id and size
    if (short_id == pid_extended) {
      if (short_size < 8u) {
        good_bit_ = false;
        return false;
      }
      ACE_CDR::ULong emheader, long_size;
      if (!(*this >> emheader) || !(*this >> long_size)) {
        return false;
      }
      const unsigned short_size_left = short_size - 8u;
      if (short_size_left && !skip(short_size_left)) {
        return false;
      }
      id = emheader & MEMBER_ID_MASK;
      size = long_size;
      must_understand = emheader & xcdr1_emheader_must_understand;
    } else {
      id = short_id;
      size = short_size;
      must_understand = pid & pid_must_understand;
    }

    reset_alignment();
  } else if (xcdr == Encoding::XCDR_VERSION_2) {
    ACE_CDR::ULong emheader;
    if (!(*this >> emheader)) {
      return false;
    }

    must_understand = emheader & xcdr2_emheader_must_understand;

    // Get Size
    const unsigned short lc = (emheader >> 28) & 0x7;
    if (lc < 4) {
      size = size_t(1) << lc;
    } else {
      ACE_CDR::ULong next_int;
      if (lc == 4 ? !(*this >> next_int) : !peek(next_int)) {
        return false;
      }
      switch (lc) {
      case 4:
        size = next_int;
        break;
      case 5:
      case 6:
      case 7:
        {
          const size_t multiplier = lc == 5 ? 1u : lc == 6 ? 4u : 8u;
          const size_t next = next_int;
          const size_t max = (std::numeric_limits<size_t>::max)();
          if (next > (max - uint32_cdr_size) / multiplier) {
            good_bit_ = false;
            return false;
          }
          size = uint32_cdr_size + next * multiplier;
        }
        break;
      }
    }
    id = emheader & 0xfffffff;
  }

  return true;
}

bool Serializer::write_parameter_id(const unsigned id, const size_t size, const bool must_understand)
{
  if (static_cast<ACE_CDR::ULong>(id) > MEMBER_ID_MAX) {
    return false;
  }

  const Encoding::XcdrVersion xcdr = encoding().xcdr_version();
  if (xcdr == Encoding::XCDR_VERSION_1) {
    if (!align_w(xcdr1_pid_alignment)) {
      return false;
    }

    // Determine if we need to use a short or long PID
    const bool long_pid = id >= (1 << 14) || size >= (1 << 16);

    // Write the short part of the PID.  PID_EXTENDED is always marked as must
    // understand so a reader that doesn't support extended PIDs rejects it.
    const ACE_CDR::UShort pid_id =
      (long_pid || must_understand ? pid_must_understand : 0) +
      (long_pid ? pid_extended : static_cast<ACE_CDR::UShort>(id));
    if (!(*this << pid_id)) {
      return false;
    }
    const ACE_CDR::UShort pid_size = long_pid ? 8 : ACE_CDR::UShort(size);
    if (!(*this << pid_size)) {
      return false;
    }

    // If PID is long, write the extended/long part.
    if (long_pid && (
          !(*this << static_cast<ACE_CDR::ULong>(id |
            (must_understand ? xcdr1_emheader_must_understand : 0))) ||
          !(*this << static_cast<ACE_CDR::ULong>(size)))) {
      return false;
    }

    reset_alignment();
  } else if (xcdr == Encoding::XCDR_VERSION_2) {
    // Compute Length Code, write EM Header and NEXTINT
    const ACE_CDR::ULong lc = (size == 1 ? 0 : size == 2 ? 1 : size == 4 ? 2 : size == 8 ? 3 : 4);
    const ACE_CDR::ULong emheader =
      (lc << 28) | id | (must_understand ? xcdr2_emheader_must_understand : 0);
    if (!(*this << emheader)) {
      return false;
    }
    if (lc == 4) {
      return *this << ACE_CDR::ULong(size);
    }
  }
  return true;
}

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL
