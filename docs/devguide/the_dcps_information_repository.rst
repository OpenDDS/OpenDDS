.. _the_dcps_information_repository:
.. _inforepo:

###############################
The DCPS Information Repository
###############################

..
    Sect<9>

The DCPS Information Repository is a non-interoperable and OpenDDS-specific service to distribute discovery information.
See :ref:`run_time_configuration--configuring-for-ddsi-rtps-discovery` for interoperable discovery.
The DCPS Information Repository is scheduled for deprecation with OpenDDS 4 and scheduled for removal with OpenDDS 5.

There is no interoperability guarantee between multiple OpenDDS releases when you are using the DCPS Information Repository.
All applications using a specific DCPS Information Repository must use the same OpenDDS release as that DCPS Information Repository.

.. _the_dcps_information_repository--dcps-information-repository-options:

***********************************
DCPS Information Repository Options
***********************************

..
    Sect<9.1>

The table below shows the command line options for the ``DCPSInfoRepo`` server:

.. list-table:: DCPS Information Repository Options
   :header-rows: 1

   * - Option

     - Description

     - Default

   * - ``-o file``

     - Write the IOR of the ``DCPSInfo`` object to the specified file

     - ``repo.ior``

   * - ``-NOBITS``

     - Disable the publication of built-in topics

     - Built-in topics are published

   * - ``-a address``

     - Listening address for built-in topics (when built-in topics are published).

     - Random port

   * - ``-z``

     - Turn on verbose transport logging

     - Minimal transport logging.

   * - ``-r``

     - Resurrect from persistent file

     - ``1`` (true)

   * - ``-FederationId <id>``

     - Unique identifier for this repository within any federation.
       This is supplied as a 32 bit decimal numeric value.

     - N/A

   * - ``-FederateWith <ref>``

     - Repository federation reference at which to join a federation.
       This is supplied as a valid CORBA object reference in string form: stringified IOR, file: or corbaloc: reference string.

     - N/A

   * - ``-?``

     - Display the command line usage and exit

     - N/A

OpenDDS clients often use the IOR file that ``DCPSInfoRepo`` outputs to locate the service.
The ``-o`` option allows you to place the IOR file into an application-specific directory or file name.
This file can subsequently be used by clients with the ``file://`` IOR prefix.

Applications that do not use built-in topics may want to disable them with ``-NOBITS`` to reduce the load on the server.
If you are publishing the built-in topics, then the ``-a`` option lets you pick the listen address of the tcp transport that is used for these topics.

Using the ``-z`` option causes the invocation of many transport-level debug messages.
This option is only effective when the DCPS library is built with the ``DCPS_TRANS_VERBOSE_DEBUG`` environment variable defined.

The ``-FederationId`` and ``-FederateWith`` options are used to control the federation of multiple ``DCPSInfoRepo`` servers into a single logical repository.
See :ref:`the_dcps_information_repository--repository-federation` for descriptions of the federation capabilities and how to use these options.

File persistence is implemented as an ACE Service object and is controlled via service config directives.
Currently available configuration options are:

.. _the_dcps_information_repository--reftable31:

.. list-table:: InfoRepo persistence directives
   :header-rows: 1

   * - Options

     - Description

     - Defaults

   * - ``-file``

     - Name of the persistent file

     - ``InforepoPersist``

   * - ``-reset``

     - Wipe out old persistent data.

     - ``0`` (false)

The following directive:

.. code-block:: cpp

    static PersistenceUpdater_Static_Service "-file info.pr -reset 1"

will persist ``DCPSInfoRepo`` updates to local file ``info.pr``.
If a file by that name already exists, its contents will be erased.
Used with the command-line option ``-r``, the ``DCPSInfoRepo`` can be reincarnated to a prior state.
When using persistence, start the ``DCPSInfoRepo`` process using a TCP fixed port number with the following command line option.
This allows existing clients to reconnect to a restarted InfoRepo.

.. code-block:: bash

    -ORBListenEndpoints iiop://:<port>

.. _the_dcps_information_repository--repository-federation:

*********************
Repository Federation
*********************

..
    Sect<9.2>

.. note:: Repository federation should be considered an experimental feature.

Repository Federation allows multiple DCPS Information Repository servers to collaborate with one another into a single federated service.
This allows applications obtaining service metadata and events from one repository to obtain them from another if the original repository is no longer available.

While the motivation to create this feature was the ability to provide a measure of fault tolerance to the DDS service metadata, other use cases can benefit from this feature as well.
This includes the ability of initially separate systems to become federated and gain the ability to pass data between applications that were not originally reachable.
An example of this would include two platforms which have independently established internal DDS services passing data between applications; at some point during operation the systems become reachable to each other and federating repositories allows data to pass between applications on the different platforms.

The current federation capabilities in OpenDDS provide only the ability to statically specify a federation of repositories at startup of applications and repositories.
A mechanism to dynamically discover and join a federation is planned for a future OpenDDS release.

OpenDDS automatically detects the loss of a repository by using the :ref:`qos-liveliness` policy on a Built-in Topic.
When a federation is used, the liveliness QoS policy is modified to a non-infinite value.
When liveliness is lost for a Built-in Topic an application will initiate a failover sequence causing it to associate with a different repository server.
Because the federation implementation currently uses a Built-in Topic ``ParticipantDataDataReaderListener`` entity, applications should not install their own listeners for this topic.
Doing so would affect the federation implementation's capability to detect repository failures.

The federation implementation distributes repository data within the federation using a reserved DDS domain.
The default domain used for federation is defined by the constant ``Federator::DEFAULT_FEDERATIONDOMAIN``.

Currently only static specification of federation topology is available.
This means that each DCPS Information Repository, as well as each application using a federated DDS service, needs to include federation configuration as part of its configuration data.
This is done by specifying each available repository within the federation to each participating process and assigning each repository to a different key value in the configuration files as described in :ref:`run_time_configuration--configuring-for-multiple-dcpsinforepo-instances`.

Each application and repository must include the same set of repositories in its configuration information.
Failover sequencing will attempt to reach the next repository in numeric sequence (wrapping from the last to the first) of the repository key values.
This sequence is unique to each application configured, and should be different to avoid overloading any individual repository.

Once the topology information has been specified, then repositories will need to be started with two additional command line arguments.
These are shown in :ref:`the_dcps_information_repository--dcps-information-repository-options`.
One, ``-FederationId <value>``, specifies the unique identifier for a repository within the federation.
This is a 32 bit numeric value and needs to be unique for all possible federation topologies.

The second command line argument required is ``-FederateWith <ref>``.
This causes the repository to join a federation at the <ref> object reference after initialization and before accepting connections from applications.

Only repositories which are started with a federation identification number may participate in a federation.
The first repository started should not be given a ``-FederateWith`` command line directive.
All others are required to have this directive in order to establish the initial federation.
There is a command line tool (``federation``) supplied that can be used to establish federation associations if this is not done at startup.
See :ref:`the_dcps_information_repository--federation-management` for a description.
It is possible, with the current static-only implementation, that the failure of a repository before a federation topology is entirely established could result in a partially unusable service.
Due to this current limitation, it is highly recommended to always establish the federation topology of repositories prior to starting the applications.

.. _the_dcps_information_repository--federation-management:

Federation Management
=====================

..
    Sect<9.2.1>

A new command line tool has been provided to allow some minimal run-time management of repository federation.
This tool allows repositories started without the ``-FederateWith`` option to be commanded to participate in a federation.
Since the operation of the federated repositories and failover sequencing depends on the presence of connected topology, it is recommended that this tool be used before starting applications that will be using the federated set of repositories.

The command is named ``repoctl`` and is located in the :ghfile:`bin/` directory.
It has a command format syntax of:

.. code-block:: bash

       repoctl <cmd> <arguments>

Some options contain endpoint information.
This information consists of an optional host specification, separated from a required port specification by a colon.
This endpoint information is used to create a CORBA object reference using the corbaloc: syntax in order to locate the 'Federator' object of the repository server.

.. _the_dcps_information_repository--reftable32:

.. list-table:: repoctl Repository Management Command
   :header-rows: 1

   * - Command

     - Syntax

     - Description

   * - ``join``

     - ``repoctl join <target> <peer> [ <federation domain> ]``

     - Calls the ``<peer>`` to join ``<target>`` to the federation.
       ``<federation domain>`` is passed if present, or the default Federation Domain value is passed.

   * - ``leave``

     - ``repoctl leave <target>``

     - Causes the ``<target>`` to gracefully leave the federation, removing all managed associations between applications using ``<target>`` as a repository with applications that are not using ``<target>`` as a repository.

   * - ``shutdown``

     - ``repoctl shutdown <target>``

     - Causes the ``<target>`` to shutdown without removing any managed associations.
       This is the same effect as a repository which has crashed during operation.

   * - ``kill``

     - ``repoctl kill <target>``

     - Kills the ``<target>`` repository regardless of its federation status.

   * - ``help``

     - ``repoctl help``

     - Prints a usage message and quits.

A join command specifies two repository servers (by endpoint) and asks the second to join the first in a federation:

.. code-block:: bash

       repoctl join 2112 otherhost:1812

This generates a CORBA object reference of ``corbaloc::otherhost:1812/Federator`` that the federator connects to and invokes a join operation.
The join operation invocation passes the default Federation Domain value (because we did not specify one) and the location of the joining repository which is obtained by resolving the object reference ``corbaloc::localhost:2112/Federator``.

.. _the_dcps_information_repository--reftable33:

.. list-table:: Federation Management Command Arguments
   :header-rows: 1

   * - Option

     - Description

   * - ``<target>``

     - This is endpoint information that can be used to locate the ``Federator::Manager`` CORBA interface of a repository which is used to manage federation behavior.
       This is used to command leave and shutdown federation operations and to identify the joining repository for the join command.

   * - ``<peer>``

     - This is endpoint information that can be used to locate the ``Federator::Manager`` CORBA interface of a repository which is used to manage federation behavior.
       This is used to command join federation operations.

   * - ``<federation domain>``

     - This is the domain specification used by federation participants to distribute service metadata amongst the federated repositories.
       This only needs to be specified if more than one federation exists among the same set of repositories, which is currently not supported.
       The default domain is sufficient for single federations.

.. _the_dcps_information_repository--federation-example:

Federation Example
==================

..
    Sect<9.2.2>

To illustrate the setup and use of a federation, this section walks through a simple example that establishes a federation and a working service that uses it.

This example is based on a two repository federation, with the simple Message publisher and subscriber from :ref:`getting_started--using-dcps` configured to use the federated repositories.

.. _the_dcps_information_repository--configuring-the-federation-example:

Configuring the Federation Example
----------------------------------

..
    Sect<9.2.2.1>

There are two configuration files to create for this example one each for the message publisher and subscriber.

The Message Publisher configuration ``pub.ini`` for this example is as follows:

.. code-block:: ini

    [common]
    DCPSDebugLevel=0

    [domain/information]
    DomainId=42
    DomainRepoKey=1

    [repository/primary]
    RepositoryKey=1
    RepositoryIor=corbaloc::localhost:2112/InfoRepo

    [repository/secondary]
    RepositoryKey=2
    RepositoryIor=file://repo.ior

Note that the ``DCPSInfo`` attribute/value pair has been omitted from the ``[common]`` section.
The user domain is 42, so that domain is configured to use the primary repository for service metadata and events.

The ``[repository/primary]`` and ``[repository/secondary]`` sections define the primary and secondary repositories to use within the federation (of two repositories) for this application.
The ``RepositoryKey`` attribute is an internal key value used to uniquely identify the repository (and allow the domain to be associated with it, as in the preceding ``[domain/information]`` section).
The ``RepositoryIor`` attributes contain string values of resolvable object references to reach the specified repository.
The primary repository is referenced at port 2112 of the ``localhost`` and is expected to be available via the TAO ``IORTable`` with an object name of ``/InfoRepo``.
The secondary repository is expected to provide an IOR value via a file named ``repo.ior`` in the local directory.

The subscriber process is configured with the ``sub.ini`` file as follows:

.. code-block:: ini

    [common]
    DCPSDebugLevel=0

    [domain/information]
    DomainId=42
    DomainRepoKey=1

    [repository/primary]
    RepositoryKey=1
    RepositoryIor=file://repo.ior

    [repository/secondary]
    RepositoryKey=2
    RepositoryIor=corbaloc::localhost:2112/InfoRepo

Note that this is the same as the ``pub.ini`` file except the subscriber has specified that the repository located at port 2112 of the ``localhost`` is the secondary and the repository located by the ``repo.ior`` file is the primary.
This is opposite of the assignment for the publisher.
It means that the publisher is started using the repository at port 2112 for metadata and events while the subscriber is started using the repository located by the IOR contained in the file.
In each case, if a repository is detected as unavailable the application will attempt to use the other repository if it can be reached.

The repositories do not need any special configuration specifications in order to participate in federation, and so no files are required for them in this example.

.. _the_dcps_information_repository--running-the-federation-example:

Running the Federation Example
------------------------------

..
    Sect<9.2.2.2>

The example is executed by first starting the repositories and federating them, then starting the application publisher and subscriber processes the same way as was done in the example of :ref:`getting_started--running-the-example`.

Start the first repository as:

.. code-block:: bash

    $DDS/bin/DCPSInfoRepo -o repo.ior -FederationId 1024

The ``-o repo.ior`` option ensures that the repository IOR will be placed into the file as expected by the configuration files.
The ``-FederationId 1024`` option assigns the value 1024 to this repository as its unique id within the federation.

Start the second repository as:

.. code-block:: bash

    $DDS/bin/DCPSInfoRepo \
      -ORBListenEndpoints iiop://localhost:2112 \
      -FederationId 2048 -FederateWith file://repo.ior

Note that this is all intended to be on a single command line.
The ``-ORBListenEndpoints iiop://localhost:2112`` option ensures that the repository will be listening on the port that the previous configuration files are expecting.
The ``-FederationId 2048`` option assigns the value 2048 as the repositories unique id within the federation.
The ``-FederateWith file://repo.ior`` option initiates federation with the repository located at the IOR contained within the named file - which was written by the previously started repository.

Once the repositories have been started and federation has been established (this will be done automatically after the second repository has initialized), the application publisher and subscriber processes can be started and should execute as they did for the previous example in :ref:`getting_started--running-the-example`.
