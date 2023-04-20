# This is a news fragment example, copy this file in this directory to have
# something added to the news. It can be named anything as long as it's
# reasonably unique, doesn't start with an underscore, and has a rst file
# extension. The branch name of the PR would probably be a good choice.
# For example: fix_rtps_segfault.rst.

# Replace this with the PR number after the PR is created. Please add follow-on
# PRs as they are created. Multiple PRs are space separated. Do not add "#" at
# the start of the numbers.
# This can be omitted, but that should only be done when a fragment doesn't
# have a PR for the change.
.. news-prs: 999999 1000000

# news-push and news-pop directives are sections in the news. They are all
# merged together in the final result. Add news items as reStructuredText lists
# within the news-push and new-pop directives.
.. news-push: Additions

# Increase rank to have following items and sections listed earlier. When the
# rank of items are the same, items with the lowest PR numbers are listed
# first. Rank is pushed and popped by news-push and news-pop.
# Unless set higher, the builtin sections have a default rank so they come in a
# consistent order.
.. news-rank: 0

- This is an addition we made.

  - This is detailed information about the addition.

.. news-pop

.. news-push: Fixes
- This is a fix we made.

  - This is detailed information about the fix.

# Sections can go inside other sections and are all merged. Sections will have
# no effect unless they have text somewhere in it.
.. news-push: RTPS
.. new-pop

.. news-pop

.. news-push: Notes
- This is a note we made.

  - This is detailed information about the note.

.. news-pop
