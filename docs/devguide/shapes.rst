.. _shapes-demo:

###########
Shapes Demo
###########

The goal of this guide is to walk through downloading and running the DDS Interoperability Shapes demo program.
The Shapes demo is a graphical application that will help you visualize some of the DDS concepts that OpenDDS supports without needing to read or write source code.
The application allows you to create publishers and subscribers on topics represented by data payloads shown as simple shapes and see the results of the different quality of service, data partitioning, and filtering options provided.
The Shapes demo makes use of the Real-Time Publish-Subscribe Wire Protocol Specification (DDSI-RTPS) that provides interoperable communications between various DDS implementations.

`This video <https://www.youtube.com/watch?v=1GDXjp2kmGg>`__ demonstrates a few of the scenarios described below.

*************************
Environment Prerequisites
*************************


#. Ensure that your environment has Qt5 libraries available:

   * Windows (x64): Use the `Qt Installer <https://www.qt.io/download-thank-you?os=windows&hsLang=en>`__, or install the qt5 package from `vcpkg <https://github.com/Microsoft/vcpkg>`__.
     The Visual C++ runtime `redistributable <https://aka.ms/vs/17/release/vc_redist.x64.exe>`__ is also required.

   * Ubuntu (x86_64): Install the ``libqt5gui5`` package.

#. Download or compile the latest Shapes demo for your operating system:

   * `Choose from the "ShapesDemo" assets in the latest release of OpenDDS <https://github.com/OpenDDS/OpenDDS/releases/latest>`__

   * Alternatively, the Shapes demo can be run from a build from source of OpenDDS with :ghfile:`Qt enabled <docs/qt.md>`.
     The demo resides in the :ghfile:`examples/DCPS/ishapes` directory.
     See :ref:`deps` for a complete list of dependencies and :ghfile:`README.md#supported-platforms` for supported platforms.

***********************
Running the Shapes Demo
***********************

#. Locate and run the Shapes demo and you should see a window that looks like this:

   .. figure:: shapes/default.png

#. The most basic example is to simply click the ``Publish`` button to begin publishing a circle and then click ``Subscribe`` to begin subscribing to the same circle topic you are publishing.
   As soon as you begin publishing, you should see a green circle bouncing around the display area with a white center.
   This indicates that this is a published object, originating at this application.

   .. figure:: shapes/pub-circle-unmatched.png

#. After clicking ``Subscribe``, as soon as the subscriber goes live and finds a match in the published circle topic, the center of the circle will go dark to indicate it is an instance of subscribed data, which in this case is being rendered over top the initial published instance.

   .. figure:: shapes/pub-circle-matched.png

#. Now that we have confirmed the ability to run an instance of the Shapes demo, let's understand some of the DDS concepts seen at work even in this simple example.

   * Each of the available shapes in the drop-down menu are DDS Topics that can be published on or subscribed to.

   * Each instance on the topic has an instance key represented by the shape's color and the shape itself is a DDS Data type comprised of x and y coordinates as well as a size.

   What may not be apparent in this simple example is that the center of the circle going from white to dark is actually demonstrating the process of discovery, entity matching, publication, and subscription between two DDS entities.
   The additional example scenarios below make use of multiple shapes demo instances running concurrently to better demonstrate these capabilities.

*****************
Example Scenarios
*****************

The pages linked below contain example scenarios that demonstrate different features of OpenDDS:

.. toctree::

   shapes/instances
   shapes/reliability
   shapes/durability
   shapes/partition
   shapes/ownership
   shapes/timebased
   shapes/contentfilter

**********
Conclusion
**********

The examples worked through above have demonstrated some of the many Data Distribution Service features provided by OpenDDS.
OpenDDS provides a portable and interoperable publish/subscribe infrastructure.
You have learned a little about many DDS concepts including:

* Discovery
* Topics and Data Types
* Publish/Subscribe semantics
* One to Many and Many to One communications
* Different Quality of Service (QoS)

  * Reliability
  * Durability
  * Ownership
  * Data Partitioning
  * Time Based Filter
* Content Filtered Topics

Hopefully this has helped provide an understanding of what using OpenDDS to provide a publish/subscribe infrastructure can look like and the different features that can be included in a product using OpenDDS as its communications infrastructure.

**********
Next Steps
**********

Now that you've acquired a conceptual baseline, here are some recommended next steps to continue learning about OpenDDS:

* Work through downloading and building OpenDDS and running a simple example through the other :ref:`quickstart`.

* Find more support information about dependencies and detailed instructions for :ref:`building`.

* Learn more about :ref:`getting_started` with OpenDDS by walking through an end-to-end example.

* Visit `the OpenDDS project on GitHub <https://github.com/OpenDDS/OpenDDS>`__.
