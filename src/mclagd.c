/*
 * (c) Copyright 2015 Hewlett Packard Enterprise Development LP
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may
 * not use this file except in compliance with the License. You may obtain
 * a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
 * License for the specific language governing permissions and limitations
 * under the License.
 */

/************************************************************************/ /**
  * @ingroup mclagd
  *
  * @file
  * Main source file for the MCLAG daemon.
  *
  *    The mclagd daemon operates as an overall OpenSwitch Multi Chassis
  *    Link Aggregation Daemon supporting aggregation of LACP trunks
  *    across two switches
  *
  *    Its purpose in life is:
  *
  *       1. During start up, read port and interface related
  *          configuration data and maintain local cache.
  *       2. During operations, receive administrative
  *          configuration changes and apply to the hardware.
  *       3. Manage LACP protocol operation for trunks in MCLAG
  *       4. Dynamically configure hardware based on
  *          operational state changes as needed.
  ***************************************************************************/
#include <errno.h>
#include <getopt.h>
#include <mqueue.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <sys/types.h>
#include <dynamic-string.h>

#include <command-line.h>
#include <daemon.h>
#include <dirs.h>
#include <fatal-signal.h>
#include <openvswitch/vlog.h>
#include <poll-loop.h>
#include <unixctl.h>
#include <util.h>
#include <vswitch-idl.h>

/**
 * Main function for mclagd daemon.
 * @param argc is the number of command line arguments.
 * @param argv is an array of command line arguments.
 * @return 0 for success or exit status on daemon exit.
			 */
int
main(int argc, char *argv[])
{

return 0;
}
