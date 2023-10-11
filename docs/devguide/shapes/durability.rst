##########
Durability
##########

.. seealso:: :ref:`quality_of_service--durability`

In this example, we will begin to look at the DDS concept of durability as it relates to a publisher's offered quality of service (QoS) and a subscriber's requested QoS.
Durability allows messages sent before a subscriber came online to be delivered to that subscriber.
As in the last example, start up two instances of the Shapes Demo.

#. In the first application window, configure the publisher's QoS by opening the QoS menu and setting the following:

   * Durability QoS: TRANSIENT_LOCAL

   Then publish one instance of Square.

#. In the second application window, configure the subscriber's QoS by opening the lower QoS menu and setting the following:

   * Reliability QoS: Reliable
   * History QoS Depth: 100
   * Durability QoS: TRANSIENT_LOCAL

   Then subscribe to one instance of Square.

#. **Results** - You should observe a Square with 100 trailing squares bouncing around the subscriber application's display window.
   This demonstrates that the subscriber is being provided with the durable data that the publisher has made available.
   The example application windows should look like this (publisher on the left, subscriber on the right):

   .. figure:: durability-example.png
