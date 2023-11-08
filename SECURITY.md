# Security Policy

## Supported Versions

Security updates are applied to the master branch and made available with the next release.
A reported vulnerability may result in a minor or patch release.
A user that wants to apply a security update to a previous release may do so by creating a pull request.

| Version | Supported          |
| ------- | ------------------ |
| 3.x     | :white_check_mark: |

## Reporting a Vulnerability

Vulnerabilities can be reported at https://github.com/OpenDDS/OpenDDS/security.

Vulnerabilities will be evaluated against the master branch.

Vulnerabilities are evaluated along the following dimensions:

* Would using DDS Security solve the problem?
  The DDS Specification and, more specifically, the RTPS Specification made no provisions for security and it was assumed that applications run in a "secure" network.
  This led to the creation of the DDS Security Specification.
  If the vulnerability is intrinsic to the DDS/RTPS Specification and using DDS Security would address it, then the maintainers of OpenDDS will most likely not accept the vulnerability report and suggest using DDS Security.
* Is the problem inherent to the DDS Specification or DDS Security Specification?
  First, the problem should be reported to OMG as all conforming implementations will suffer from the same vulnerability.
  Second, the maintainers of OpenDDS will evaluate the severity of the vulnerability to determine if a correction that goes against the specification is warranted.
* If the problem is not related to specification-defined behavior, then the vulnerability is likely a bug in OpenDDS and will most likely be accepted.

See https://opendds.readthedocs.io/en/latest-release/devguide/internet_enabled_rtps.html#security-considerations.

Accepted vulnerabilities will be published after the security update is applied to the master branch.
