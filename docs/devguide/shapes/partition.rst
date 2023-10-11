#########
Partition
#########

.. seealso:: :ref:`quality_of_service--partition`

In this example, we will begin to look at the DDS concept of partitions.
In this example, 4 instances of the Shapes Demo application will need to be running simultaneously, each instance passed initial parameters to help configure the application's partition.
Each step below will describe how one of the instances should be started and configured for publishing/subscribing.

#. **First publisher:** Start an instance of the Shapes demo passing it a command line argument of ``-partition "A"``.
   Then, have this application instance publish a Square.
   You may leave all the QoS menu items defaulted.

#. **Second publisher:** Start an instance of the Shapes demo passing it a command line argument of ``-partition "B"``.
   Then, have this application instance publish a Circle.
   You may leave all the QoS menu items defaulted.

#. **Third publisher:** Start an instance of the Shapes demo passing it a command line argument of ``-partition "*"``.
   Then, have this application instance publish a Triangle.
   You may leave all the QoS menu items defaulted.

#. **Subscriber (fourth instance):** Start an instance of the Shapes demo passing it a command line argument of ``-partition "A"``.
   Then, have this application instance subscribe to Square, Circle, and Triangle.
   You may leave all the QoS menu items defaulted.

#. **Results** - You should observe that only the Square and Triangle appear in the subscriber application's display area.
   This is because this subscriber instance matches to both partition ``A`` and ``*``.
   However, the circle is being published in partition B, which the subscriber does not belong to.
   Therefore, the circle is grayed out because it is an unmatched subscription currently.
   The example application windows should look like this (subscriber on the bottom right):

   .. figure:: partition-example.png
