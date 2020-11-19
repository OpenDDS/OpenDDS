# OpenDDS Security

This document is intended as a collection of high-level notes on OpenDDS
Security. OpenDDS Security provides security controls and capabilities for
RTPS, which includes authentication of participants and encryption of messages.
It an implementation of DDS Security Version 1.1 (OMG formal/2018-04-01).

## Debug Logging

OpenDDS Security has debug messages like the rest of OpenDDS, many are under
the `DCPSSecurityDebug` but, as of writing, there are sill many messages still
under `DCPSDebugLevel` and `DCPSTransportDebugLevel` and need to be
transitioned. Security debug messages are broken broken down into named
categories which can be specified individually or activated on the same
accumulative 1 to 10 scale as `DCPSDebugLevel`.

See OpenDDS Developer's Guide Section 7.2 "Common Configuration Options" for
the categories and usage, or `dds/DCPS/debug.h` and `dds/DCPS/debug.cpp` for
the source.

## Authentication

Authentication occurs after participants discover each other. It consists of a
three-way handshake using the "builtin SDP participant" readers and writers.

- Request
  - Based on GUIDs, one side takes the lead and starts the handshake by sending
    their security documents. We can call this participant the leader, with the
    other side being the follower.
- Reply
  - The follower will verify the leader and send their own documents back.
- Final
  - Finally the leader will verify the follower and send a "final" message
    back, signaling that association can continue.

The role taken in the authentication handshake can be overridden using
`OpenDDS::DCPS::security_debug.force_auth_role`.

<!-- TODO: List OMG authentication states and what they actually mean -->

## Key Exchange

During key exchange all keys of secure entities will be exchanged using the
"Builtin Participant Volatile Message Secure" topic. It can be thought of as a
boot strap to the normal security.

- Because they are special keys, "Volitile" keys all have the key id `00 00 00 00`.

- Each side should send at least 8 keys, one for each secure entity.

- `bookkeeping` security debug logging category will show when these keys are
  generated and exchanged. `showkeys` security debug logging category will log
  the keys themselves.

## Fake Encryption

Anyone debugging OpenDDS security to any significant extent will probably find
the fake encryption option useful. It disables all encryption in OpenDDS
Security, but leaves the rest of the infrastructure alone. This allows one to
see what is being sent in Wireshark.

To enable it, pass `-DCPSSecurityFakeEncryption=1` to all programs using
security. If all participants are not set the same, it will cause security to
fail. It can also be set in the common section of ini files. Refer to the RTPS
and DDS Security specs for the structure of secure RTPS packets to aid in
manually demarshaling them.
