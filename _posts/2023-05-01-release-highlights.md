---
layout: post
title: "OpenDDS 3.24 Highlights"
categories: news
author: OpenDDS
---

# Transition to Unity Foundation

[OpenDDS 3.24](https://github.com/OpenDDS/OpenDDS/releases/tag/DDS-3.24) marks the first release of OpenDDS not performed by [Object Computing, Inc. (OCI)](https://objectcomputing.com)!
As announced in {% post_url 2023-03-17-transition-to-unity %}, the administration of the OpenDDS Foundation passed from OCI to [Unity Foundation](https://unityfoundation.io) in March 2023.
The relevant points from the transition are:

* OpenDDS will remain open-source software.
* The OpenDDS Foundation will continue to develop OpenDDS and support the OpenDDS Community.
  OpenDDS Foundation will offer support contracts, training, consulting and open-source development related to OpenDDS.
  OpenDDS Foundation will not develop any proprietary code.
* OpenDDS Foundation has gained not-for-profit status.
* Unity Foundation will dedicate one engineer to the maintenance, development, and release of OpenDDS.
* Systems and processes administered by OCI on behalf of the OpenDDS Foundation will no longer be maintained.
  Unity Foundation will take over some of these systems and processes while others will be deprecated and eliminated to reduce overhead.

For more on OCI's role, see {% link foundation/minutes/2023-04-12.html %}.

As part of the transition, OpenDDS Foundation has adopted the following (tentative) calendar:

| Month     | Activity                |
|-----------|-------------------------|
| January   | Release and TAB Meeting |
| February  | Release Article         |
| March     | Topic Article/Webinar   |
| April     | Release and TAB Meeting |
| May       | Release Article         |
| June      | Townhall Webinar        |
| July      | Release and TAB Meeting |
| August    | Release Article         |
| September | Topic Article/Webinar   |
| October   | Release and TAB Meeting |
| November  | Release Article         |
| December  | Year in Review Webinar  |

The general plan is to have one release and [Technical Advisory Board (TAB) Meeting]({% link foundation/index.html %}) every quarter.
This will be followed by a news article (like this one) that goes over the highlights of current release.
In June and December, there will be webinars to get feedback from the community and reflect on progress.
In March and September, there will an article or webinar on some aspect of OpenDDS.

# OpenDDS Developer's Guide

A major accomplishment in OpenDDS 3.24 was the migration of the Developer's Guide from an Open Office Document that was hosted on a private server to [reStructuredText in the repository](https://github.com/OpenDDS/OpenDDS/tree/master/docs/devguide).
The Developer's Guide corresponding to the latest release is hosted at https://opendds.readthedocs.io/en/latest-release/.
There is also a version that follows the master branch at https://opendds.readthedocs.io/en/master.
Versions of the Developer's Guide corresponding to specific releases, e.g., https://opendds.readthedocs.io/en/dds-3.24/, can be viewed by using the Read the Docs version switcher (the book icon always at the bottom of the viewable part of the page), or by choosing from the versions on [the OpenDDS Read the Docs project page](https://readthedocs.org/projects/opendds/).

We believe this is a significant improvement for two reasons:
1. It makes it that much easier to write the documentation along side the code which increases the chance that the relevant documentation gets created.
2. The community can now edit the Developer's Guide.
   If you have a correction, please submit a pull request!

The conversion was performed by an automated process and certain elements did not translate well.
We will continue to correct errors as we find them.
However, if you find them before we do, please submit an issue or pull request.

Have we mentioned that you can now submit pull requests to change the OpenDDS Developer's Guide?

# TAO 2.2a

OCI's distribution of ACE/TAO, called TAO 2.2a, was previously the default version used by the `configure` script.
Performing a release of OpenDDS sometimes involved creating a release of TAO 2.2a which involves assets internal to OCI.
As OpenDDS Foundation no longer has access to these assets, we decided to decouple the release process from TAO 2.2a and make DOCGroup ACE6/TAO2 the default ACE/TAO moving forward.

# Release Tarballs

Another change related to the transition from OCI is the availability of release tarballs.
Posting release tarballs to http://download.ociweb.com was a practice that was established before OpenDDS was hosted on GitHub.
This practice has been discontinued since we no longer have access to the server and the release tarballs are available on GitHub.

# OpenDDS can now be cross-compiled on MacOS

Currently, MacOS supports different processor types (Intel and ARM).
Assume an Intel-based build host and that a native build of OpenDDS is available in `$HOST_DDS`.
To build for an ARM processor, one could use the `configure` script as follows:

    ./configure --target=macos-cross --host-tools=$HOST_DDS --target-arch=arm64-apple-macos

# Anonymous Types in Unions

Anonymous sequences and arrays can now be used in union branches.
This example demonstrates what is now possible:

    union UnionType switch (int32) {
    case 0:
      sequence<uint16> anon_u16_seq;
    case 1:
      uint32 anon_u32_array[2];
    };

# Content Subscription for `DynamicDataReader`s

[Query conditions](https://opendds.readthedocs.io/en/latest-release/devguide/content_subscription_profile.html#query-condition) allow the middleware to filter samples using a SQL-like expression.
[Content-Filtered Topics](https://opendds.readthedocs.io/en/latest-release/devguide/content_subscription_profile.html#content-filtered-topic) allow the user to apply a filter expression to create a new topic.
These features are now supported for `DynamicDataReader`s and `DynamicDataWriter`s can also take advantage of content filtering to make communication more efficient in certain circumstances.
As shown in the [QueryCondition test](https://github.com/OpenDDS/OpenDDS/tree/master/tests/DCPS/QueryCondition) and [ContentFilteredTopic test](https://github.com/OpenDDS/OpenDDS/tree/master/tests/DCPS/ContentFilteredTopic), `create_querycondition` can be called on a `DynamicDataReader` and that `create_datareader` can be called with a content-filtered topic that was created with dynamic type support.
This work is one more step toward feature parity between dynamic `DataWriter`s and `DataReader`s and their statically typed counterparts.

# Secure writers and reader in the same participant can now associate

The test battery contains a scenario where a secure participant creates both a writer and a reader that associate.
However, the security settings were so lax that they tested very little.
With more strict security settings, the association failed because information that was normally generated during secure discovery was not available.
The fix was to bypass authentication and key exchange in this case.

# RtpsRelay Hardening

The RtpsRelay is a horizontally scalable service that forwards RTPS packets.
It is used to connect (secure) OpenDDS participants that need to operate over the Internet.
As such, the RtpsRelay is exposed to some of the adverse conditions on the Internet.
OpenDDS 3.24.0 adds two new features to deal with these conditions.

First, the `-MaxAddrSetSize` controls the maximum number of IP:port pairs that the RtpsRelay will maintain for a client participant.
Usually, packets from a client participant will arrive from a single IP:port.
There are even legitimate cases, like when a mobile phone transitions to or from WiFi, where a client participant may be using multiple IP:port pairs to communicate.
However, some routers and network configurations are broken and use a different outgoing port for each packet.
This options allows such packets to be rejected when the number of IP:port pairs for a client participant exceeds the given limit and limits the size of the internal maps used by the RtpsRelay.
IP:port pairs that are inactive are eventually purged from the internal maps (see the `-Lifespan` option).

Second, some broken routers reflect all packets sent by the RtpsRelay.
If this behavior is detected, then the offending source IP address is put on a reject list for the time specified by `-RejectedAddressDuration`.

# What's next?

Our goal for each quarterly release is to add or expand one or two features of general interest to the OpenDDS Community.
For our next release, we will be working on:

* CMake Support
  * Make it easy for users to use OpenDDS from their CMake-based applications.
  * Compile OpenDDS using CMake.
    Our goal here is to increase the pool of potential contributors by switching to a more modern build system.
* Configuration Overhaul
  * Consistency between what can be configured on the command line vs. configuration file vs. API.
  * Add the ability to configure OpenDDS using environment variables as environment variable are used extensively in container-based deployments.
  * Add a generic configuration API that is easily wrapped for different language bindings so that everything that can be done in C++ can also be done in other languages.
  * Provide a generic approach for components that support dynamic reconfiguration.
