##################
Multiple Instances
##################

#. Start up two instances of the Shapes Demo for this example. You should be able to see two application windows like so:

   .. figure:: two-instances.png

#. **Publish** - Let's change it up and begin this example by publishing a triangle by selecting ``Triangle`` from the top Shape dropdown menu.
   Then click the ``Publish`` button.
   You should observe a green triangle bouncing around the application window's display area.
   The triangle, from a DDS perspective, is a piece of information that is being published to the rest of the system for other entities to receive if they are set up to be interested in triangles.

   .. figure:: two-instances-pub-triangle.png

#. **Subscribe** - In the other application window, select ``Triangle`` from the bottom Shape dropdown menu.
   Then click the ``Subscribe`` button.
   You should observe the green triangle, published by the first application instance bouncing around the display area in the application window you chose to subscribe in.

   .. figure:: pub-sub-triangle.png

#. **Results** - The triangle in the subscriber's window, tracking the initial triangle from the publisher's window, exemplifies the DDS publish/subscribe paradigm for a single publisher to single subscriber.
   The first application has a single publisher sending information out into the system and the second application has a single subscriber receiving that information.
