PROBLEM
=======
When using QoS policies via XML file, specifically for the topic policies, the  ``durability_service`` field is ignored by the QOS_XML_Handler component. This results in passing to the topic a ``TopicQos`` structure where the ``durability_service`` field is uninitialized, causing a failure in the topic's creation.

PROBLEM CONSIDERATION
---------------------
* The OpenDDS QoS XML documentation specifically mentions the possibility to add the ``durability_service`` element in the Topic QoS policy.
* Others elements might be affected as well.
* The problem seems to lie in the use of function ``DwDrTpBase::read_qos`` instead of function ``DwTpBase::read_qos`` in file ``dds/DCPS/QOS_XML_Handler/QOS_Topic_T.cpp:23``

PROOF OF CONCEPT - WORKING EXAMPLE
==================================
The POC consists of a Subscriber that loads the QoS policies from the ``qos.xml`` file, creates the domain participant, the topic, then subscriber and the data reader, and then terminates. The topic creation function call fails because we passed the incriminated QoS policies loaded from file.
There are 2 error messages:

* a ``>>> BUG_REPORT_ERROR <<<`` message when the Topic is unsuccessfully created, with reported part of the uninitialized component (added by me)
* an error message from OpenDDS itself (``(10500|10500) NOTICE: Qos_Helper::valid::TopicQos, invalid durability_service qos``)
