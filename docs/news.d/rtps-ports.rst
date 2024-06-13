.. news-prs: 4655

.. news-start-section: Additions
- Added config properties to give more control over what UDP ports RTPS uses:

  - For RTPS Discovery:

    - Added :cfg:val:`[rtps_discovery]SedpPortMode=probe`, which will use ports similar to how the RTPS specification defines them.
      This uses the existing port parameter properties for SPDP and a new one, :cfg:prop:`[rtps_discovery]DY`.
    - Added :cfg:prop:`[rtps_discovery]SpdpPortMode` as an alias to the now deprecated :cfg:prop:`[rtps_discovery]SpdpRequestRandomPort`.
    - Added :cfg:prop:`[rtps_discovery]SpdpMulticastAddress`, :cfg:prop:`[rtps_discovery]Ipv6SpdpMulticastAddress`, :cfg:prop:`[rtps_discovery]SedpMulticastAddress`, and :cfg:prop:`[rtps_discovery]Ipv6SedpMulticastAddress` to set the multicast addresses and ports separately on SPDP and SEDP.
    - See :ref:`here <config-ports-used-by-rtps-disc>` for the full overview of port usage.

  - For RTPS/UDP Transport:

    - Added :cfg:val:`[transport@rtps_udp]PortMode=probe`, which will use ports exactly as the RTPS specification defines them.
      This uses new port parameter properties: :cfg:prop:`[transport@rtps_udp]PB`, :cfg:prop:`[transport@rtps_udp]DG`, :cfg:prop:`[transport@rtps_udp]PG`, :cfg:prop:`[transport@rtps_udp]D2`, and :cfg:prop:`[transport@rtps_udp]D3`.
    - See :ref:`here <config-ports-used-by-rtps-udp>` for the full overview of port usage.

  - All ports calculated using port parameter properties now warn if they overflow the 16 bit integer.
  - :cfg:val:`[rtps_discovery]SedpPortMode=probe` and :cfg:val:`[transport@rtps_udp]PortMode=probe` might make :ref:`config template customizations <run_time_configuration--adding-customizations>` unnecessary.
.. news-end-section
