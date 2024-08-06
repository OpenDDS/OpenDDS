###########
Reliability
###########

.. seealso:: :ref:`quality_of_service--reliability`

In this example, we will begin to look at the DDS concept of reliability as it relates to a publisher's offered quality of service (QoS) and a subscriber's requested QoS.
As in the last example, start up two instances of the Shapes Demo.

#. **Publisher/Subscriber QoS** - In this example we will publish 3 separate topics, each being configured with QoS settings to demonstrate Reliability QoS compatibility.
   Use the upper ``QoS`` button to open a window that will provide you with QoS configuration options for the publisher (Writer QoS).
   Here you should see the Reliability QoS setting, which defaults to reliable.
   Likewise, if you click the lower ``QoS`` button, a window will open which provides configuration options for the subscriber (Reader QoS).
   As you can see, the subscriber Reliability QoS defaults to best effort.

   .. figure:: publisher-qos-menu.png

   .. figure:: subscriber-qos-menu.png

#. In the first application window, publish one instance of each of the following topics using the QoS menu to set the reliability accordingly before pressing the ``Publish`` button to begin publishing that topic:

   * Square: RELIABLE
   * Circle: BEST_EFFORT
   * Triangle: BEST_EFFORT

#. In the second application window, subscribe to one instance of each of the following topics using the QoS menu to set the reliability accordingly before pressing the ``Subscribe`` button to begin subscribing to that topic:

   * Square: RELIABLE
   * Circle: BEST_EFFORT
   * Triangle: RELIABLE

#. **Results** - You should observe in the subscriber application's display window that there are only 2 matched topics - Square and Circle.
   The subscriber for the Triangle topic did not find a match due to requesting a quality of service of RELIABLE, but the only available publisher was only offering BEST_EFFORT.
   Because the publisher could not guarantee reliable delivery of messages, the subscriber was unable to fulfill its requirement and therefore was unable to find a match.
   Thus, the unmatched triangle topic on the subscriber's side is grayed out.
   Your two application windows should look similar to the following:

   .. figure:: reliability-example.png
