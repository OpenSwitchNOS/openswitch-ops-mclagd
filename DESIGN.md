# Component level design of ops-mclagd and ops-mclagkad

## Responsibilities of ops-mclagd
ops-mclagd implements the MCLAG state machine, ISL protocol, ISL state machine and MAC table synchronization between the switches in a MCLAG pair. It interacts with ops-lacpd and with ops-mclagkad. It drives into the database information needed by LACPd to validate and advertise LACPDUs for Multi-Chassis LAGs. It also drives specific port states into the database which is used by switchd to program the ASIC.

## Responsibilities of ops-mclagkad
ops-mclagkad implements the keepalive state machine by exchanging periodic keep alive packets over L3 interface between the MCLAG pairs. It reads keepalive configuration from OVSDB and updates the keepalive status to the DB for ops-mclagd to determine the split brain behavior.

## Design choices
ops-mclagd was created as a separate daemon to handle MCLAG specific responsibilities, instead of overloading ops-lacpd with the same. Since keepalive is a functionality that needs to run over OOBM or on data ports, it is decided to run it as a daemon separate from ops-mclagd.

## Relationships to external OpenSwitch entities

```ditaa

+-------------+      +-------------+       +----------+
|             +------>ISL interface+------->MLAG Peer |
|             |      +-------------+       +----------+
| ops-mclagd  |
|             |
|             |       +----------+
|             +-------|          |
+-------------+       |          |
                      |   OVSDB  |      +------------+     +--------+
                      |          |      |            |     |        |
+-------------+       |          +----->|switchd     +---->|ASIC    |
|             |       |          |      +------------+     +--------+
|             <-------|          |
|             |       +----------+
|ops-mclagkad |
|             |      +---------------+     +-----------+
|             +----->|               |     |           |
|             |      | L3 Interface  +---->|MLAG Peer  |
+-------------+      +---------------+     +-----------+
```


## OVSDB-Schema
MCLAG configuration and status is stored in the table named MCLAG. Some more information required for LACP daemon is stored in MCLAG_Remote_Interface table.

### Configuration columns:
ops-mclagd reds the following configuration information from MCLAG table.
isl_port: References port to be used as inter switch link.
isl_config: Contains key-value pairs for hello interval, timeout and hold time
split_brain: Configures how split brain is handled.
device_priority: MCLAG device priority. The switch with lower priority value is treated as the primary if ISL link going down and the split-brain is configured to have one fragment active.

### Status and statistics columns:
These status columns in MCLAG table are are updated by ops-mclagd:
oper_status: Contains islp_state, oper_system_id and oper_device_priority
peer_status: Contains information about the MCLAG peer switch i.e., peer_isl_port, peer_system_id and peer_device_priority
isl_statistics column is updated by the daemon which contains ISL link rx/tx statistics.

MCLAG_Remote_Interfaces table contains the following status columns for every remote interface on peer switch that is part of MCLAG aggregation. These are used by LACPD daemon to validate the formation of LAG.
lacp_partner_port_id: Remote partner port ID
lacp_partner_key: Remote partner LACP key

### Keepalive configuration and state
Keep alive configuration and state information is stored in MCLAG table in keepalive_config, keepalive_timers and keepalive_status columns.


## Internal structure of ops-mclagd

### This daemon contains following threads:

1. IDL thread: Subscribes to and handles notifications from MCLAG related tables (MCLAG, MCLAG-Remote-Interfaces, Port, Interface tables). It does not subscribe with the MAC table. It determines any changes to the configuration in OVSDB and sends messages to the other threads

2. MCLAGd thread: It runs the main MCLAG state machine (formation of MCLAG, handling of change between dual active and split brain status etc.).

3. ISLP thread: Implements ISL protocol, exchanging ISLP messages between the peer MLAG switch and other threads of this daemon

4. MACSync thread: Implements all MAC Synchronization operations

5. MACIDL thread: Subscribes to and handles notifications as well as transactions for the MAC table which contains all learned and programmed MAC entries

6. Timer thread: Runs all timer requests from other threads. It uses timerfd and epoll to run different timers and call the registered callbacks on timeout expiry

### Communication mechanism between threads:
MLAGd thread and Rx/TX threads communicate with other using POSIX message queues (mqueue). We investigated multiple mechanisms. We decided to use POSIX message queues due to following advantages:
- Can be used as a descriptor for select/poll
- Easy to use interface
- Simple code, no mutex and semaphores which are typically required when using linked list as message queues.
- Supports priority of messages

The main thread of the daemon creates following message queues:
1. ops-mlagd-mlagd: Name of Message queue for sending messages to MLAGd thread
2. ops-mlagd-islp: Name of message queue for sending messages to ISLP thread
3. ops-mclagd-macsync: Name of message queue for sending messages to the MAC Sync thread
Other threads which need to communicate with these threads obtain a mqueue descriptor calling mq_open with the name of the corresponding message queue and use mq_send to send the messages.

The ISLP thread also listens on a socket for ISL protocol messages. It uses epoll to wait for any messages on this socket and on the message queue.
The MLAGd thread waits on the message queue only and uses mq_receive on which it is blocked until a message is sent from any other thread.
IDL threads use the OVSDB IDL APIs to listen for notifications from the DB and receive the updates (idl_wait, poll_block and idl_run).

## Internal structure of ops-mclagkad
ops-mclagkad has very similar structure as ops-mclagd with multiple threads and mqueue for communication between them.

## Any other sections that are relevant for the module
None

## References
None
