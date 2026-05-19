.. news-prs: 5212
.. news-start-section: Additions
.. news-start-section: RtpsRelay
- When security is enabled, the RtpsRelay can cache the partitions of a client participant in a session and
use the cached partitions to forward the client's messages immediately in subsequent sessions before the client
completes endpoint discovery with the relay. New options introduced for this feature include:

  - ``-CertificateIdPattern`` specifies a regex pattern used to match client's certificate subject name. Only clients that
    match the pattern will have their partitions cached.
  - ``-SynchronizeAsyncDiscoveryCache`` specifies whether the RtpsRelay instances in the relay domain should synchronize
    their caches with each other.
  - ``-AsyncDiscoveryCacheTimeout`` specifies the duration for an inactive cache entry to be removed from the cache for
    local clients, i.e., clients that connect to this relay instance.
  - ``-AsyncDiscoveryRemoteCacheTimeout`` specifies the duration for an inactive cache entry to be removed from the
    cache for remote clients, i.e., clients that connect to other relay instances.
  - ``-LogAsyncDiscovery`` specifies whether to log the activities related to this feature.
.. news-end-section
.. news-end-section
