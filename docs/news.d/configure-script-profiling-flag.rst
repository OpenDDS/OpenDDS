.. news-prs: 5169

.. news-start-section: Additions
- Added a '--profiling' flag to the configure script to enable common flags used during profiling (gcc / clang)

  - This enables some optimization ('-O3') but also includes debuging info ('-ggdb') and protects frame pointers ('-fno-omit-frame-pointer')
    in order to help in generating clean stack traces for profilers and other utilities which examine the call stack.

.. news-end-section

