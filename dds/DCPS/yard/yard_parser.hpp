// Dedicated to the public domain by Christopher Diggins
// http://www.cdiggins.com
//
// Contains definitions for sample parser management classes
// to be used with the YARD framework

#ifndef YARD_PARSER_HPP
#define YARD_PARSER_HPP

namespace yard
{
    ////////////////////////////////////////////////////////////////////////
    // A skeleton parser base class

    template<typename Token_T, typename Iter_T = const Token_T*>
    struct BasicParser
    {
        // Constructor
        BasicParser(Iter_T first, Iter_T last)
            : mBegin(first), mEnd(last), mIter(first)
        { }

        // Parse function
        template<typename StartRule_T>
        bool Parse()
        {
            try {
                return StartRule_T::Match(*this);
            }
            catch(...) {
                return false;
            }
        }

        // Public typedefs
        typedef Iter_T Iterator;
        typedef Token_T Token;

        // Input pointer functions
        Token GetElem() { return *mIter; }
        void GotoNext() { assert(mIter < End()); ++mIter; }
        Iterator GetPos() { return mIter; }
        void SetPos(Iterator pos) { mIter = pos; }
        bool AtEnd() { return GetPos() >= End(); }
        Iterator Begin() { return mBegin; }
        Iterator End() { return mEnd; }

        template<typename T>
        void OutputLocation(const T& /*x*/)
        { }

    protected:

        // Member fields
        Iterator    mBegin;
        Iterator    mEnd;
        Iterator    mIter;
    };

    ////////////////////////////////////////////////////////////////////////
    // Extends the basic parser with abstract syntax tree (AST)
    // building capabilities

    template<typename Token_T, typename Iter_T = const Token_T*>
    struct TreeBuildingParser
        : BasicParser<Token_T, Iter_T>
    {
        // Constructor
        TreeBuildingParser(Iter_T first, Iter_T last)
            : BasicParser<Token_T, Iter_T>(first, last), mTree()
        { }

        // Parse function
        template<typename StartRule_T>
        bool Parse()
        {
            try {
                return StartRule_T::Match(*this);
            }
            catch(...) {
                return false;
            }
        }

        // Public typedefs
        typedef Iter_T Iterator;
        typedef Token_T Token;
        typedef Ast<Iterator> Tree;
        typedef typename Tree::AbstractNode Node;

        // AST functions
        Node* GetAstRoot() { return mTree.GetRoot(); }
        template<typename Rule_T>
        void CreateNode() { mTree.template CreateNode<Rule_T>(*this); }
        void CompleteNode() { mTree.CompleteNode(*this); }
        void AbandonNode() { mTree.AbandonNode(*this); }

    protected:

        // Member fields
        Tree mTree;
    };

    ////////////////////////////////////////////////////////////////////////
    // A useful simple parser for parsing ascii text

    struct SimpleTextParser
        : TreeBuildingParser<char>
    {
        // Public typedefs
        typedef const char* Iterator;
        typedef char Token;
        typedef Tree::AbstractNode Node;

        // Constructor
        SimpleTextParser(Iterator first, Iterator last)
            : TreeBuildingParser<char>(first, last)
        { }

        // Parse function
        template<typename StartRule_T>
        bool Parse() {
            try {
                return StartRule_T::Match(*this);
            }
            catch (...) {
                return false;
            }
        }

        template<typename T>
        void OutputLocation()
        {
            char line[256];
            Iterator pFirst = GetPos();
            while (pFirst > mBegin && *pFirst != '\n')
                pFirst--;
            if (*pFirst == '\n')
                ++pFirst;
            Iterator pLast = GetPos();
            while (pLast < mEnd && *pLast != '\n')
                pLast++;
            size_t n = pLast - pFirst;
            n = n < 254 ? n : 254;
            strncpy(line, pFirst, n);
            line[n] = '\0';

            char marker[256];
            n = GetPos() - pFirst;
            n = n < 254 ? n : 254;
            for (size_t i=0; i < n; ++i)
                marker[i] = ' ';
            marker[n] = '^';
            marker[n + 1] = '\0';

            // Count lines
            int nline = 1;
            for (Iterator i = mBegin; i < pFirst; ++i) {
                if (*i == '\n') {
                    ++nline;
                }
            }

            printf("character number %d\n", (int) n);
            printf("line number %d\n", nline);
            printf("%s\n", line);
            printf("%s\n", marker);
        }
    };

 }

#endif
