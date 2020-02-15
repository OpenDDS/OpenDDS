C++ API Reference
=================

Preprocessor Macros
-------------------

.. var:: OpenDDS::DCPS::ServiceParticipant* TheServiceParticipant

  Get a pointer to the global :class:`OpenDDS::DCPS::ServiceParticipant`
  singleton instance.

.. var:: DDS::DomainParticipantFactory* TheParticipantFactory

  Get a pointer to the global :class:`DDS::DomainParticipantFactory` singleton
  instance. Initializes OpenDDS if not already done.

.. function:: DDS::DomainParticipantFactory* \
    TheParticipantFactoryWithArgs(int& argc, ACE_TCHAR** argv)

  Get a pointer to the global :class:`DDS::DomainParticipantFactory` singleton
  instance. Initializes OpenDDS with the given arguments if not already done.
  Takes valid ``-DCPS`` arguments out of ``argv`` and adjusts ``argc``
  accordingly.

.. type:: ACE_TCHAR

  Is defined by ACE as ``char`` normally unless ACE was configured with
  ``ACE_USES_WCHAR``, then it is defined as ``wchar``.

``namespace DDS``
-----------------
.. namespace-push:: DDS

.. class:: DomainParticipantFactory

  Factory for :class:`DomainParticipant`.

  .. function:: \
    DomainParticipant create_participant( \
      DomainId_t domainId, \
      DomainParticipantQos qos, \
      DomainParticipantListener a_listener, \
      StatusMask mask)

    Create a new :class:`DomainParticipant`.

  .. function:: \
      ReturnCode_t delete_participant(DomainParticipant a_participant)

    Delete a Participant.

  .. function:: \
      DomainParticipant lookup_participant(DomainId_t domainId)

    Lookup a Participant.

  .. function:: \
      ReturnCode_t set_default_participant_qos(DomainParticipantQos qos)

    Set Default Participant QoS.

  .. function:: \
      ReturnCode_t get_default_participant_qos(DomainParticipantQos qos)

    Get Default Participant QoS.

  .. function:: \
      DomainParticipantFactory get_instance()

    Get Instance.

  .. function:: \
      ReturnCode_t set_qos(DomainParticipantFactoryQos qos)

    Set QoS.

  .. function:: \
      ReturnCode_t get_qos(DomainParticipantFactoryQos qos)

    Get QoS.

.. class:: DomainParticipant

  A Domain Participant

.. namespace-pop::

``namespace OpenDDS``
---------------------------

.. namespace-push:: OpenDDS

``namespace DCPS``
***************************
.. namespace-push:: DCPS

.. class:: ServiceParticipant

  The Service Participant.

  .. function:: void default_configuration_file(const ACE_TCHAR* path)

    Set a configuration file to use if ``-DCPSConfigFile`` wasn't passed to
    :any:`TheParticipantFactoryWithArgs`. Must be used before
    :any:`TheParticipantFactory` or :any:`TheParticipantFactoryWithArgs` are
    called.

.. namespace-pop::

.. namespace-pop::
