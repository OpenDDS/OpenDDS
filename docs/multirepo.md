# Domain and Repository Configuration Settings

With the ability to attach a process to more than a single DCPSInfoRepo
repository object, repository information and the associations between domains
and the repository that contains the information for a given domain become more
complex. Each repository needs to be made known to the application, and each
domain must be associated with a specific repository. DCPSInfoRepo objects
provide DDS service information to application `Service_Participant` objects for
one or more DDS domains.

The repository information and domain associations can be configured for the
DDS service using the command line, a configuration file, or the API. The
previously used defaults, command line arguments and configuration file
settings will work as is for existing applications that do not want to take
advantage of the new capability. The repository default or a repository
specified on the command line or in the common section of the configuration
file is used as the default repository.

Each repository that will be attached to an application must be specified. This
can be done via command line, configuration file, or using the API. In all
cases the information that must be specified is the IOR for the repository, and
a unique key for that repository within the given process.

Each domain that will be used for communication within a process must be
associated with a single repository that contains all information about that
specific domain. Any given repository may contain information on more than one
domain, but a domain may be associated with only a single repository. This
association is made by specifying it either in a configuration file or using
the API. No command line option exists to specify the association between a
domain and a repository.

A default repository is used when no specific information is given about a
repository or no specific association has been made between a domain and a
repository. This means that if no repository or domain information is specified
at all in the application, then the default repository IOR of `file://repo.ior`
will be used as the stringified IOR value for the repository used by all
domains within the process.

If a single repository is specified on the command line or using the
configuration file `[common]` section key DCPSInfoRepo, then that will be used
as the default repository for all domains which are not explicitly bound to a
different repository. If a domain is not explicitly associated with a
repository, it will be associated with the default repository.

The API support for configuring repositories and associations of domains to
repositories is provided by the `Service_Participant` methods
`set_repo_ior("IOR", repo_key)` and `set_repo_domain( domain, repo_key)` where
"IOR" is the repository IOR or its location, `repo_key` is a unique key value
for that repository within the application and domain is a DDS domain to be
associated with a specific repository.

Configuration file specification for repository and domain associations with
repositories are contained in individual `[repository]` and `[domain]`
subsections within the configuration file. Each repository being specified
should reside in its own subsection. The subsection name is arbitrary and can
be used to document the use or intent of the repository being specified. Each
domain being associated with a repository within a configuration file should
reside in its own subsection. This subsection name is also arbitrary and can be
used for documentation purposes.

Subsections are specified using a slash separated path syntax, similar to
directory paths used on Unix systems and their derivatives. A repository
subsection should be on a path with the format `[repository/<NAME>]` where the
`repository/` is literal and the `<NAME>` is to be replaced by a unique
subsection name. Similarly a domain subsection would be specified as
`[domain/<NAME>]`. There may be any number of repository or domain sections
within a single configuration file.

Repository subsections require at least 2 key/value pairs to be specified
within each subsection. The required keys to specify values for are
`RepositoryIor` and `RepositoryKey`. The `RepositoryKey` values must be unique
for each repository within the entire configuration file. They also must not be
reused for a different repository through the use of the API.

Optional key/value pairs that may be specified in a repository subsection are
the `DCPSBitTransportIPAddress` and `DCPSBitTransportPort` keys. These are
identical to the existing keys for the common subsection but specify the
address and port to be bound by the transport used for the instant repository.
When these key values are used in the common subsection, they specify the
values to be bound to the transport used for the default repository.

While `RepositoryKey` values must be unique within a given process, there are
no constraints on the value between processes. That is there is no requirement
that key values be consistent or the same in different processes for specifying
a particular repository.

Domain subsections require 2, and only 2, key/value pairs to be specified
within each subsection. The keys to specify values for are `DomainId` and
`DomainRepoKey`. The `DomainRepoKey` values may be specified either in the
configuration file or later through the API, but must have been specified
before a `DomainParticipant` for the domain is created. The special value
`DEFAULT_REPO` can be used to associate a domain with the default repository
explicitly. If more than one subsection specifies an association between a
single domain and different repositories, only the last (in the file)
association will be honored.

## Examples

### Single Repo, Single Domain

This example defines a single repository to be used for all domains within the
application.

```
[common]
DCPSInfoRepo = file://repo3.ior
```

### Single Repo, Multiple Domains

This example defines a default repository and explicitly associates two domains
with it. Note that the [domain] subsections are redundant in this example,
since they would be associated with the default repository by default. These
sections can be considered as documentation in this case.

```
[common]
DCPSInfoRepo = file://repo1.ior

[domain/Subscriber]
DomainId = 411
DomainRepoKey = DEFAULT_REPO

[domain/Publisher]
DomainId = 511
DomainRepoKey = DEFAULT_REPO
```

This example defines a single repository (not the default) and associates two
domains with it. Note that other domains will be associated with the default
repository, which in the absence of a command line or API specification of the
default will be the default IOR of `file://repo.ior`.

```
[repository/local]
RepositoryKey = 311
RepositoryIor = file://repo2.ior

[domain/Subscriber]
DomainId      = 711
DomainRepoKey = 311

[domain/Publisher]
DomainId      = 811
DomainRepoKey = 311
```

### Multiple Repos, Multiple Domains

This example defines three different repositories and associates six domains
with them. Note how the repository key values are used to do this.

```
[repository/Top]
RepositoryKey = 0
RepositoryIor = file://repo1.ior

[repository/Middle]
RepositoryKey = 1
RepositoryIor = file://repo2.ior

[repository/Bottom]
RepositoryKey = 2
RepositoryIor = file://repo3.ior

[domain/Band]
DomainId      = 311
DomainRepoKey = 0

[domain/Information]
DomainId      = 411
DomainRepoKey = 0

[domain/Vmx]
DomainId      = 611
DomainRepoKey = 1

[domain/QuickieMart]
DomainId      = 711
DomainRepoKey = 1

[domain/Warnings]
DomainId      = 811
DomainRepoKey = 1

[domain/Emergencies]
DomainId      = 911
DomainRepoKey = 2
```
