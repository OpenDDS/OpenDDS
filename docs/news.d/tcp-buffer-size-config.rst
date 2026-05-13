.. news-prs: 5215

.. news-start-section: Additions
- Added :cfg:prop:`[transport@tcp]send_buffer_size` and
  :cfg:prop:`[transport@tcp]rcv_buffer_size`. By default, the TCP transport
  now leaves socket buffer sizing to the platform unless these are set to
  positive values.

.. news-end-section
