.. news-prs: 5301

.. news-start-section: Fixes
- Fixed generated union deserialization, ``set_default``, and ``vread`` code that could set a union's discriminator to select a member that hadn't been constructed.

  - With versions of tao_idl that no longer zero union storage in the default constructor, this could free invalid memory and crash.

.. news-end-section

.. news-start-section: Notes
- Application code should use a union member's modifier to change the active member, not the ``_d()`` modifier.
  The IDL-to-C++ mappings only allow ``_d()`` to select a different discriminator value for the member that's already active.
  Code that calls ``_d()`` to switch members, for example ``u._d(2); u.member(value);``, can crash with newer versions of tao_idl.

.. news-end-section
