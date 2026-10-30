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

* The DDS Specification and, more specifically, the RTPS Specification made no provisions for security and it was assumed that applications run in a "secure" network with trusted participants.
  Bugs reported for OpenDDS without security enabled, therefore, will not be treated as vulnerability.
* DDS Security Specification was developed for non-secure environments.
  If the vulnerability is intrinsic to the DDS/RTPS Specification and using DDS Security would address it, then the maintainers of OpenDDS will most likely not accept the vulnerability report and suggest using DDS Security.
  In other words, OpenDDS deployments on non-secure environments should have DDS Security enabled.
* Is the problem inherent to the DDS Specification or DDS Security Specification?
  First, the problem should be reported to OMG as all conforming implementations will suffer from the same vulnerability.
  Second, the maintainers of OpenDDS will evaluate the severity of the vulnerability to determine if a correction that goes against the specification is warranted.
* If the problem is not related to specification-defined behavior, then the vulnerability is likely a bug in OpenDDS and will most likely be accepted.
  Note: Since OpenDDS is a toolkit for building many types of applications, it contains code that is not intended to be used in secured deployments (such as DCPSInfoRepo) and we will not treat any bugs in that code as vulnerabilities.

See https://opendds.readthedocs.io/en/latest-release/devguide/internet_enabled_rtps.html#security-considerations.

Accepted vulnerabilities will be published after the security update is applied to the master branch.
