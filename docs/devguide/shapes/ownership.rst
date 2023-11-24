#########
Ownership
#########

.. seealso:: :ref:`quality_of_service--ownership`

In this example, we will begin to look at the DDS concept of Ownership QoS.
In this example, 3 instances of the Shapes Demo application will need to be running simultaneously.

#. In the first application window, subscribe to one instance of Square after using the QoS menu to set the Ownership QoS accordingly:

   * Ownership QoS: EXCLUSIVE

#. Now start a second instance of the shapes demo. In this application window, publish one instance of Square after using the QoS menu to set the Ownership QoS accordingly:

   * Ownership QoS: EXCLUSIVE
   * Ownership QoS Strength: 50

#. **Initial Results** - At this point you should observe that the Square has appeared in the subscriber application's display area matching the color, size, and location of the square in the publisher's display area.
   The example application windows should look like this (publisher on the left, subscriber on the right):

   .. figure:: ownership-example-initial.png

#. Now start a third instance of the shapes demo.
   In this application window, publish one instance of Square after using the QoS menu to set the Ownership QoS accordingly:

   * Ownership QoS: EXCLUSIVE
   * Ownership QoS Strength: 75
   * **Before clicking "Publish" increase the size of the Square using the size slider.**
     (This will help make the changes more visible.)

#. **Final Results** - At this point you should observe that the larger Square has appeared in the subscriber application's display area.
   Since the Ownership QoS has been set to exclusive, only one writer of the topic -- the writer with the highest strength -- will have its messages find their way to the subscriber.
   The example application windows should look like this (initial publisher on the left, subscriber on the right, *new* publisher on the right):

   .. figure:: ownership-example-final.png

#. **Note** - At this point you could simulate a higher strength publisher leaving the system by closing the second publisher application window.
   Then you should observe the subscriber failing over to the initial publisher's smaller instance of the Square that had a lower ownership strength.
