.. _annex:

#####
Annex
#####

.. _fnmatch-exprs:

*******************
Fnmatch Expressions
*******************

A wildcard-capable string used to match one or more names from a set.
This is used in :ref:`qos-partition` and in :ref:`sec` documents.
Recognized values will conform to POSIX ``fnmatch()`` function as specified in POSIX 1003.2-1992, Section B.6.
This is a subset of UNIX shell file matching and is similar to, but separate from standard regular expressions.

Simplified, this consists of the following:

``?``
  Will match any single character.
  For example ``ab?`` matches ``abc`` and ``abb``.

``*``
  Will match any zero or more characters.
  For example ``*`` will match anything and ``a*`` matches ``a``, ``abc``, and ``aaaaa``.

``[]``
  Will match a single character specified in the brackets.
  For example ``a[bc]`` matches ``ab`` and ``ac``.
  Can also use ranges, for example ``a[b-d]`` matches ``ab``, ``ac``, and ``ad``.

``\``
  Will escape the following character.
  For example ``\?`` just matches ``?`` and ``\\`` matches ``\``.
