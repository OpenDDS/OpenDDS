.. news-prs: 4574

.. news-start-section: Removals
- Values passed to the configure script via ``--mpcopts`` are no longer split on spaces.

  - For example, ``./configure --mpcopts="-value_template build_flags+=-Wall -Werror"`` must now be written as ``./configure --mpcopts=-value_template --mpcopts="build_flags+=-Wall -Werror"``.
.. news-end-section

.. news-start-section: Additions
- Add a ``configure`` script option for MPC options requiring a value.

  - For example, ``./configure --mpc:value_template build_flags+="-Wall -Werror"``.
.. news-end-section
