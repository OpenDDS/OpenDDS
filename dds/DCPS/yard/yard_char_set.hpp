// Public domain, by Christopher Diggins
// http://www.cdiggins.com
//
// These are classes for defining set of characters at compile-time, and checking membership
// with maximum efficiency.

#ifndef YARD_CHAR_SET_HPP
#define YARD_CHAR_SET_HPP

namespace yard
{
  typedef unsigned char uchar;

  template<typename char_type>
  struct BasicCharSet
  {
    typedef char_type char_t;

    static const size_t size = 1 << (sizeof(char_type) * 8);

    BasicCharSet() {
      for (size_t i=0; i < size; i++) a[i] = false;
    }
    BasicCharSet(const char_t& x) {
      for (size_t i=0; i < size; i++) a[i] = false;
      a[x] = true;
    }
    BasicCharSet(const BasicCharSet& x) {
      for (size_t i=0; i < size; i++) a[i] = x[i];
    }
    BasicCharSet(const char_t& beg, const char_t& end) {
      for (size_t i=0; i < size; i++) a[i] = false;
      for (size_t i=beg; i < end; i++) a[i] = true;
    }
    BasicCharSet& operator|=(const BasicCharSet& x) {
      for (size_t i=0; i < size; i++) a[i] = a[i] || x[i];
      return *this;
    }
    BasicCharSet& operator|=(const char_t& x) {
      a[x] = true;
      return *this;
    }
    BasicCharSet& operator&=(const BasicCharSet& x) {
      for (size_t i=0; i < size; i++) a[i] = a[i] && x[i];
      return *this;
    }
    BasicCharSet& operator=(const BasicCharSet& x) {
      for (size_t i=0; i < size; i++) a[i] = x[i];
      return *this;
    }
    bool operator==(const BasicCharSet& x) {
      for (size_t i=0; i < size; i++) if (x[i] != a[i]) return false;
      return true;
    }
    bool operator!=(const BasicCharSet& x) {
      for (size_t i=0; i < size; i++) if (x[i] != a[i]) return true;
      return false;
    }
    BasicCharSet operator!() {
      BasicCharSet x;
      for (size_t i=0; i < size; i++) x.a[i] = !a[i];
      return x;
    }
    bool& operator[](const char_t& c) {
      return a[c];
    }
    const bool& operator[](const char_t& c) const {
      return a[c];
    }
    friend BasicCharSet operator&(const BasicCharSet& x,
      const BasicCharSet& y)
    {
      return BasicCharSet(x) &= y;
    }
    friend BasicCharSet operator|(const BasicCharSet& x,
      const BasicCharSet& y)
    {
      return BasicCharSet(x) |= y;
    }
    bool a[size];
  };

  typedef BasicCharSet<uchar> CharSetBase;

  template
  <
    uchar T0=0, uchar T1=0, uchar T2=0, uchar T3=0, uchar T4=0,
    uchar T5=0, uchar T6=0, uchar T7=0, uchar T8=0, uchar T9=0,
    uchar T10=0, uchar T11=0, uchar T12=0, uchar T13=0, uchar T14=0,
    uchar T15=0, uchar T16=0, uchar T17=0, uchar T18=0, uchar T19=0,
    uchar T20=0, uchar T21=0, uchar T22=0, uchar T23=0, uchar T24=0,
    uchar T25=0, uchar T26=0, uchar T27=0, uchar T28=0, uchar T29=0,
    uchar T30=0, uchar T31=0
  >
  struct CharSet : CharSetBase
  {
    CharSet() : CharSetBase() {
      a[T0]=true; a[T1]=true; a[T2]=true; a[T3]=true; a[T4]=true;
      a[T5]=true; a[T6]=true; a[T7]=true; a[T8]=true; a[T9]=true;
      a[T10]=true; a[T11]=true; a[T12]=true; a[T13]=true; a[T14]=true;
      a[T15]=true; a[T16]=true; a[T17]=true; a[T18]=true; a[T19]=true;
      a[T20]=true; a[T21]=true; a[T22]=true; a[T23]=true; a[T24]=true;
      a[T25]=true; a[T26]=true; a[T27]=true; a[T28]=true; a[T29]=true;
      a[T30]=true; a[T31]=true;
      a[0] = false; // ??
    }
  };

  struct EmptyCharSet : CharSet<>
  { };

  template
  <
    uchar T0, uchar T1
  >
  struct CharSetRange : CharSetBase
  {
    CharSetRange() : CharSetBase() {
      assert(T0 <= T1);
      for (size_t c=T0; c <= T1; c++) {
        a[c] = true;
      }
    }
  };

  template
  <
    typename T0,
    typename T1
  >
  struct CharSetUnion : CharSetBase
  {
    CharSetUnion() : CharSetBase() {
      const T0 x0;
      const T1 x1;
      for (size_t i=0; i<size; i++) {
        a[i] = x0[static_cast<uchar>(i)] || x1[static_cast<uchar>(i)];
      }
    }
  };

  template
  <
    typename T0, typename T1
  >
  struct CharSetIntersection : CharSetBase
  {
    CharSetIntersection() : CharSetBase() {
      const T0 x0;
      const T1 x1;
      for (size_t i=0; i<size; i++) {
        a[i] = x0[i] && x1[i];
      }
    }
  };

  template
  <
    typename T
  >
  struct CharSetNot : CharSetBase
  {
    CharSetNot() : CharSetBase() {
      const T x;
      for (size_t i=0; i<size; i++) {
        a[i] = !x[i];
      }
    }
  };

  typedef CharSetRange<'a', 'z'>
      LowerCaseLetterCharSet;

  typedef CharSetRange<'A', 'Z'>
      UpperCaseLetterCharSet;

  typedef CharSetUnion<LowerCaseLetterCharSet, UpperCaseLetterCharSet>
      LetterCharSet;

  typedef CharSetRange<'0', '9'>
      DigitCharSet;

  typedef CharSetRange<'0', '7'>
      OctDigitCharSet;

  typedef CharSetUnion<DigitCharSet,CharSetUnion<CharSetRange<'a', 'f'>, CharSetRange<'A', 'F'> > >
      HexDigitCharSet;

  typedef CharSetUnion<LetterCharSet, DigitCharSet>
      AlphaNumCharSet;

  typedef CharSetUnion<LetterCharSet, CharSet<'_'> >
      IdentFirstCharSet;

  typedef CharSetUnion<LetterCharSet, CharSet<'\''> >
      WordLetterCharSet;

  typedef CharSetUnion<IdentFirstCharSet, DigitCharSet>
      IdentNextCharSet;

  typedef CharSet<' ','\t','\n','\r'>
      WhiteSpaceCharSet;
}

#endif // #ifndef YARD_CHAR_SET_HPP

