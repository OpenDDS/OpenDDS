.. news-prs: 0

.. news-start-section: Security
- The built-in crypto plugin now rejects a protected submessage whose ``octetsToNextHeader`` runs past the end of the received data, instead of reading the ``SEC_POSTFIX`` and authenticated bytes from beyond the buffer.

.. news-end-section
