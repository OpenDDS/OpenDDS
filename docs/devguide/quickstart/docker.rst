######
Docker
######

This guide will take you through the process of compiling and runnning an OpenDDS application with Docker.
Docker images containing a pre-built OpenDDS are available in `GitHub Container Registry <https://github.com/OpenDDS/OpenDDS/pkgs/container/opendds>`__.
An image corresponding to a particular release has a tag of the form ``DDS-X.xx``, e.g., ``DDS-3.23``.

*************
Prerequisites
*************

Check that ``docker`` and ``docker-compose`` are installed.

.. code-block:: bash

   docker --version
   docker-compose --version

*****************************
Compile the Messenger Example
*****************************

#. Enter a container

   .. code-block:: bash

      docker run --rm -ti -v "$PWD:/opt/workspace" ghcr.io/opendds/opendds:latest-release

#. Copy the ``Messenger`` directory

   .. code-block:: bash

      cp -R /opt/OpenDDS/DevGuideExamples/DCPS/Messenger Messenger
      cd Messenger

#. Configure and build the Messenger example

   .. code-block:: bash

      source /opt/OpenDDS/setenv.sh
      mwc.pl -type gnuace
      make

#. Exit the container

   .. code-block:: bash

      exit

***********************************
Run the Messenger Example with RTPS
***********************************

#. Enter the ``Messenger`` directory

   .. code-block:: bash

      cd Messenger

#. Run the Messenger example with RTPS

   .. code-block:: bash

      docker-compose up

***************************************
Run the Messenger Example with InfoRepo
***************************************

#. Enter the ``Messenger`` directory if not done as part of the previous section

   .. code-block:: bash

      cd Messenger

#. Run the Messenger example with InfoRepo

   .. code-block:: bash

      docker-compose -f docker-compose-inforepo.yml up

#. Use Control-C to kill the InfoRepo process

**********
Next Steps
**********

See :ref:`getting_started` for a detailed explanation of the Messenger C++ Example or :ref:`java` for the Java Example.
