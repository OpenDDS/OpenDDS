#######
inspect
#######

The ``inspect`` tool is a way to monitor samples from OpenDDS XTypes writers with a matching topic, type name and domain.
It creates a recorder that will associate with any compatible discovered readers.
Any sample received by the recorder will be printed to stdout.

.. warning::

    ``inspect`` will only work with remote participants that can offer complete XTypes ``TypeObject``\s of the topic types being published.
    By default OpenDDS doesn't do this; see section 16.7.1 of the OpenDDS Developer's Guide for details.

******************
Command Line Usage
******************

::

    inspect [OPTIONS] TOPIC_NAME TYPE_NAME DOMAIN_ID
    inspect --help|-h|--version|-v

Positional Arguments
====================

``TOPIC_NAME``
    The name of the topic to listen for.

``TYPE_NAME``
    The full name (including any modules) of the topic type.
    This should NOT include a leading ``::``.

``DOMAIN_ID``
    The DDS Domain to participant in.

OPTIONS
=======

All OpenDDS command line options listed in section 7.2 of the OpenDDS Developer's Guide are also available.

``-h``, ``--help``
    Prints this same information

``-v``, ``--version``
    Displays the version. This is the same as OpenDDS's.

``-w``, ``--writer-count``
    Print number of associated writers when they change. Default is to not to.

``--samples COUNT``
    Wait for at least this number of samples and exit.
    May actually print more.
    Default is to print samples forever.

``--time SECONDS``
    Print samples for the given number of seconds and exit.
    Default is to print samples forever.

*************
Example Usage
*************

``DevGuideExamples/DCPS/Messenger`` can be modified to work with ``inspect`` by:

1. Generating complete ``TypeObject``\s by changing ``Messenger.mpc`` so that the file starts with::

    project(*idl): dcps {
      requires += no_opendds_safety_profile
      TypeSupport_Files {
        dcps_ts_flags += -Gxtypes-complete // <-- ADD THIS
        Messenger.idl
      }

2. Rerunning MPC to regenerate build files.
3. Adding delays of at least 10 seconds to before the publisher waits for the subscriber and after it writes the samples.
   This can be done using ``ACE_OS::sleep(10);``.
   Rebuild after this.
   This should be done because the messenger normally runs too fast for ``inspect`` to catch the samples.
4. Enabling OpenDDS to use the complete ``TypeObject``\s by replacing ``rtps.ini`` with:

.. code:: ini

    [common]
    DCPSDefaultDiscovery=rtps_disc
    DCPSGlobalTransportConfig=$file

    [rtps_discovery/rtps_disc]
    UseXTypes=complete

    [transport/the_rtps_transport]
    transport_type=rtps_udp


Once that's all set up, run ``./run_test.pl --rtps`` in ``DevGuideExamples/DCPS/Messenger`` and ``inspect "Movie Discussion List" "Messenger::Message" 42 --writer-count`` in a separate command line session.
This should result in ``inspect`` printing messages like these after a short period of time::

    struct Messenger::Message
      String8 from = "Comic Book Guy"
      String8 subject = "Review"
      Int32 subject_id = 99
      String8 text = "Worst. Movie. Ever."
      Int32 count = 0

If the delays are not added to publisher or the timing of the added delays need to be increased, then it could just print something like::

    Listening to 1 writer(s)
    Listening to 0 writer(s)
