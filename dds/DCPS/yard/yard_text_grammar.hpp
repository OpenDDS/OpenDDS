// Public domain, by Christopher Diggins
// http://www.cdiggins.com
//
// This file defines the core YARD grammar rules for ASCII text parsing.
// Each type in this file represents a separate PEG grammar production
// (parsing rule).

#ifndef YARD_TEXT_GRAMMAR_HPP
#define YARD_TEXT_GRAMMAR_HPP

namespace yard
{
namespace text_grammar
{
        // accepts a single char, returns false only if at the end of the file
        struct AnyChar
        {
                template<typename ParserState_T>
                static bool Match(ParserState_T& p) {
                        if (p.AtEnd()) {
                                return false;
                        };
                        p.GotoNext();
                        return true;
                }
        };

        // accepts a single char as specified by the template parameter
        template<char C>
        struct Char
        {
            template<typename ParserState_T>
                static bool Match(ParserState_T& p) {
                        if (p.AtEnd()) { return false; }
                        if (p.GetElem() == C) { p.GotoNext(); return true; }
                        return false;
                }
        };

        // consume a single character but prints an error message and throws an exception,
        // if the character is not found in the input stream.
        template<char C>
        struct ExpectChar {
                template<typename ParserState_T>
                static bool Match(ParserState_T& p) {
                        if (p.AtEnd() || p.GetElem() != C)
                        {
                                printf("expected character: %c\n", C);
                                throw 0;
                        }
                        p.GotoNext();
                        return true;
                }
        };

  // accepts anything except a specific Char
  // equivalent to Seq<Not<Char<C>, AnyChar> > but more efficient
  template<char C>
  struct NotChar
  {
    template<typename ParserState_T>
    static bool Match(ParserState_T& p) {
      if (p.AtEnd()) { return false; }
      if (p.GetElem() != C) { p.GotoNext(); return true; }
      return false;
    }
  };

   // CharSetParser matches a single character in the character set
        template<typename CharSet_T>
        struct CharSetParser
        {
                static const CharSet_T cs;

                template<typename ParserState_T>
                static bool Match(ParserState_T& p) {
                        if (p.AtEnd()) return false;
                        if (cs.a[static_cast<int>(p.GetElem())]) {
                                p.GotoNext();
                                return true;
                        }
                        return false;
                }
        };

        template<typename CharSet_T>
        const CharSet_T CharSetParser<CharSet_T>::cs;

        // accepts any single char, except for the specified character set
        template<typename CharSet_T>
        struct AnyCharExcept
                : Seq<NotAt<CharSetParser<CharSet_T> >, AnyChar>
        { };

        // CharSetRangeParser is a shorthand for CharSetParser<char_set_range>
        template<char C0, char C1>
        struct CharRange : CharSetParser<CharSetRange<C0, C1> > { };

        // decimal digit parser
        struct Digit : CharSetParser<DigitCharSet> { };

        // binary digit parser
        struct BinDigit : Or<Char<'0'>, Char<'1'> > { };

        // hexadecimal digit parser
        struct HexDigit : CharSetParser<HexDigitCharSet> { };

        // octal digit parser
        struct OctDigit : CharSetParser<OctDigitCharSet> { };

        // parses letters and underscores
        struct IdentFirstChar : CharSetParser<IdentFirstCharSet> { };

        // parses letters and underscores and numbers
        struct IdentNextChar : CharSetParser<IdentNextCharSet> { };

        // parses letters
        struct Letter : CharSetParser<LetterCharSet> { };

        // parses letters or numbers
        struct AlphaNum : CharSetParser<AlphaNumCharSet> { };

        // Not an alpha numeric character or underscore
        struct NotAlphaNum : NotAt<IdentNextChar> { };

        // parses lower case letters
        struct LowerCaseLetter : CharSetParser<LowerCaseLetterCharSet> { };

        // parses upper case letters
        struct UpperCaseLetter : CharSetParser<UpperCaseLetterCharSet> { };

        // Ident matches C/C++/Java/etc. identifiers. A letter or underscore followed
        // by a sequence of letters, underscores and numbers of arbitrary length
        struct Ident
        {
                template<typename ParserState_T>
                static bool Match(ParserState_T& p) {
                  if (p.AtEnd()) { return false; }
                  if (IdentFirstChar::template Match(p))
                  {
                        while (IdentNextChar::template Match(p)) { }
                        return true;
                  }
                  return false;
                }
        };

        // this represents the NULL string
        struct NS {
                static char GetChar(int) {
                  return '\0';
                }
        };

        // this matches a sequence of characters
        template
        <
                char C0 = '\0', char C1 = '\0', char C2 = '\0', char C3 = '\0',
                char C4 = '\0', char C5 = '\0', char C6 = '\0', char C7 = '\0',
                char C8 = '\0', char C9 = '\0', char C10 = '\0', char C11 = '\0',
                char C12 = '\0', char C13 = '\0', char C14 = '\0', char C15 = '\0'
        >
        struct CharSeq
        {
        static char GetChar(int n) {
          switch(n) {
                case (0) : return C0;
                case (1) : return C1;
                case (2) : return C2;
                case (3) : return C3;
                case (4) : return C4;
                case (5) : return C5;
                case (6) : return C6;
                case (7) : return C7;
                case (8) : return C8;
                case (9) : return C9;
                case (10) : return C10;
                case (11) : return C11;
                case (12) : return C12;
                case (13) : return C13;
                case (14) : return C14;
                case (15) : return C15;
                default : assert(false && "maximum length of strings is 16 chars");
          }
          return '\0';
        }

        template<typename ParserState_T>
        static bool Match(ParserState_T& p) {
                typename ParserState_T::Iterator pos = p.GetPos();
                        int n = 0;
                        char ch = GetChar(n);
                        while (ch != '\0')
                        {
                                if (p.AtEnd() || p.GetElem() != ch)
                                {
                                        p.SetPos(pos);
                                        return false;
                                }
                                p.GotoNext();
                                ch = GetChar(++n);
                        }
                        return true;
                }
        };

        // this matches sequences of lower case characters, ignoring the case
        // of the input
        template
        <
                char C0 = '\0', char C1 = '\0', char C2 = '\0', char C3 = '\0',
                char C4 = '\0', char C5 = '\0', char C6 = '\0', char C7 = '\0',
                char C8 = '\0', char C9 = '\0', char C10 = '\0', char C11 = '\0',
                char C12 = '\0', char C13 = '\0', char C14 = '\0', char C15 = '\0'
        >
        struct CharSeqIgnoreCase
        {
        static char GetChar(int n) {
          switch(n) {
                case (0) : return C0;
                case (1) : return C1;
                case (2) : return C2;
                case (3) : return C3;
                case (4) : return C4;
                case (5) : return C5;
                case (6) : return C6;
                case (7) : return C7;
                case (8) : return C8;
                case (9) : return C9;
                case (10) : return C10;
                case (11) : return C11;
                case (12) : return C12;
                case (13) : return C13;
                case (14) : return C14;
                case (15) : return C15;
                default : assert(false && "maximum length of strings is 16 chars");
          }
          return '\0';
        }

        static char ToLowercase(char x)
        {
                if (x >= 'A' && x <= 'Z')
                        return 'a' + (x - 'A');
                return x;
        }

        template<typename ParserState_T>
        static bool Match(ParserState_T& p) {
                typename ParserState_T::Iterator pos = p.GetPos();
                for (int n = 0; GetChar(n) != '\0'; ++n) {
                        if (p.AtEnd())
                        {
                                p.SetPos(pos);
                                return false;
                        }
                        if (ToLowercase(p.GetElem()) != GetChar(n))
                        {
                                p.SetPos(pos);
                                return false;
                        }
                        p.GotoNext();
                }
                return true;
                }
        };

        // this matches parser types which end a word boundary
        template<typename T>
        struct Word :
        Seq<
                T,
                NotAt<IdentNextChar>
        >
        { };

        template<typename T>
        struct DoubleQuoted :
                Seq<Char<'\"'>, T, Char<'\"'> >
        { };

        template<typename T>
        struct SingleQuoted :
                Seq<Char<'\''>, T, Char<'\''> >
        { };

        template<typename T>
        struct SingleOrDoubleQuoted :
                Or<SingleQuoted<T>, DoubleQuoted<T> >
        { };

} // text_grammar
} // yard

#endif // #ifndef YARD_TEXT_GRAMMAR_HPP
