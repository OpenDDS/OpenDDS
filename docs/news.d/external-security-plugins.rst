# This will have to be replaced with the PR number after the PR is created:
.. news-prs: 0

.. news-start-section: Additions
- DDS Security plugins can now be loaded from external shared libraries and combined with the built-in plugins.

  - New ``[security_plugin]`` config sections declare loadable plugin libraries and ``[security]`` sections select the plugin used for each security interface.
  - The new ``DCPSGlobalSecurityConfig`` common setting selects a process-wide security configuration, analogous to ``DCPSGlobalTransportConfig``.

.. news-end-section
