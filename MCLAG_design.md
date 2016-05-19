# Overview
Multi Chassis LAG (MCLAG) is a feature that allows a LAG to span more than one switch. Traditionally, LAGs have been point-to-point links between a pair of network devices. Traffic gets load balanced among interfaces of the LAG, they help increase the aggregate bandwidth and link failure recovery is in the sub-second order. While LAGs offer link level redundancy, there is no node level redundancy and STP would be the way to achieve that. The problem with STP is that alternate links are blocked and so there is no load balancing or bandwidth gain and the typical reconvergence time when a node fails is not sub-second. MCLAG  attempts to bridge this gap by providing node level redundancy while still retaining the classical benefits of LAG like load balancing, increased aggregate bandwidth and sub-second failure recovery.  This is a proprietary feature with no cross vendor inter-operability and most vendors have an implementation of this feature on their product lines.

Typical MCLAG Topology:
``` ditaa

                                   +-------------------------------------+
                                   |                                     |
                                   |          SWITCH                     |
                                   |                                     |
                                   +---^--------------------------^------+
                                       |                          |
                                       |                          |
                    +------------------v---+                  +---v-------------------+
                    |                      |                  |                       |
                    |   MLAG SWITCH 1      |2   ISL          2|    MLAG SWITCH 2      |
                    |                      <------------------>                       |
                    |                      |                  |                       |
                    +------------------^---+                  +-----^-----------------+
                                      1|                           1|
                                       |                            |
                                       |                            |
                                       |                            |
                                       |                            |
                                   +---v----------------------------v-----+
                                   |                                      |
                                   |            switch/server             |
                                   |                                      |
                                   |                                      |
                                   +--------------------------------------+
```

Terminology
LAG	    Link Aggregation
LACP	Link Aggregation Control Protocol
MCLAG	Multi Chassis LAG (also MLAG)
ISL	    Inter Switch Link

# High level design of MCLAG
MCLAG feature is implemented by two daemons.
1. mclagd runs the main MCLAG state machine, ISL state machine and MAC table synchronization across the MCLAG peers.
2. mclagkad implements the keep alive state machine. It exposes the keepalive status to mclagd via OVSDB.

mclagd daemon also interacts with lacpd. vswitchd is used to configure the ASIC with the hardware configuration related to MCLAG.

## Design choices
MCLAG functionality is implemented in a daemon separate from LACP daemon. The differences are mostly specific to keeping two devices and their control and data planes in sync. For example:
-	MAC Address-Sync is only required for MCLAG and not a requirement for P2P LAGs
-	Split brain handling is only required for MCLAG and not P2P LAGs
-	MCLAG has additional H/W programming to do over and above the existing LACP block/unblock setting.

The keepalive functionality is implemented in a separate daemon. This is because keep alive functionality may run either over OOBM interface or on data port. Keeping it a separate daemon makes it easy to run it on any namespace.

## Participating modules

``` ditaa
+----------------------+                            +---------------------+
|                      |                            |                     |
|                      |                            |                     |
|        ops-cli       |                            |      ops-restd      |
|                      |                            |                     |
|                      |                            |                     |
+-----------^----------+                            +----------^----------+
            |                                                  |
            |                                                  |
            |                                                  |
            |                                                  |
+-----------v--------------------------------------------------v---------------------------+
|                                                                                          |
|                                  OVSDB                                                   |
|                                                                                          |
+-----------^-------------------------^------------------------^---------------------------+
            |                         |                        |
            |                         |                        |
            |                         |                        |
            |                         |                        |
+-----------v----------+  +-----------+----------+  +----------v----------+
|                      |  |                      |  |                     |
|                      |  |                      |  |                     |
|       ops-mclagkad   |  |       ops-mclagd     |  |     ops-switchd     |
|                      |  |                      |  |                     |
|                      |  |                      |  |                     |
+-----------------^----+  +----^-----------------+  +---------+-----------+
                  |            |                              | User Space
+----------------------------------------------------------------------------+------------->
                  |            |                              | Kernel Space
                  |       +----v-----+                        |
                  |       |          |                        |
                  |       |          |                        |
                  |       |  Linux   |                        |
                  +------->Interfaces+------------------------+
                          |          |                        |
                          |          |                        |
                          +----^-----+                        |
                               |                              | Kernel Space
+----------------------------------------------------------------------------+
                               |                              | Hardware
                          +-----------------------------------v----------+
                          |                                              |
                          |                         Hardware             |
                          |                                              |
                          +----------------------------------------------+
```

mclagd interacts with lacpd. Dependencies between these two are summarized below.
1. For interfaces which are configured as multi-chassis, mclagd determines the values of LACP related information like actor id and key. This information is used by lacpd to advertise LACP PDUs. Until these values are configured, LACPD does not advertise LACP PDUs for the ports
2. mclagd on each switch exchanges the MCLAG interface system ID and key information (along with some more fields) between the switches. LACP validates the information of both local and remote interfaces of a MCLAG before deciding whether it can unblock ports on the LAG.

mclagd interacts with mclagkad using status information described in the next section.

## OVSDB-Schema

### Configuring an MCLAG between two switches
Table MCLAG in OVSDB contains a single row for MCLAG configuration and status. This row is created with default values during bootup or with saved configuration values

There are three steps for creating a MCLAG between two switches.
1. Configure a LAG on each switch using the command interface lag <lagname>. This creates a port table row for the LAG
2. Set the LAG type to MCLAG using the command interface <lagname> multi-chassis.
3. Configure the ISL using the command mclag interswitch-link under the context of the interface which should be configured as ISL. This sets the MCLAG table row's ISL port with the uuid of ISL link.

Other configurations related to MCLAG are:
1. ISL Configuration: Hello interval, Hold time and Timeout
2. MCLAG Configuration: Device Priority and Split Brain (dual active or one-fragment-active)

### MCLAG status information
Following status information are available in MCLAG table.
MCLAG operational status: This column contains ISL link state, operational system ID and operational device priority.
MCLAG peer status: Peer ISL port name, 	Peer system ID and Peer device priority.
MCLAG Port Statistics: Contains ISL rx/tx statistics.

Following status information is added to the port table's new column 'mclag_status' in order for LACP daemon to advertise the correct information in LACPDUs.
actor_system_id
actor_port_id
actor_key

Table MCLAG_Remote_interfaces is created to provide the information about each remote interface.

### Hardware configuration
Following columns are added to the port table:
mac_learn_disable: If set to true, this disables MAC learning. This is used to disable MAC learn on ISL port
flood_block: If set to true, this disables flooding of traffic on the port. This is used to prevent flood traffic from looping back via remote MLAG switch's MLAG port
egress_redirect_to_port: This is used to configure the ISL port to which the egress of a MLAG port should be sent when that MCLAG port's link  goes down
egress_blocked_to_ports: This is used to prevent unicast traffic coming via an ISL link going to MLAG ports

### Keepalive configuration and status
This is stored in MCLAG table in keepalive_config, keepalive_timers and keepalive_status columns.

## References
None
