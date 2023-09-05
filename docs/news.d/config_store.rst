.. news-prs: 4162 4241 4242
.. news-start-section: Additions
- OpenDDS now stores ``rtps_udp`` transport configuration in the key-value store.
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

- OpenDDS now stores ``multicast`` transport configuration in the key-value store.
  The following members of ``MulticastInst`` must now be accessed with getters and setters:

  -  ``default_to_ipv6_``
  -  ``port_offset_``
  -  ``group_address_``
  -  ``local_address_``
  -  ``reliable_``
  -  ``syn_backoff_``
  -  ``syn_interval_``
  -  ``syn_timeout_``
  -  ``nak_depth_``
  -  ``nak_interval_``
  -  ``nak_delay_intervals_``
  -  ``nak_max_``
  -  ``mak_timeout_``
  -  ``ttl_``
  -  ``rcv_buffer_size_``
  -  ``async_send_``

- OpenDDS now stores ``shmem`` transport configuration in the key-value store.
  The following members of ``ShmemInst`` must now be accessed with getters and setters:

  -  ``pool_size_``
  -  ``datalink_control_size_``

.. news-end-section
