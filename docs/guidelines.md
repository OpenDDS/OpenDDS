# OpenDDS Development Guidelines

This document organizes our current thoughts around development guidelines in
a place that's readable and editable by the overall user and maintainer
community. It's expected to evolve as different maintainers get a chance to
review and contribute to it.

Although ideally all code in the repository would already follow these
guidelines, in reality the code has evolved over many years by a diverse group
of developers. At one point an automated re-formatter was run on the codebase,
migrating from the [GNU C
style](https://www.gnu.org/prep/standards/html_node/Writing-C.html) to the
current, more conventional style, but automated tools can only cover a subset
of the guidelines.

**Table of Contents**

* [Repository](#repository)
* [Automated Build Systems](#automated-build-systems)
* [Doxygen](#doxygen)
* [Dependencies](#dependencies)
* [Text File Formatting](#text-file-formatting)
* [C++ Standard](#c-standard)
* [C++ Coding Style](#c-coding-style)
  * [Example](#example)
  * [Punctuation](#punctuation)
  * [Whitespace](#whitespace)
  * [Language Usage](#language-usage)
  * [Pointers and References](#pointers-and-references)
  * [Naming](#naming)
  * [Comments](#comments)
  * [Documenting Code for Doxygen](#documenting-code-for-doxygen)
  * [Preprocessor](#preprocessor)
    * [Includes](#includes)
  * [Time](#time)

## Repository

The repository is hosted on Github at
[objectcomputing/OpenDDS](https://github.com/objectcomputing/OpenDDS) and is
open for pull requests.

## Automated Build Systems

Pull requests will be tested automatically and full CI builds of the master
branch can be found at
[http://scoreboard.ociweb.com/oci-dds.html](http://scoreboard.ociweb.com/oci-dds.html).

All tests listed in `DDS_ROOT/bin/dcps_tests.lst` are part of the automated
test suite.

## Doxygen

Doxygen is run on OpenDDS regularly. There are two hosted versions of this:

- [Latest Release](http://download.opendds.org/doxygen)
  - Based on the current release of OpenDDS.
- Master
  - Based on the master branch in the repository. To access it, go to the
    [scoreboard](http://scoreboard.ociweb.com/oci-dds.html) and click the green
    "Doxygen" link near the top.
  - Depending on the activity in the repository this might be unstable because
    of the time it takes to get the updated Doxygen on to the web sever. Prefer
    latest release unless working with newer code.

See [Documenting Code for Doxygen](#documenting-code-for-doxygen) to see how to
take advantage of Doxygen when writing code in OpenDDS.

## Dependencies

- MPC is the build system, used to configure the build and generate platform
  specific build files (Makefiles, VS solution files, etc.).
- ACE is a library used for cross-platform compatibility, especially
  networking and event loops. It is used both directly and through TAO.
- TAO is a C++ CORBA implementation built on ACE used extensively in the
  traditional OpenDDS operating mode which uses the DCPSInfoRepo. TAO types are
  also used in the End User DDS API. The TAO IDL compiler is used internally
  and by the end user to allow OpenDDS to use user defined IDL types as topic
  data.
- Perl is an interpreted language used in the configure script, the tests, and any
  other scripting in OpenDDS codebase.
- Google Test is required for OpenDDS tests. By default, CMake will be used to
  build a specific version of Google Test that we have as a submodule. An
  appropriate prebuilt or system Google Test can also be used.

See [dependencies.md](dependencies.md) for all dependencies and details on how
these are used in OpenDDS.

## Text File Formatting

All text files in the source code repository follow a few basic rules. These
apply to C++ source code, Perl scripts, MPC files, and any other plaintext
file.

- A text file is a sequence of lines, each ending in the "end-of-line"
  character (AKA Unix line endings).
- Based on this rule, all files end with the end-of-line character.
- The character before end-of-line is a non-whitespace character (no trailing
  whitespace).
- Tabs are not used.
  - One exception, MPC files may contain literal text that's inserted into
    Makefiles which could require tabs.
  - In place of a tab, use a set number of spaces (depending on what type of
    file it is, C++ uses 2 spaces).
- Keep line length reasonable. I don't think it makes sense to strictly
  enforce an 80-column limit, but overly long lines are harder to read. Try to
  keep lines to roughly 80 characters.

## C++ Standard

The C++ standard used in OpenDDS is C++03. There are some caveats to this but
the OpenDDS must be able to be compiled with C++ 2003 compilers.

Use the C++ standard library as much as possible. The standard library should
be preferred over ACE, which in turn should be preferred over system specific
libraries. The C++ standard library includes the C standard library by
reference, making those identifiers available in namespace std. Not all
supported platforms have standard library support for wide characters
(`wchar_t`) but this is rarely needed. Preprocessor macro `DDS_HAS_WCHAR` can
be used to detect those platforms.

## C++ Coding Style

- C++ code in OpenDDS must compile under the [compilers listed in the
  `README.md` file](../README.md#compilers).
- Commit code in the proper style from the start, so follow-on commits to
  adjust style don't clutter history.
- C++ source code is a plaintext file, so the guidelines in Text file
  formatting apply.
- A modified Stroustrup style is used (see [tools/scripts/style](../tools/scripts/style)).
  - Warning: not everything in tools/scripts/style represents the current
    guidelines.
- Sometimes the punctuation characters are given different names, this document
  will use:
  - Parentheses `( )`
  - Braces `{ }`
  - Brackets `[ ]`

### Example

```C++
template<typename T>
class MyClass : public Base1, public Base2 {
public:
  bool method(const OtherClass& parameter, int idx = 0) const;
};

template<typename T>
bool MyClass<T>::method(const OtherClass& parameter, int) const
{
  if (parameter.foo() > 42) {
    return member_data_;
  } else {
    for (int i = 0; i < some_member_; ++i) {
      other_method(i);
    }
    return false;
  }
}
```

### Punctuation

The punctuation placement rules can be summarized as:

- Open brace appears as the first non-whitespace character on the line to start
  function definitions.
- Otherwise the open brace shares the line with the preceding text.
- Parentheses used for control-flow keywords (`if`, `while`, `for`, `switch`)
  are separated from the keyword by a single space.
- Otherwise parentheses and brackets are not preceded by spaces.

### Whitespace

- Each "tab stop" is two spaces.
- Namespace scopes that span most or all of a file do not cause indentation of
  their contents.
- Otherwise lines ending in `{` indicate that subsequent lines should be
  indented one more level until `}`.
- Continuation lines (when a statement spans more than one line) can either be
  indented one more level, or indented to nest "under" an `(` or similar
  punctuation.
- Add space around binary operators and after commas: `a + b`
- Do not add space around parentheses for function calls, a properly formatted
  function call looks like `func(arg1, arg2, arg3);`
- Do not add space around brackets for indexing, instead it should look like:
  `mymap[key]`
- In general, do not add space :) Do not add extra spaces to make syntax
  elements (that span lines/statements) line up. This only causes unnecessary
  changes in adjacent lines as the code evolves.

### Language Usage

- Add braces following control-flow keywords even when they are optional.
- `this->` is not used unless required for disambiguation or to access members
  of a template-dependent base class.
- Declare local variables at the latest point possible.
- `const` is a powerful tool that enables the compiler to help the programmer
  find bugs. Use `const` everywhere possible, including local variables.
- Modifiers like `const` appear left of the types they modify, like: `const
  char* cstring = ...`. `char const*` is equivalent but not conventional.
- For function arguments that are not modified by the callee, pass by value for
  small objects (8 bytes?) and pass by const-reference for everything else.
- Arguments unused by the implementation have no names (in the definition that
  is, the declarations still have names), or a `/*commented-out*/` name.
- Use `explicit` constructors unless implicit conversions are intended and
  desirable.
- Use the constructor initializer list and make sure its order matches the
  declaration order.
- Prefer pre-increment/decrement (`++x`) to post-increment/decrement (`x++`)
  for both objects and non-objects.
- All currently supported compilers use the template inclusion mechanism. Thus
  function/method template definitions may not be placed in normal `*.cpp`
  files, instead they can go in `_T.cpp` (which are `#included` and not
  separately compiled), or directly in the `*.h`. In this case, `*_T.cpp` takes
  the place of `*.inl` (except it is always inlined). See ACE for a description
  of `*.inl` files.

### Pointers and References

Pointers and references go along with the type, not the identifier. For example:

```C++
int* intPtr = &someInt;
```

Watch out for multiple declarations in one statement. `int* c, b;` does not
declare two pointers! It's best just to break these into separate statements:

```C++
int* c;
int* b;
```

In code targeting C++03, `0` should be used as the null pointer. For C++11 and
later, `nullptr` should be used instead. `NULL` should never be used.

### Naming

**(For library code that the user may link to)**

- Preprocessor macros visible to user code must begin with `OPENDDS_`
- C++ identifiers are either in top-level namespace `DDS` (OMG spec defined) or
  `OpenDDS` (otherwise)
- Within the `OpenDDS` namespace there are some nested namespaces:
  - `DCPS`: anything relating to the implementation of the DCPS portion of the
    DDS spec
  - `RTPS`: types directly tied to the RTPS spec
  - `Federator`: DCPSInfoRepo federation
  - `FileSystemStorage`: reusable component for persistent storage
- Naming conventions
  - `ClassesAreCamelCaseWithInitialCapital`
  - `methodsAreCamelCaseWithInitialLower` OR
    `methods_are_lower_case_with_underscores`
  - `member_data_use_underscores_and_end_with_an_underscore_`

### Comments

- Add comments only when they will provide MORE information to the reader.
- Describing the code verbatim in comments doesn't add any additional
  information.
- If you start out implementation with comments describing what the code will
  do (or pseudocode), review all comments after implementation is done to make
  sure they are not redundant.
- Do not add a comment before the constructor that says `// Constructor`. We
  know it's a constructor. The same note applies to any redundant comment.

### Documenting Code for Doxygen

Doxygen is run on the codebase with each change in master and each release.
This is a simple guide showing the way of documenting in OpenDDS.

Doxygen supports multiple styles of documenting comments but this style should
be used in non-trivial situations:

```C++
/**
 * This sentence is the brief description.
 *
 * Everything else is the details.
 */
class DoesStuff {
// ...
};
```

For simple things, a single line documenting comment can be made like:

```C++
/// Number of bugs in the code
unsigned bug_count = -1; // Woops
```

The extra `*` on the multiline comment and `/` on the single line comment are
important. They inform Doxygen that comment is the documentation for the
following declaration.

If referring to something that happens to be a namespace or other global
object (like DDS, OpenDDS, or RTPS), you should precede it with a `%`.
If not it will turn into a link to that object.

For more information, see [the Doxygen manual](http://www.doxygen.nl/manual/).

### Preprocessor

- If possible, use other language features things like inlining and constants
  instead of the preprocessor.
- Prefer `#ifdef` and `#ifndef` to `#if defined` and `#if !defined` when
  testing if a single macro is defined.
- Leave parentheses off preprocessor operators. For example, use `#if defined X
  && defined Y` instead of `#if defined(X) && defined(Y)`.
- As stated before, preprocessor macros visible to user code must begin with
  `OPENDDS_`.
- Ignoring the header guard if there is one, preprocessor statements should be
  indented using two spaces starting at the pound symbol, like so:

```C++
#if defined X && defined Y
#  if X > Y
#    define Z 1
#  else
#    define Z 0
#  endif
#else
#  define Z -1
#endif
```

#### Includes

As a safeguard against headers being dependant on a particular order, includes
should be ordered based on a hierarchy going from local headers to system
headers, with spaces between groups of includes. This order can be generalized
as the following:

1. The corresponding header to the source file (`Foo.h` if we were in
   `Foo.cpp`).
2. Headers from the local project.
3. Headers from external OpenDDS-based libraries.
4. User API OpenDDS Headers.
4. Internal API OpenDDS Headers.
5. Headers from external TAO-based libraries.
6. Headers from TAO.
7. Headers from external ACE-based libraries.
8. Headers from ACE.
9. Headers from external non-ACE-based libraries.
10. Headers from system and C++ standard libraries.

Headers should only use local includes (`#include "foo/Foo.h"`) if the header
is relative to the file. Otherwise system includes (`#include <foo/Foo.h>`)
should be used to make it clear that the header is on the system include path.

### Time

Measurements of time can be broken down into two basic classes: A specific
point in time (Ex: 00:00 January 1, 1970) and a length or duration of time
without context (Ex: 134 Seconds). In addition, a computer can change its clock
while a program is running, which could mess up any time lapses being measured.
To solve this problem, operating systems provide what's called a monotonic
clock that runs independently of the normal system clock.

ACE can provide monotonic clock time and has a class for handling time
measurements, `ACE_Time_Value`, but it doesn't differentiate between specific
points in time and durations of time. It can differentiate between the system
clock and the monotonic clock, but it does so poorly. OpenDDS provides three
classes that wrap `ACE_Time_Value` to fill these roles: `TimeDuration`,
`MonotonicTimePoint`, and `SystemTimePoint`. All three can be included using
`dds/DCPS/TimeTypes.h`. Using `ACE_Time_Value` is discouraged unless directly
dealing with ACE code which requires it and using `ACE_OS::gettimeofday()` or
`ACE_Time_Value().now()` in C++ code in `dds/DCPS` treated as an error by the
`dds_fuzz.pl` linter script.

`MonotonicTimePoint` should be used when tracking time elapsed internally and
when dealing with `ACE_Time_Value`s being given by the `ACE_Reactor` in
OpenDDS. `ACE_Condition`s, like all ACE code, will default to using system
time. They must modified to use monotonic time using by passing
`ConditionAttributesMonotonic()` as the second argument in the constructor. An
example of this can be seen `wait_messages_pending()` in
`dds/DCPS/MessageTracker.cpp`.

More information on using monotonic time with ACE can be found
[here](http://www.dre.vanderbilt.edu/~schmidt/DOC_ROOT/ACE/docs/ACE-monotonic-timer.html).

`SystemTimePoint` should be used when dealing with the DDS API and timestamps
on incoming and outgoing messages.
