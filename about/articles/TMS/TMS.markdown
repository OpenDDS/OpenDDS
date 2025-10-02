---
layout: page
title: Building Tactical Microgrid Standard Systems Using OpenDDS
---
<p class="author">
Son Dinh, Senior Software Engineer (Object Computing, Inc.)<br>
Fred Hornsey, Senior Software Engineer (Object Computing, Inc.)<br>
October 2025
</p>

In this article we briefly discuss the problems of traditional battlefield power systems and how the Tactical Microgrid Standard (TMS) [^1] addresses these problems.
We then present a demo of a simulated power system conforming to the TMS specification which uses OpenDDS [^6] as its underlying data-centric communication middleware.
In a subsequent section, we provide more details about OpenDDS and how it is a viable option when building a TMS-compliant system.

## Overview of Tactical Microgrid Standard

Traditional power systems used in tactical deployments have several drawbacks from not having a standardized communication and control interface.
Figure 1 demonstrates typical microgrid configurations and their drawbacks prior to TMS (refer to the materials published by Tactical Microgrid Standard Industry Day, 2025 [^11] for more defail).

<p align="center">
  <img src="images/BeforeTMS.jpg" width="600"/><br>
  Figure 1: Drawbacks before TMS
</p>

One problem is inefficient fuel usage and power generation.
This is due to the lack of coordination between multiple isolated power generators, which can cause suboptimal load balancing across the generators and lead to inefficient fuel use.

The second problem is the lack of interoperability between devices manufactured by different vendors – equipment from different vendors can’t be easily connected to and communicate with each other.
This vendor lock-in severely limits the options available to expand the system.

The third problem with legacy power systems is that they have limited support for control, monitoring, and situational awareness.
This adds an additional burden to the operators to manually track and react to issues at run-time, which could lead to more error-prone operations.

TMS addresses these issues by providing a modular architecture for the power system and a standard communication and control interface between the components.

<p align="center">
  <img src="images/WithTMS.jpg" width="600"/><br>
  Figure 2: Improvements with TMS
</p>

This addresses the first problem by providing better communication across the power generators, essentially creating a microgrid formed by the previously standalone generators.
The enhanced coordination between the power generators helps balance the loads more efficiently and optimize fuel usage.

TMS addresses the second problem, at the communication level, by adopting Object Management Group's Data Distribution Service (OMG DDS) [^2] as its underlying communication infrastructure and defining a common interface between TMS components.
By building on top of a proven technology in DDS, TMS inherits the advantages that DDS provides, including interoperability of equipment from different vendors, even if each vendor uses a different DDS implementation.
This allows a microgrid to flexibly expand or shrink its scale while being able to connect devices from different suppliers.

TMS addresses the third problem by introducing control devices that monitor the microgrid and has the ability to make decisions automatically with the presence of a situational awareness module.
In addition to the automatic commands, it also allows an operator to issue commands manually.
The commands, either automatic or manual, are sent to the corresponding devices using DDS.
The control devices have a global view of the whole grid that creates an opportunity for further enhancements to optimize the microgrid’s operation even more.
TMS also provides a mechanism for failover with a [microgrid controller selection algorithm](#appendix) to avoid a single point of failure.

## TMS Device Classification

TMS defines two classes of device roles: *control device roles* and *power device roles*.
Each of these two classes specifies several modular components.
In particular, the control device roles comprise the microgrid controller, the microgrid dashboard, and the monitor components.
The power device roles consist of source, storage, load, distribution, and conversion components.
These are logical components and can be grouped into a single physical device.
For example, a single physical power device can provide both conversion and distribution functions.
Figure 3 shows these roles as specified in the TMS specification [^1].

<p align="center">
  <img src="images/TMS_devices.png" /><br>
  Figure 3: Device roles in a TMS system
</p>


Below, we give a brief description of each component. First, the roles for *control devices*:

- **Microgrid Dashboard** (MD): receives operating information from the Microgrid Controller and displays it to the operator.
It can also allow the operator to manually control the microgrid. Instructions from the MD will be sent to the Microgrid Controller.

- **Microgrid Controller** (MC): incorporates the instructions from MD and operating information from power devices to build awareness of the current operating situation.
It then instructs the individual power devices accordingly.

- **Monitor** (MON): records and displays the data exchanged between devices in the microgrid.

Next, roles from the *power devices*:

- **Source** (SRC): device that generates power, such as those containing a generator or solar panel.
It has a single power port to connect to other devices for distributing or consuming power.

- **Storage** (STOR): a device that stores energy and has a power port to provide energy to other devices.

- **Load** (LOAD): a device that consumes power. It also has a single power port for receiving power.

- **Distribution** (DIST): a power distributing device that has two or more power ports.

- **Conversion** (CONV): contains multiple power ports with power converters, which allow converting both voltage and frequency.

Besides the components for handling power-related functionalities, each of these power devices has a Device Controller component that communicates with other devices and the microgrid controller using the DDS topics defined in the TMS specification.

For simplicity, we assume that each device only implements a single logical component for the rest of this article.
For example, a physical device can only perform power distribution or conversion, but not both.
In particular, we use the following devices in this article: microgrid controller, microgrid dashboard, source, distribution, and load.

## Overview of Data Distribution Service

In this section we briefly discuss what Data Distribution Service (DDS) is and what features make it stand out from other options since it was chosen as the underlying data communication middleware for TMS.

DDS is a standard for data-centric, real-time communication using a publish/subscribe model.
In DDS, a domain defines a logical data space where participants can communicate using topics.
In particular, a publisher publishes data belonging to a topic, which is then distributed to one or more subscribers who subscribe to the same topic.
Each topic has a specific data type, typically specified using Interface Definition Language (IDL).
In IDL, users can define a topic type similarly to how they would define a data structure in most programming languages.
Furthermore, the DDS-XTypes specification [^4] allows topic types to be extended and modified while still maintaining interoperability with existing participants that use previously-defined data types.

DDS has a rich set of Quality of Service (QoS) policies that enable users to specify the requirements of their specific application.
A few of the QoS policies that DDS provides are useful to real-time applications to specify their constraints, such as `DEADLINE`, `LATENCY_BUDGET`, `TIME_BASED_FILTER`, or `TRANSPORT_PRIORITY`.
This is especially important for a mission-critical system like TMS, since excessive delay or any timing unpredictability can cause failure of the mission.

DDS uses the Real-Time Publish Subscribe (RTPS) protocol [^3] as its wire protocol for interoperability between different DDS implementations.
This means a microgrid can combine devices from different vendors, with each using a different DDS implementation.
Some OMG member organizations, including Object Computing, maintain a system for testing interoperability across different DDS implementations.

Additionally, DDS supports a pluggable security architecture with its own specification [^5].
It provides mechanisms for authentication, authorization, data privacy and integrity. TMS, at the time of writing, doesn’t use DDS Security.
However, it will likely be added to the standard in a future revision to leverage the mentioned security features and take advantage of the existing DDS implementations.

OpenDDS, an open-source implementation of DDS, supports all of the DDS specifications mentioned in this section.
In particular, it implements DDS-XTypes for extensible and dynamic topic types and its necessary IDL extensions, DDS-RTPS for interoperability with other DDS implementations, and DDS Security.

In the next section, we demonstrate an application that simulates a TMS-compliant microgrid using OpenDDS.
In a subsequent section, we provide more details on and references to OpenDDS resources.

## TMS Demo Using OpenDDS

We developed a set of applications that simulate devices common to TMS-compliant microgrids including a microgrid controller, a source, a load, and a distribution device.
The simulation is driven by a command-line interface (CLI) program that sets up the topology of the power devices and acts partially as a microgrid dashboard from which an operator can issue commands to the devices in the microgrid.

The following diagram shows the components of the simulated microgrid.

<p align="center">
  <img src="images/Simulated_microgrid.jpg" width="600"/><br>
  Figure 4: Components of the simulated microgrid
</p>

Except the CLI, each device implementation, including SRC, DIST, LOAD, and MC, can run multiple instances simultaneously.
This simulates an actual microgrid with multiple power devices of the same category, for example, a SRC with a fuel generator can co-exist with another SRC with solar panels.
Multiple simultaneous instances of microgrid controllers are also desirable to provide redundancy and failover capability in the event that the main controller fails.

Communication between the power devices and the control devices is implemented using the DDS data flows specified in the TMS specification.
To simulate the electric connection and power flow between the power devices, we use a custom DDS domain and topics that are separate from the DDS domain and topics used by TMS.

The source code for this demo can be accessed at a public repository [^8].
In addition, we use a tool named opendds-monitor [^7] to graphically monitor and display DDS topic data exchanged among the components in this demo.
The source code of opendds-monitor can be accessed at https://github.com/OpenDDS/opendds-monitor.

Next, we discuss the data flows used in our simulation in detail.
We first describe the data flows specified in the TMS spec.
Then, we describe the custom data flows that are used to simulate the power connections and electric current between devices.

## TMS Data Flows

### Handshaking

The handshaking function is implemented by all power devices and the microgrid controller.
It provides data flows for announcing the presence of a device and maintaining its liveliness from other devices.
Each device announces its presence using the TMS `DeviceInfo` DDS topic at the device’s start time.
This topic contains the device’s identity, role, TMS topics that it supports, and product and power hardware information.

<p align="center">
  <img src="images/TMS_DeviceInfo.jpg" /><br>
  Figure 5: Devices announce their information
</p>

Below is a screenshot of the opendds-monitor tool for displaying the contents of the TMS `DeviceInfo` sample sent from a controller, named "TestMC".
On the left-hand side, the monitor lists the topics for which it detects publishers and subscribers.
When a topic is selected from the left-hand side list, a tab is created for that topic on the right-hand side.
This tab displays a history table containing a list of samples for the topic associated with this tab, and the details of a sample selected from the history table.

<p align="center">
  <img src="images/monitor_DeviceInfo.jpg" /><br>
  Figure 6: A TMS `DeviceInfo` sample captured by opendds-monitor
</p>

Each device maintains its liveliness, as perceived by the other devices, by periodically publishing to the TMS `Heartbeat` topic.
The period of the `Heartbeat` samples is 1 second, meaning a device is expected to send a `Heartbeat` sample once every one second.
If a `Heartbeat` is not received within 3 seconds after the last `Heartbeat` of the sending device, the receiving device is said to have missed the sending device’s `Heartbeat`, and the receiving device considers the sending device as inactive.
The `Heartbeat` topic is used by each power device to determine its active microgrid controller, based on the [microgrid controller selection algorithm](#appendix).

<p align="center">
  <img src="images/TMS_Heartbeat.jpg" /><br>
  Figure 7: Devices maintain their liveliness using TMS `Heartbeat`
</p>

### Operator Input

This data flow allows an operator of the microgrid to enter their input to influence the operation of the microgrid through the microgrid dashboard.
This is done via the TMS `OperatorIntentRequest` topic.

In this demo, we use this data flow to start and stop a power device.
For example, we can stop a source device and observe that the power stops flowing from this device to other devices, such as load or distribution devices.
In our demo, the CLI acts partially as an MD and allows the user to enter input from the command line to start or stop a device.

<p align="center">
  <img src="images/TMS_OperatorIntentRequest.jpg" /><br>
  Figure 8: CLI requests an operation change from the operator
</p>

Figure 9 shows a sample of the `OperatorIntentRequest` topic sent from the CLI, named “CLI Client”, to the controller, named “TestMC”.
Within this sample, the nested field `desiredOperatorIntent.intentType` is set to an enum value of `OIT_OPERATOR_DEFINED`, which means this intention of operation is defined by an operator.
The nested member `desiredOperatorIntent.devices` is a sequence of `DeviceIntent`s with each element specifying the operation intention for a specific device.
In this example, the intention is set for a source device, named “TestSource”.
The priority type is set to `OPT_NEVER_OPERATE`, meaning that the operator intends to transition the device to the lowest supported energy level.

<p align="center">
  <img src="images/monitor_OperatorIntentRequest.jpg" /><br>
  Figure 9: An `OperatorIntentRequest` sample captured by opendds-monitor
</p>

### Source Start/Stop

This data flow enables the microgrid controller to request a source device to transition from one operating level to another.
This request may be initiated by the MC itself, through its situational awareness and automatic control functions.
It can also be the result of an `OperatorIntentRequest` request issued earlier by the microgrid dashboard.
The TMS topic used in this flow is `EnergyStartStopRequest`, as shown in the diagram below.

<p align="center">
  <img src="images/TMS_EnergyStartStopRequest.jpg" /><br>
  Figure 10: MC requests a change to the energy level of a SRC or STOR
</p>

An example of the contents of the `EnergyStartStopRequest` topic is shown in a screenshot of the opendds-monitor below.
The member `requestId` contains nested fields for specifying the identities of the sending and receiving devices.
The `fromLevel` member specifies the current energy level of the target device; `ESSL_ANY` matches all energy levels and was used in this example.
The `toLevel` member specifies the requested energy level of the target device.
In this example, the target device, named “TestSource”, is being turned off.

<p align="center">
  <img src="images/monitor_EnergyStartStopRequest.jpg" /><br>
  Figure 11: An `EnergyStartStopRequest` sample captured by opendds-monitor
</p>

The TMS `EnergyStartStopRequest` topic is also used to transition the energy levels for storage devices.
In this demo, we are not including storage devices.
However, the usage is similar and can be easily added if desired.

### Microgrid Controller Selection

In TMS microgrids, the microgrid controller (MC) has a global view of the whole microgrid and is responsible for making intelligent operational decisions based on its awareness of the status of the devices in the microgrid.
Because of its significance, there can be (and should be) more than one controller in a microgrid to provide redundancy in the event of the failure of the main controller.
Each power device individually selects a controller as its active MC using a selection algorithm provided by the TMS spec.
Even though the power devices execute the selection algorithm separately, it is expected that they will eventually converge on selecting the same active microgrid controller.
Thus, a single controller will take control of the whole microgrid.

#### Algorithm

The TMS spec describes the microgrid controller selection algorithm and a corresponding state diagram.
The provided algorithm and its state diagram, however, are simplified and lack details on how some specific actions should be performed.
We, therefore, break down the state diagram further into a more detailed diagram with additional states that better describe our implementation.
Our implementation of the microgrid controller selection algorithm has additional steps that are not described in the TMS spec.
Interested readers are referred to the [Appendix](#appendix) section for more details on our implementation.

#### Data Flow

The microgrid controller selection algorithm is mainly driven by the heartbeats of the MCs and the three timers: New MC, Lost Active MC, and No MC timers.
In addition to the TMS `Heartbeat` topic, when a power device experiences a change in its active MC, it publishes to the TMS `ActiveMicrogridControllerState` topic to notify the control devices of its current active MC.

<p align="center">
  <img src="images/TMS_ActiveMicrogridControllerState.jpg" /><br>
  Figure 12: Power device notifies a change to active MC
</p>

Figure 13 shows a sample of the `ActiveMicrogridControllerState` topic, sent from the SRC, named “TestSource”, to notify its current active MC, named “TestMC”.
The `masterId` member is optional (annotated with `@optional` in IDL) and can be empty to indicate that the power device doesn’t have an active MC.

<p align="center">
  <img src="images/monitor_ActiveMicrogridControllerState.jpg" /><br>
  Figure 13: An `ActiveMicrogridControllerState` sample captured by opendds-monitor
</p>

## Simulation Data Flows

In addition to the TMS data flows, the demo implements several custom data flows to construct and simulate the topology of the power devices, as well as the electric current between the devices.
To isolate the TMS data flows from the simulation data flows, we use separate DDS domains for them.
In the following sections, we briefly describe these data flows and topics.

### Power Devices List

The controllers learn about the power devices in the microgrid through the TMS Handshaking function, including the `DeviceInfo` and the periodic `Heartbeat` messages.
However, the power devices initially are isolated processes as there are no connections between them to simulate the power topology of devices in an actual microgrid.

With this data flow, the CLI can send a request to the controllers by publishing to the `PowerDevicesRequest` topic.
It then receives back the list of power devices in the simulated TMS domain using the `PowerDevicesReply` topic.
The power devices list can then be displayed to the user so that they can connect the devices as they desire.

<p align="center">
  <img src="images/TMS_PowerDevicesRequest.jpg" /><br>
  Figure 14: CLI requests the list of power devices in the TMS domain
</p>

The IDL for these two topics are shown below.

```
@topic
@final
struct PowerDevicesRequest {
  @key tms::Identity mc_id;
};

@nested
@appendable
struct PowerDeviceInfo {
  tms::DeviceInfo device_info;
  tms::EnergyStartStopLevel essl;
  @optional tms::Identity master_id;
};
typedef sequence<PowerDeviceInfo> PowerDeviceInfoSeq;

@topic
@final
struct PowerDevicesReply {
  @key tms::Identity mc_id;
  PowerDeviceInfoSeq devices;
};
```

The `PowerDevicesRequest` topic contains the identity of the target MC, allowing the MCs to ignore requests for other MCs.
The `PowerDevicesReply` contains a list of power devices that are reachable from the corresponding MC.
The information for each power device includes its `DeviceInfo`, current energy level, and its current active MC.

### Power Topology

Once the CLI has information about the power devices, the user can specify the topology of the simulated microgrid by connecting the power devices.
The CLI verifies that the construction entered by the user conforms to the characteristics of the devices, for example, a SRC has a single power port and therefore cannot be connected to two devices.
The power topology information is sent to the MCs via the `PowerTopology` topic, and then propagated to each individual power device via the `PowerConnection` topic.

<p align="center">
  <img src="images/Construct_topology.jpg" /><br>
  Figure 15: User connects the power devices using the CLI
</p>

The IDL definitions of these two topic types are shown below.

```
@nested
@appendable
struct ConnectedDevice {
  tms::Identity id;
  tms::DeviceRole role;
};
typedef sequence<ConnectedDevice> ConnectedDeviceSeq;

@topic
@final
struct PowerConnection {
  @key tms::Identity pd_id;
  ConnectedDeviceSeq connected_devices;
};
typedef sequence<PowerConnection> PowerConnectionSeq;

@topic
@final
struct PowerTopology {
  @key tms::Identity mc_id;
  PowerConnectionSeq connections;
};
```

The `PowerConnection` topic contains the identity of the device to which the connections apply.
It also contains a list of other devices to which it is connected, each with its identity and role.
The `PowerTopology` is simply a list of the `PowerConnections` for each device.

### Electric Current

We use a DDS topic, named `ElectricCurrent`, to simulate the power flow between the devices.
It can be published by source and distribution devices, and subscribed to by the load and distribution devices.

<p align="center">
  <img src="images/Simulated_current.jpg" /><br>
  Figure 16: Simulated electric current
</p>

The IDL of the `ElectricCurrent` topic is shown below.

```
@topic
@final
struct ElectricCurrent {
  IdentitySeq power_path;
  float amperage;
};
```

It contains the path of an electric current which starts with a SRC.
This allows the receiving device to verify that current is flowing through it and not another device.

Currently, the `ElectricCurrent` supports only an amperage metric, but it can be enhanced to support other metrics.
For simplicity of the demo, each DIST device splits the current evenly across its output power ports.

## Demo in Action

The configuration of the demo is shown in Figure 17.
It consists of a CLI, two microgrid controllers, a source device, a distribution device, and two load devices, with each running as a separate process.

<p align="center">
  <img src="images/Demo_setup.jpg" width="600"/><br>
  Figure 17: Demo configuration
</p>

### Inspect Active Microgrid Controller

We start all devices at the same time.
The CLI can display the list of MCs, including both the available and unavailable ones, using the `list-mc` command.

<p align="center">
  <img src="images/CLI_list-mc.jpg" /><br>
  Figure 18: List all reachable microgrid controllers
</p>

The list of power devices connected to each MC can be displayed by the `list-pd` command.
The information for each device includes its identity, device role, energy level, and its currently selected controller.
If this command is issued before a device has completed selecting its active MC, the active MC for the device is noted as *Undetermined*, as shown in Figure 19.

<p align="center">
  <img src="images/CLI_list-pd_undetermined.jpg" /><br>
  Figure 19: Power devices haven’t determined their active MCs
</p>

Each device selects its active MC using the microgrid controller selection algorithm described in the [Appendix](#appendix).
After it has finished selecting an MC, the selected MC is displayed in the console of the device.
The figure below shows the console of the source device.
The other power devices have similar output.

<p align="center">
  <img src="images/SRC_selectMC.jpg" /><br>
  Figure 20: Power device completes selecting active MC
</p>

In this setup, we have two MCs with identities “TestMC_1” and “TestMC_2” and equal `priorityRanking`.
The microgrid controller selection algorithm thus selects the “TestMC_1” controller since it has the first identity alphabetically.
The active MCs selected by the power devices can also be seen from the CLI as below.

<p align="center">
  <img src="images/CLI_list-pd_converge.jpg" /><br>
  Figure 21: Power devices converge on a single MC
</p>

As shown in the figure, all power devices converge on selecting the same MC, “TestMC_1”, as their active MC.

### Connect Power Devices

To construct the microgrid power topology, we use the `connect-pd` command from the CLI.
Figure 22 shows the user input to connect the power devices following the topology configuration of this demo.

<p align="center">
  <img src="images/CLI_connect-pd.jpg" /><br>
  Figure 22: Connect power devices from the CLI
</p>

This step goes through the list of devices, and for each device, the user can input whether they want to connect it to each of the other devices.
The CLI sends a `PowerTopology` topic sample to one of the MCs which will then propagate the connection information to each power device.

Once the power topology has been set up, the source starts publishing to the `ElectricCurrent` topic to simulate the power flow.

<p align="center">
  <img src="images/SRC_sendpower.jpg" /><br>
  Figure 23: Source device sends power
</p>

The power is sent to the distribution device with identity “TestDist”, which then distributes the power to the load devices “TestLoad_1” and “TestLoad_2”, as shown in the following figure.

<p align="center">
  <img src="images/DIST_relaypower.jpg" /><br>
  Figure 24: Distribution device distributes power to the load devices
</p>

Each load device, in turn, receives the power from the distribution device.
We include the output from the load device “TestLoad_1” below.
The output from the load device “TestLoad_2” is similar.

<p align="center">
  <img src="images/LOAD1_receivepower.jpg" /><br>
  Figure 25: Load device receives power from the distribution device
</p>

From the CLI, the user can stop and start a power device.
For example, to stop the source device “TestSource”, the user can use command `stop TestSource`.

Stopping a power device requests the device to operate at the lowest energy level, which essentially means turning it off.
For the source device, it will stop publishing to the `ElectricCurrent` topic which simulates the behavior of a real device.
Consequently, the distribution and load devices will no longer receive power.

To start the source device again, we use the command `start TestSource` from the CLI.

### Microgrid Controller Failover

We demonstrate the scenario when the main MC fails and the backup MC takes over the control of the microgrid.
When the main controller, “TestMC_1”, is terminated, each power device will detect the missed heartbeat event for that MC which triggers the microgrid controller selection process to select the next active MC.

<p align="center">
  <img src="images/SRC_term_MC1.jpg" /><br>
  Figure 26: Source device selects a new active MC
</p>

A similar process happens on the distribution device,

<p align="center">
  <img src="images/DIST_term_MC1.jpg" /><br>
  Figure 27: Distribution device selects a new active MC
</p>

and the two load devices. (We include the console output of “TestLoad_1” here. The output of “TestLoad_2” is similar.)

<p align="center">
  <img src="images/LOAD1_term_MC1.jpg" /><br>
  Figure 28: Load device selects a new active MC
</p>

In the figure below of the CLI console, we can see that the controller “TestMC_1” is unavailable, and all power devices converge on the new active MC, “TestMC_2”.

<p align="center">
  <img src="images/CLI_term_MC1.jpg" /><br>
  Figure 29: Output of the `list-mc` and `list-pd` commands after "TestMC_1" terminates
</p>

If we start the controller “TestMC_1” again, the CLI will indicate that it becomes available as shown in the following figure.
However, the power devices will keep the controller “TestMC_2” as their active MC until the microgrid controller selection process is triggered again, for example, when “TestMC_2” crashes and stops sending its heartbeats.
Refer to the [Appendix](#appendix) for more details on the microgrid controller selection algorithm.

<p align="center">
  <img src="images/CLI_start_MC1_again.jpg" /><br>
  Figure 30: Controller “TestMC_1” is brought back up
</p>

## OpenDDS: Complete and Open Source DDS for TMS

In this demo, we used OpenDDS and its tools to simulate a microgrid system that conforms to the TMS specification.
We have demonstrated a few scenarios of a TMS system.
The demo highlights the following features of OpenDDS:

**IDL compiler**: We used `opendds_idl`, the IDL compiler provided by OpenDDS, to process the IDL file containing the topics and types defined in the TMS spec.
`opendds_idl` implements OMG Interface Definition Language, version 4.2, and supports the necessary features required by the TMS spec, such as its data types, the `@optional`, `@extensibility`, and `@try_construct` annotations.

**DDS QoS**[^2]: Each topic defined in the TMS spec is associated with a QoS profile.
Our demo has shown that OpenDDS provides all the QoS policies required by TMS.

**RTPS and XTypes**: OpenDDS implements the interoperability wire protocol RTPS, version 2.4 [^3], ensuring applications developed using OpenDDS will interoperate with applications developed using other DDS implementations.
In addition, OpenDDS implements XTypes, version 1.3 [^4], which enables extensible types, dynamic language binding, and data representations such as XCDR2.

In addition to the above features and DDS standards, OpenDDS also implements the DDS Security standard, version 1.1 [^5].
Although the current TMS specification doesn’t require DDS security, it is likely to be added to a future version of the standard.
OpenDDS is ready to provide these security features to existing and future TMS systems.

More information on OpenDDS can be found in the project’s documentation [^9] and website [^10].

## Appendix

### Microgrid Controller Selection Algorithm

Below is the detailed state diagram of the microgrid controller selection algorithm implemented by each power device.
Note that we use the terms active MC and selected MC interchangeably in this section.

<p align="center">
  <img src="images/MC_selection_algorithm.jpg" /><br>
  Figure 31: Microgrid controller selection state diagram
</p>

The selection algorithm tracks the availability of all MCs that are reachable from the power device.
The availability of an MC is determined by whether the power device misses heartbeats from that MC.
Each MC must publish TMS `Heartbeat` samples periodically, with a period of 1 second.
If a power device does not receive a heartbeat from an MC after 3 seconds since the last heartbeat, the MC is unavailable to the power device.

The bottom of the diagram with the states in orange describes the transitions between the *Available* and *Unavailable* states of each MC.
The transitions are driven by a timer with duration of 3 seconds, accounting for the deadline of each heartbeat.
When a heartbeat is received for an MC, the power device schedules a Missed Heartbeat timer.
When the timer expires, the MC becomes unavailable.

In practice, we don’t have to create a timer for each MC.
Instead, we just track the time point of the last heartbeat received for each of them.
When the power device selects a new active MC, the MCs for which the last heartbeats were more than 3 seconds ago are ignored.

The states in blue at the top of the diagram correspond to the status of the current active MC selected by the power device.
At the beginning, there is no MC selected.
When the power device receives a heartbeat from any MC, it starts a New MC timer with a duration of 3 seconds (transition T1).
When this timer expires, the device selects an active MC from the list of available MCs (transition T2).

When there are multiple candidates for an active MC, the power device selects the MC with highest `priorityRanking` in the nested member `MicrogridControllerInfo` of the `DeviceInfo` sample published by the MCs.
If multiple MCs have the equal `priorityRanking` value, the MC with first identity alphabetically is selected.
Due to this, the MC that triggered the New MC timer is not necessarily the selected MC.

Transition T3 is similar to what we described for each individual MC.
That is, each time a heartbeat is received from the active MC, the Missed Heartbeat timer is rescheduled.
Unlike the unselected MCs, for the active MC, we have to use actual timers to drive its state transitions.
If the Missed Heartbeat timer expires, the power device transitions to the *Active MC Unavailable* state, and schedules a Lost Active MC timer with a duration of 6 seconds (transition T4).

If a heartbeat is received from the active MC while the Lost Active MC timer hasn’t expired yet, the power device moves back to the *Active MC Available* state (transition T5).
Receipt of heartbeats from non-active MCs does not affect this timer.
If, however, the Lost Active MC timer expires, the power device will move to the *No Active MC* state (transition T6) and have two options.

First, if there are available MCs, the power device will select a new active MC, using the same procedure described for transition T2.
In that case, the power device transitions to the *Active MC Available* state, with a new active MC chosen.
Assuming there is no network partition, it is expected that after transitions T2 and T7, all power devices in the microgrid will select the same active MC.

The second case occurs when there is no available MC for the power device to select.
This happens when all MCs have missed their heartbeats, and the global, coordinating state *No Available MCs* is set (transition T8).
In this case, a No MC timer with a duration of 10 seconds is scheduled.
This timer is concurrent with the Lost Active MC timer: it is scheduled at the same time as the Lost Active MC timer if the active MC is the last MC that missed heartbeat, or after the Lost Active MC timer is scheduled if the last available MC becomes unavailable while the Lost Active MC timer is pending.

If a heartbeat is received from any MC while the No MC timer is pending, the No MC timer is cancelled, and the power device moves to the *No MC* state and starts a New MC timer to trigger the next selection of a new active MC.
However, if the No MC timer expires without any heartbeats received, the power device doesn’t have any active MC and has to process the `CONFIG_ON_COMMS_LOSS` configuration.

<br><br>

[^1]: Tactical Microgrid Communications and Control: <https://quicksearch.dla.mil/qsDocDetails.aspx?ident_number=285095>
[^2]: OMG Data Distribution Service: <https://www.omg.org/spec/DDS/1.4/About-DDS>
[^3]: OMG DDS Interoperability Wire Protocol: <https://www.omg.org/spec/DDSI-RTPS/2.5/About-DDSI-RTPS>
[^4]: OMG Extensible and Dynamic Topic Types for DDS: <https://www.omg.org/spec/DDS-XTypes/1.3/About-DDS-XTypes>
[^5]: OMG DDS Security: <https://www.omg.org/spec/DDS-SECURITY/1.1/About-DDS-SECURITY>
[^6]: OpenDDS GitHub Repository: <https://github.com/OpenDDS/OpenDDS>
[^7]: opendds-monitor GitHub Repository: <https://github.com/OpenDDS/opendds-monitor>
[^8]: The Demo Repository: <https://github.com/OpenDDS/DeveloperResources/tree/main/tactical-microgrid-standard>
[^9]: OpenDDS Documentation: <https://opendds.readthedocs.io/en/latest-release>
[^10]: OpenDDS Project: <https://opendds.org>
[^11]: Tactical Microgrid Standard Industry Day Slides, Public Release 2025
