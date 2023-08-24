.. news-prs: 4162
.. news-start-section: Additions
- OpenDDS now stores ``rtps_udp_transport`` configuration in the key-value store.
  The following members of ``RtpsUdpInst`` must now be accessed with getters and setters:

  -  ``send_buffer_size_``
  -  ``rcv_buffer_size_``
  -  ``use_multicast_``
  -  ``ttl_``
  -  ``multicast_interface_``
  -  ``anticipated_fragments_``
  -  ``max_message_size_``
  -  ``nak_depth_``
  -  ``nak_response_delay_``
  -  ``heartbeat_period_``
  -  ``receive_address_duration_``
  -  ``responsive_mode_``

.. news-end-section
