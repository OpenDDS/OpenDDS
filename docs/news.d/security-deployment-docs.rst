.. news-prs: 5341

.. news-start-section: Documentation
- Added a warning to :ref:`dds_security--metadata-protection-kind` that topics with neither metadata nor RTPS protection have no protection against replayed or forged RTPS submessages.
- Documented that the RtpsRelay's Relay Domain doesn't use DDS Security and must be isolated from clients and the public Internet.
  See :ref:`internet_enabled_rtps--deployment-considerations`.
- Documented that the DCPS Information Repository and the transports other than ``rtps_udp`` don't support DDS Security and must only be used on trusted networks.
- Documented that listener operations can still be running after a listener is replaced or its entity is deleted, and that listeners must not be deleted directly.

.. news-end-section
