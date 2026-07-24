#!/usr/bin/env python
# will register to complete the start transition the latest
# will send a message to elog
#------------------------------------------------------------------------------
"""
Example of a more advanced midas frontend that has
* multiple equipment (one polled, one periodic)
* support for the "-i" flag on the command line
* support for user-settings in the ODB
* an earlier transition sequence.

See `examples/basic_frontend.py` to see the core concepts first.

When running with the "-i" flag on the command line, we will set the global
variable `midas.frontend.frontend_index`. This means that you can run the
same program multiple times, but do different things based on the index
(e.g. if you have 4 digitizers, you could run the same program four times,
and connect to a different one based on "-i 1", "-i 2" etc).
"""
import time, os, sys, subprocess
import json
import midas
import midas.frontend
import midas.event
import random
import ctypes

import  TRACE
TRACE_NAME = "daqmon"

CMD_EXECUTION_FINISHED = 0
CMD_EXECUTION_REQUEST  = 1

CMD_STATUS_FINISHED_OK = 0
CMD_STATUS_IN_PROGRESS = 1

class PeriodicEquipment(midas.frontend.EquipmentBase
        equip_name = "daqmon" ):
    """
    This periodic equipment is very similar to the one in `examples/basic_frontend.py`
    """
    def __init__(self, client):
        # If using the frontend_index, you should encode the index in your
        # equipment name.
        equip_name = "daqmon" 
        
        # The default settings that will be found in the ODB at
        # /Equipment/MyMultiPeriodicEquipment_1/Common etc.
        # Again, the values specified in the code here only apply the first 
        # time a frontend runs; after that the values in the ODB are used.
        #
        # If you need the ODB values for some reason, they are available at
        # self.common.
        #
        # Note that we're setting a UNIQUE EVENT ID for each equipment based
        # on the frontend_index - this is important so you can later 
        # distinguish/assemble the events.
        default_common = midas.frontend.InitialEquipmentCommon()
        default_common.equip_type   = midas.EQ_PERIODIC
        default_common.buffer_name  = "SYSTEM"
        default_common.trigger_mask = 0
        default_common.event_id     = 500 + midas.frontend.frontend_index
        default_common.period_ms    = 100
        default_common.read_when    = midas.RO_RUNNING
        default_common.log_history  = 1

        # These settings will appear in the ODB at
        # /Equipment/MyMultiPeriodicEquipment_1/Settings etc. The settings can
        # be accessed at self.settings, and will automatically update if the 
        # ODB changes.
        default_settings = {"Prescale factor": 10, 
                            "Some array": [1, 2, 3],
                            "String array (specific size)": [ctypes.create_string_buffer(b"ABC", 32),
                                                             ctypes.create_string_buffer(b"DEF", 32)],
                            "String array (auto size)": ["uvw", "xyzzzz"]}
        
        # We MUST call __init__ from the base class as part of our __init__!
        midas.frontend.EquipmentBase.__init__(self, client, equip_name, default_common, default_settings)

        # This is just a variable we'll use to keep track of how long it's
        # been since we last sent an event to midas.
        self.prescale_count = 0
        
        # Set the status that appears on the midas status page.
        self.set_status("Initialized")


#------------------------------------------------------------------------------
# readout function propagates the status of failed elements,
# returns None, as no actual readout is performed
#------------------------------------------------------------------------------
    def readout_func(self):
        # check nodes
#         daq_nodes_hkey = self.client.odb_get_hkey("/Mu2e/ActiveRunConfiguration/DAQ/Nodes")
#         nodes = self.client._odb_enum_key(hkey);
#         TRACE.TRACE(TRACE.TLVL_DEBUG,f'nodes:{nodes}')
#         
#         for node in nodes: 
#             print(node, node[1], ".....",node[1].name,".....",node[1].type)
#             # if (node[1].type == midas.TID_KEY):
#                 
        
        return None

    def settings_changed_func(self):
        """
        You can define this function to be told about when the values in
        /Equipment/MyMultiPeriodicEquipment_1/Settings have changed.
        self.settings is updated automatically, and has already changed
        by this time this function is called.
        
        In this version, you just get told that a setting has changed
        (not specifically which setting has changed).
        """
        self.client.msg("High-level: Prescale factor is now %d" % self.settings["Prescale factor"])
        self.client.msg("High-level: Some array is now %s" % self.settings["Some array"])

    def detailed_settings_changed_func(self, path, idx, new_value):
        """
        You can define this function to be told about when the values in
        /Equipment/MyMultiPeriodicEquipment_1/Settings have changed.
        self.settings is updated automatically, and has already changed
        by this time this function is called.
        
        In this version you get told which setting has changed (down to
        specific array elements).
        """
        if idx is not None:
            self.client.msg("Low-level: %s[%d] is now %s" % (path, idx, new_value))
        else:
            self.client.msg("Low-level: %s is now %s" % (path, new_value))
            
class MyMultiFrontend(midas.frontend.FrontendBase):

    def __init__(self):
        TRACE.INFO("-- START",TRACE_NAME)
        # If using the frontend_index, encode the index in your equipment name.
        fe_name = "daqmon" #  % midas.frontend.frontend_index
        midas.frontend.FrontendBase.__init__(self, fe_name)
        
        # add equipment. Can add as many as needed.
        # Whereas the C frontend system only allows one polled
        # equipment per frontend, the python system allows multiple.
        self.add_equipment(PeriodicEquipment(self.client))
#------------------------------------------------------------------------------
# this frontend executes only global commands
#-------v-----------------------------------------------------------------------
        self.cmd_top_path    = "/Mu2e/Commands/Global"
#------------------------------------------------------------------------------
# elog configuration
#-------v-----------------------------------------------------------------------
        self.mu2e_config_dir = os.path.expandvars(self.client.odb_get('/Mu2e/ConfigDir'));

#------------------------------------------------------------------------------
# mu2e_config is called the latest - it sends BOR and EOR messages...
#-----------------------------------------------------------------------------
#         self.client.set_transition_sequence(midas.TR_START, 700)
#         self.client.set_transition_sequence(midas.TR_STOP , 700)
# 
#         self.client.odb_watch(self.cmd_top_path+"/Run", self.process_command)

        TRACE.INFO(f'-- END',TRACE_NAME)

                   
#------------------------------------------------------------------------------
# the configuration may change from one run to another,
# don't need to restart this frontend only because of that
# for now, this frontend also starts DQM 
#------------------------------------------------------------------------------
    def begin_of_run(self, run_number):
        TRACE.TRACE(TRACE.TLVL_DEBUG,f'-- BEGIN: run_number:{run_number}')

#------------------------------------------------------------------------------
# if requested, start DQM processes for all subsystems
# TODO: can do multithreading
# first, check is the subsystem is enabled
# if it is, check if the DQM for this subsystem is enabled 
#------------------------------------------------------------------------------
        TRACE.TRACE(TRACE.TLVL_DEBUG,f'-- before_dqm:{run_number}')
#------------------------------------------------------------------------------
# begin run message in elog
# however send it only if the DB is used - otherwise assume scrap running
#------------------------------------------------------------------------------
        TRACE.TRACE(TRACE.TLVL_DEBUG,f'-- END: run_number:{run_number}')
        return

        
    def end_of_run(self, run_number):

        TRACE.TRACE(TRACE.TLVL_DEBUG,f'-- start END_OF_RUN: {run_number}')

        return;


if __name__ == "__main__":
    # We must call this function to parse the "-i" flag, so it is available
    # as `midas.frontend.frontend_index` when we init the frontend object.
    midas.frontend.parse_args()
    
    TRACE.Instance = "daqmon".encode();

    # Now we just create and run the frontend like in the basic example.
    with MyMultiFrontend() as my_fe:
        my_fe.run()
