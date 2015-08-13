// Public domain, by Christopher Diggins
// http://www.cdiggins.com
//
// This files defines the basic types used for defining YARD parse rules.
//
// Parse rules are types with no fields that implement a single static Match function.
// The Match function takes a parser as a parameter, and returns a boolean value indicating whether
// the associated rule is matches the current position of the parser. If the Match function
// fails, it must restore the parser to its original state.
//
// Most but not all Match rules will advance the parser's internal position pointer if successful.
// Some exception are zero-width assertions such as "At", and "Not".
// Parsing rules can be combined using various rule operations such as "Or" for matching
// any single rule from a set of rules, or "Seq" which attempts to match a sequence
// of rules.

#ifndef YARD_BASE_GRAMMAR_HPP
#define YARD_BASE_GRAMMAR_HPP

namespace yard
{
    // Store creates a new node with the given Label if parsing is successful.
    // A Label type is expected to provide an "id" static integer value for
    // use in case statements.
    template<class Rule_T>
    struct Store
    {
        template<typename ParserState_T>
        static bool Match(ParserState_T& p)
        {
            p.template CreateNode<Rule_T>();

            //printf("created node %s\n", typeid(Rule_T).name());
            bool b = false;

            try {
                b = Rule_T::template Match(p);
            }
            catch(...)
            {
                p.AbandonNode();
                throw;
            }

            if (b) {
                p.CompleteNode();
                //printf("completed node %s\n", typeid(Rule_T).name());
                return true;
            }
            else {
                p.AbandonNode();
                //printf("abandoned node %s\n", typeid(Rule_T).name());
                return false;
            }
        }
    };

    // Finao (Failure Is Not An Option) Throws an Exception if the rule fails to match
    template<typename Rule_T>
    struct Finao
    {
        template<typename ParserState_T>
        static bool Match(ParserState_T& p) {
            if (!Rule_T::template Match(p)) {
                printf("Ill formed input, expected rule %s failed to match\n",
                    typeid(Rule_T).name());
                p.template OutputLocation<Rule_T>();
                throw 0;
            }
            return true;
        }
    };

    // Short-hand for Store<Finao<T> >
    template<typename Rule_T>
    struct StoreFinao :
        Finao< Store<Rule_T> >
    { };

    // True_T is used as a default rule in Seq operations
    // always returns true, and doesn't update the input parser
    struct True_T
    {
        template<typename ParserState_T>
        static bool Match(ParserState_T&) {
            return true;
        }
    };

    // False_T is used as a default rule in choice operations,
    // this rule always returns false, and doesn't update the input parser
    struct False_T {
        template<typename ParserState_T>
        static bool Match(ParserState_T&) {
            return false;
        }
    };

    // used to match the end of file,
    // probably for checking that there isn't any data there that would
    // otherwise be ignored by the parser
    // matching this rule doesn't actually consume the end of file
    struct EndOfInput
    {
        template<typename ParserState_T>
        static bool Match(ParserState_T& p) {
          return p.AtEnd();
        }
    };

    // At returns true if the specified rule can be matched, but
    // does not advance the input pointer.
    template<typename Rule_T>
    struct At
    {
        template<typename ParserState_T>
        static bool Match(ParserState_T& p) {
            typename ParserState_T::Iterator pos = p.GetPos();
            if (Rule_T::template Match(p)) {
                p.SetPos(pos);
                return true;
            }
            return false;
        }
    };

    // Not returns true if the specified rule can not be matched.
    // but does not advance the input pointer
    template<typename Rule_T>
    struct NotAt
    {
        template<typename ParserState_T>
        static bool Match(ParserState_T& p) {
            typename ParserState_T::Iterator pos = p.GetPos();
            if (Rule_T::template Match(p)) {
                p.SetPos(pos);
                return false;
            }
            return true;
        }
    };

    // Or attempts to match each of the passed rules until
    // one is found successful.
    template<
        typename T0
        , typename T1
        , typename T2 = False_T
        , typename T3 = False_T
        , typename T4 = False_T
        , typename T5 = False_T
        , typename T6 = False_T
        , typename T7 = False_T
        , typename T8 = False_T
        , typename T9 = False_T
    >
    struct Or
    {
        template<typename ParserState_T>
        static bool Match(ParserState_T& p) {
          return
              T0::template Match(p)
            || T1::template Match(p)
            || T2::template Match(p)
            || T3::template Match(p)
            || T4::template Match(p)
            || T5::template Match(p)
            || T6::template Match(p)
            || T7::template Match(p)
            || T8::template Match(p)
            || T9::template Match(p);
        }
    };

    // Seq attempts to match each of the passed rules, sequentially until
    // they all pass. If one fails, the input is reset to the original position
    template
    <
        typename T0
        , typename T1
        , typename T2 = True_T
        , typename T3 = True_T
        , typename T4 = True_T
        , typename T5 = True_T
        , typename T6 = True_T
        , typename T7 = True_T
        , typename T8 = True_T
        , typename T9 = True_T
    >
    struct Seq
    {
        template<typename ParserState_T>
        static bool Match(ParserState_T& p) {
            typename ParserState_T::Iterator pos = p.GetPos();
            if (
            T0::template Match(p)
            && T1::template Match(p)
            && T2::template Match(p)
            && T3::template Match(p)
            && T4::template Match(p)
            && T5::template Match(p)
            && T6::template Match(p)
            && T7::template Match(p)
            && T8::template Match(p)
            && T9::template Match(p)
            )
            {
                return true;
            }
            else
            {
                p.SetPos(pos);
                return false;
            }
        }
    };

    // Star matches a rule 0 or more times, as many times as possible.
    // This always returns true
    // note: unlike Perl regular expression, partial backtracking is not
    // performed
    template<typename Rule_T>
    struct Star
    {
        template<typename ParserState_T>
        static bool Match(ParserState_T& p) {
            if (!p.AtEnd())
            {
                while (Rule_T::template Match(p))
                { }
            }
            return true;
        }
    };

    // Plus matches the rule as many times as possible, and returns true if
    // at least one match is successful
    // note: unlike Perl regular expression, partial backtracking is not
    // performed
    template<typename Rule_T>
    struct Plus
    {
        template<typename ParserState_T>
        static bool Match(ParserState_T& p) {
            if (!Rule_T::template Match(p)) {
                return false;
            }
            Star<Rule_T>::template Match(p);
            return true;
        }
    };

    // Opt attempts to match a rule, but returns true even if not
    // successful, even at the end of a file.
    template<typename Rule_T>
    struct Opt
    {
        template<typename ParserState_T>
        static bool Match(ParserState_T& p) {
            if (!p.AtEnd())
                Rule_T::template Match(p);
            return true;
        }
    };

    // Repeat attempts to match a rule precisely N times.
    template<typename Rule_T, unsigned int N>
    struct Repeat
    {
        template<typename ParserState_T>
        static bool Match(ParserState_T& p)
        {
            typename ParserState_T::Iterator pos = p.GetPos();
            for (int i=0; i < N; ++i) {
                if (!Rule_T::template Match(p)) {
                  p.SetPos(pos);
                  return false;
                }
            }
            return true;
        }
    };

    // UntilPast eats single characters until the rule is successfully matched
    // or end of file is reached. UntilPast returns false only if the end of the file
    // is found before before the rule.
    template<typename Rule_T>
    struct UntilPast
    {
        template<typename ParserState_T>
        static bool Match(ParserState_T& p) {
            typename ParserState_T::Iterator pos = p.GetPos();
            while (true) {
                if (Rule_T::template Match(p)) {
                    return true;
                }
                p.GotoNext();
                if (p.AtEnd()) {
                    p.SetPos(pos);
                    return false;
                }
            }
            return true;
        }
    };

    // Similar to UntilPast, but doesn't parse the last rule.
    template<typename Rule_T>
    struct UntilAt
        : UntilPast<At<Rule_T> >
    { };

    // Short-hand for Finao<Store<T> >
    template<typename Label_T, typename Rule_T>
    struct MustStore :
        Finao<Store<Rule_T> >
    { };

    // Matches the second rule if the first rule matches, but will not recover if the second
    // rule fails to match
    template<typename T, typename U>
    struct FinaoIf :
        Seq<T, Finao<U> >
    { };

    // Constructs an AST node from the second rule if the first rule matches
    template<typename Label_T, typename T, typename U>
    struct StoreIf :
        Seq<T, MustStore<Label_T, U> >
    { };

    // This matches a sequence, but will not recover if failure occurs after
    // the first item was matched
    template
    <
        typename T0
        , typename T1
        , typename T2 = True_T
        , typename T3 = True_T
        , typename T4 = True_T
        , typename T5 = True_T
        , typename T6 = True_T
        , typename T7 = True_T
        , typename T8 = True_T
        , typename T9 = True_T
    >
    struct NoFailSeq :
        FinaoIf<
            T0,
            Seq<
                Finao<T1>,
                Finao<T2>,
                Finao<T3>,
                Finao<T4>,
                Finao<T5>,
                Finao<T6>,
                Finao<T7>,
                Finao<T8>,
                Finao<T9>
            >
        >
    { };

    // While rule T matches goes on to match U
    template<typename T, typename U>
    struct RepeatWhile : Star<Seq<T, U> > { };

    // While rule T matches goes on to match U, but does not allow failure.
    template<typename T, typename U>
    struct RepeatWhileNoFail : Star<Seq<T, Finao<U> > > { };

    // Used for debugging
    // Will match the rule "p".
    template<typename T>
    struct Log
    {
        template<typename ParserState_T>
        static bool Match(ParserState_T& p) {
            static int nUniqueLogMsg = 0;
            int nCurLogMsg = nUniqueLogMsg++;
            printf("#%d : matching parse rule %s\n", nCurLogMsg, typeid(T).name());
            p.template OutputLocation<T>();
            bool b = T::template Match(p);
            if (b) {
                printf("#%d : succeeded for rule %s\n", nCurLogMsg, typeid(T).name());
            }
            else {
                printf("#%d : failed for rule %s\n", nCurLogMsg, typeid(T).name());
            }
            return b;
        }
    };

    template<typename T, typename U>
    struct StarExcept :
        Star<Seq<NotAt<U>, T> >
    { };
}

#endif // #ifndef YARD_BASE_GRAMMAR_HPP
