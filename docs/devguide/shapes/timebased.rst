#################
Time-Based Filter
#################

.. seealso:: :ref:`quality_of_service--time-based-filter`

In this example, we will begin to look at the DDS concept of Time Based Filter QoS.
In this example, only 2 instances of the Shapes Demo application will be necessary.

#. In the first application window, publish 4 instances, 2 each of Circle and Square, but vary the colors to be unique.
   QoS settings on the publisher can remain defaulted.
   For this demonstration we will publish:

   * Circle - Green
   * Circle - Red
   * Square - Green
   * Square - Blue

#. In the second application window, subscribe to the Square topic using the default QoS settings.
   Then, subscribe to Circle by first using the QoS configuration menu to set the following:

   * Time Based Filter QoS - Minimum Separation: 2 (seconds)

#. **Results** - Both samples of Square should appear fluidly bouncing around the subscriber's application window.
   However, you will notice, the Circle samples are being sub-sampled based on the time filter that was configured.
   Only the most recent sample within the minimum separation is displayed in the subscriber's window.
   You should notice that each time the time based filter meets its required minimum separation the circle samples are updated and, for an instant, should have the same location as those in the publisher's window.
   Notice in the following graphics how the circles in the subscriber do not match up perfectly with the position of the circles in the publisher.
   Here are a couple examples of what your application windows might look like at this point (publisher on the left, subscriber on the right):

   .. figure:: time-based-filter-example-1.png

   .. figure:: time-based-filter-example-2.png
