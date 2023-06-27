.. news-prs: 4180
.. news-start-section: Fixes
- Fixed bug introduced by #4120

  - The fix introduced in #4120 causes the TransportClient to silently drop messages when the client's guid is not initialized.
    This causes issues for TransportClients that send messages to the transport before association.
    One such example is a DataWriter with liveliness configured.
    The DataWriter will send liveliness messages to the transport (which will be dropped) and hang waiting for them to be delivered.
  - The solution was set the guid for a TransportClient before calling any method that uses the guid.
.. news-end-section
