#######
OpenDDS
#######

Welcome to the documentation for OpenDDS |release|! If you are just getting
started with OpenDDS, please check out ":doc:`devguide/introduction`".

*****************
Developer's Guide
*****************

.. toctree::
  :maxdepth: 2

  devguide/preface
  devguide/introduction

.. attention::

  If we were to go all in with Sphinx, the rest of the Developer's Guide would
  go here.


*************
API Reference
*************

.. toctree::
  :glob:

  apiref/*

.. attention::

  I would like there to be a definition of the public C++ API. I really can't
  see Doxygen meeting this goal between IDL and all the internal stuff it
  includes right now. For now all I can think of is mannually defining it, but
  this warrents further research and discussion.

  Also Add:

  Reference guide for IDL. This would contain IDL syntax relevent to OpenDDS
  and what C++ types they map to. Also document IDL annotations here.

  Maybe Limited Reference Guide to MPC or at least the OpenDDS specfic base
  projects and such.

  man page for opendds_idl (Remove opendds_idl options from devguide) ability
  to install the man page.


**********************
Internal Documentation
**********************

This documentation are for those who want to contribute to OpenDDS and those
who are just curious!

.. toctree::
  :maxdepth: 1

  Development Guidelines <guidelines>

.. attention::

  If we were to go all in with Sphinx, convert guidelines to rst. Also add
  other files optionally converting them to rst. Maybe even the files in the
  design dir...


******************
Indices and tables
******************

* :ref:`genindex`
* :ref:`modindex`
* :ref:`search`
