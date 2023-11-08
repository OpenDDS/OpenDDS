#######################
Content-Filtered Topics
#######################

.. seealso:: :ref:`content_subscription_profile--content-filtered-topic`

In this example we will begin to look at the DDS concept of Content-Filtered Topics.
In this example only 2 instances of the Shapes Demo application will be necessary.


#. In the first application window publish a single instance of Square using the default QoS settings.

#. In the second application window you will subscribe to the Square topic.
   However, the content filter needs to be configured first.
   So click on the ``Filter`` button and it should bring up a dialog that looks like this:

   .. figure:: content-based-filter-menu.png

#. For this example we will use the default bounding box for the content filter which is controlled by the x and y slider bars in the menu.
   We will also use the default filter of **outside** which will tell the subscriber to ignore all samples outside the given bounding box.
   Then also toggle the filter by selecting ``Enable`` and finish by clicking the ``OK`` button.

#. Now that the content filter is set up, subscribe to the Square topic.

#. **Results** - You should be able to observe the square bouncing around the publisher application's display window.
   On the subscriber's side, however, the square's position and graphic will only be updated when it is within the content filter's bounding box.
   All samples outside the content filter area are filtered out.
   Here are a couple of examples of what your application windows might look like at this point (publisher on the left, subscriber on the right):

   .. figure:: content-based-filter-1.png

   Here you can see that while the published instance is within the content filter bounds, the subscriber's square updates normally keeping in step with the square on the publisher's side.

   .. figure:: content-based-filter-2.png

   Here you can see that when the published instance leaves the confines of the content filter, the subscriber's square remains at the last valid position and no longer updates its position until the published instance once again enters into the bounds of the content filter.
