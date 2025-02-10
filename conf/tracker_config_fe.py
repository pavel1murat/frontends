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
TRACE_NAME = "mu2e_config"

CMD_EXECUTION_FINISHED = 0
CMD_EXECUTION_REQUEST  = 1

CMD_STATUS_FAILED      = -1
CMD_STATUS_FINISHED_OK =  0
CMD_STATUS_IN_PROGRESS =  1

class PeriodicEquipment(midas.frontend.EquipmentBase):
    """
    This periodic equipment is very similar to the one in `examples/basic_frontend.py`
    """
    def __init__(self, client):
        # If using the frontend_index, you should encode the index in your
        # equipment name.
        equip_name = "Mu2eConfigEq" 
        
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
        
    def readout_func(self):
        """
        In this periodic equipment, this function will be called periodically.
        It should return a `midas.event.Event` or None.
        """
        if self.prescale_count == self.settings["Prescale factor"]:
            event = midas.event.Event()
            data = [1,2,3,4,5,6,7,8]
            
            event.create_bank("MYBK", midas.TID_INT, data)
            event.create_bank("BNK2", midas.TID_BOOL, [True, False])
            
            self.prescale_count = 0
            return event
        else:
            self.prescale_count += 1
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
            
class TrackerConfigFrontend(midas.frontend.FrontendBase):

    def __init__(self):
        # If using the frontend_index, encode the index in your equipment name.
        fe_name = "mu2e_config" #  % midas.frontend.frontend_index
        midas.frontend.FrontendBase.__init__(self, fe_name)
        
        # add equipment. Can add as many as needed.
        # Whereas the C frontend system only allows one polled
        # equipment per frontend, the python system allows multiple.
        self.add_equipment(PeriodicEquipment(self.client))
#------------------------------------------------------------------------------
# tracekr_config is not doing anything, so the sequence is not important
#-----------------------------------------------------------------------------
#        self.client.set_transition_sequence(midas.TR_START, 700)
#        self.client.set_transition_sequence(midas.TR_STOP , 700)

        self.client.odb_watch("/Mu2e/Commands/Configure/Tracker/Run", self.process_command)
        TRACE.TRACE(TLVL_DEBUG,f'constructor END',TRACE_NAME)
        print("constructor end");

#------------------------------------------------------------------------------
# the configuration may change from one run to another,
# don't need to restart this frontend only because of that
#------------------------------------------------------------------------------
    def begin_of_run(self, run_number):

        config_name  = self.client.odb_get("/Mu2e/ActiveRunConfiguration/Name")
        partition_id = self.client.odb_get('/Mu2e/ActiveRunConfiguration/DAQ/PartitionID')

        self.set_all_equipment_status("Running", "greenLight")
        self.client.msg("CONFIG_FE: run number %d started" % run_number)
        return

        
    def end_of_run(self, run_number):

        config_name  = self.client.odb_get("/Mu2e/ActiveRunConfiguration/Name")
        partition_id = self.client.odb_get('/Mu2e/ActiveRunConfiguration/DAQ/PartitionID')
        
        self.set_all_equipment_status("Okay", "greenLight")
        self.client.msg("CONFIG_FE: end of run number %d" % run_number)
        return

#-------v-----------------------------------------------------------------------
# Configure command
#------------------------------------------------------------------------------
    def cmd_configure(self):
        
        n_active_stations = 0;
        for station in range(0,17):
            path_station = '/Mu2e/ActiveRunConfiguration/Tracker/Station_%02i'%station;
#------------------------------------------------------------------------------
# need to intialize the DTCs, send message to the frontend
#------------------------------------------------------------------------------
            for plane in range (0,1):
                path_plane = path_station+'/Plane_%02i'%plane;
                enabled = self.client.odb_get(path_plane+'/Enabled');
                if (enabled == 1):
                    pp   = self.client.odb_get_link_destination(path_plane+'/DTC').split('/');
#------------------------------------------------------------------------------
# rely on the path looking like '/Mu2e/DAQ/Nodes/nodename/DTC0'
# frontend is aware of which DTCs need to be initialized
#------------------------------------------------------------------------------
                    nodename = pp[len(pp)-2];
#------------------------------------------------------------------------------
# define the command
#------------------------------------------------------------------------------
                    cmd_path = '/Mu2e/ActiveRunConfiguration/DAQ/Nodes/'+nodename+'/Frontend/Command';
                    self.client.odb_set(cmd_path+'/Locked'       ,1)
                    self.client.odb_set(cmd_path+'/Name'         ,'configure')
                    self.client.odb_set(cmd_path+'/Status'       ,1);            # undefined ??
                    self.client.odb_set(cmd_path+'/ResultChecked',0);
                    self.client.odb_set(cmd_path+'/Doit'         ,1) ;

                    n_active_stations += 1;
                    TRACE.TRACE(TLVL_DEBUG,f'-- set {cmd_path}/Doit = 1, n_active_stations:{n_active_stations}')
                    break;
#------------------------------------------------------------------------------
# at this point, the frontend should detect the command, process it,
# specify the Status and set Doit = 0 
# end of loop over the stations
#-------v----------------------------------------------------------------------
        TRACE.TRACE(TLVL_DEBUG,f'-- END, configure sent to n_active_stations:{n_active_stations}')
        
        return
    
#---v--------------------------------------------------------------------------
# Process command
#------------------------------------------------------------------------------
    def process_command(self, client, path, new_value):
        """
        callback : configure the detector, set 'Configure' back to zero when done
        """
        TRACE.TRACE(TLVL_DEBUG,f'path:{path}',TRACE_NAME);
#------------------------------------------------------------------------------
# a sanity check: return of the whole tracker is disabled
#------------------------------------------------------------------------------
        subsystem_path = '/Mu2e/ActiveConfiguration/Tracker' 
        enabled        = self.client.odb_get(subsystem_path+'/Enabled');
        if (enabled == 0):
            TRACE.TRACE(TLVL_WARNING,f'Tracker is not enabled, bail out');
            self.client.odb_set(subsystem_path+'Status',0)
            return;

        subsystem_cmd_path = subsystem_path+'/Command'
        subsystem_doit     = self.client.odb_get(cmd_path+'/Doit')
        if (subsystem_doit != 1):
#-------^----------------------------------------------------------------------
# likely, self-resetting the request
#------------------------------------------------------------------------------
            TRACE.TRACE(TLVL_DEBUG,f'Tracker/Command/Doit:{subsystem_doit}');
            return

        self.client.odb_set(subsystem_cmd_path+'/Status',1)     # undefined
        self.client.odb_set(subsystem_cmd_path+'/State' ,1)     # command being executed
        
        cmd_name = self.client.odb_get(subsystem_cmd_path+'/Name');
        
        if (cmd_name.upper() == 'CONFIGURE'):
            cmd_configure()
#------------------------------------------------------------------------------
# for all global commands, wait is a common step (timeout should be a parameter, not a constant)
#-------v----------------------------------------------------------------------
        nfailed = 0;
        nleft   = 0;
        for i in range(0,30):
            time.sleep(1.0)       # sleep for one second
            TRACE.TRACE(TLVL_DEBUG,f'-- wait iteration:{i}')
            nleft = 0;
            for station in range(0,17):
                path_station = '/Mu2e/ActiveRunConfiguration/Tracker/Station_%02i'%station;
                enabled      = self.client.odb_get(path_station+'/Enabled');
                if (enabled == 0):  continue
                
                pp       = self.client.odb_get_link_destination(path_plane+'/DTC').split('/');
                nodename = pp[len(pp)-2];

                cmd_path = '/Mu2e/ActiveRunConfiguration/DAQ/Nodes/'+nodename+'/Frontend/Command';
                doit     = self.client.odb_get(cmd_path+'/Doit');
                if (doit == 1):
                    # not finished...
                    nleft += 1;
                    TRACE.TRACE(TLVL_DEBUG,f'-- set {cmd_path}/Doit = 1, nleft:{nleft}')
                else:
                    # finished, first see if the result is already checked
                    result_checked = self.client.odb_get(cmd_path+'/ResultChecked');
                    if (result_checked == 1) : continue
                    # finished, but not yet checked
                    status = self.client.odb_get(cmd_path+'/Status');
                    TRACE.TRACE(TLVL_DEBUG,f'station:{station} done, status:{status}')
                    if (status == CMD_STATUS_FAILED):
                        nfailed += 1
                    self.client.odb_set(cmd_path+'/ResultChecked',1);
                    
            TRACE.TRACE(TLVL_DEBUG,f'end of niter:{i} nleft:{nleft} nfailed:{nfailed}')

#-------^----------------------------------------------------------------------
# check the number of not-finished configure processes
#-------v----------------------------------------------------------------------
        TRACE.TRACE(TLVL_DEBUG,f'end of wait nleft:{nleft} nfailed:{nfailed}')
#------------------------------------------------------------------------------
# check the number of not-finished configure processes
#-------v----------------------------------------------------------------------
        if (nleft == 0) and (nfailed == 0):
            # configure finished successfully
            self.client.odb_set(subsystem_path+'/Status',CMD_STATUS_FINISHED_OK)
        else:
            TRACE.TRACE(TLVL_ERROR,f'nleft:{nleft} nfailed:{nfailed}')
            self.client.odb_set(subsystem_path+'/Status',-nleft-nfailed)
#------------------------------------------------------------------------------
# and mark command execution as finished, done
#-------v----------------------------------------------------------------------
        self.client.odb_set(subsystem_path+'/State'        ,0)
        self.client.odb_set(subsystem_path+'/Doit'         ,0)
#------------------------------------------------------------------------------
# finally, release locks
#-------v----------------------------------------------------------------------
        for station in range(0,17):
            path_station = '/Mu2e/ActiveRunConfiguration/Tracker/Station_%02i'%station;
            enabled      = self.client.odb_get(path_station+'/Enabled');
            if (enabled == 1):
                pp       = self.client.odb_get_link_destination(path_plane+'/DTC').split('/');
                nodename = pp[len(pp)-2];
                cmd_path = '/Mu2e/ActiveRunConfiguration/DAQ/Nodes/'+nodename+'/Frontend/Command';
                self.client.odb_set(cmd_path+'/Locked',0);
        
        TRACE.TRACE(TLVL_DEBUG,"FINISHED")

#------------------------------------------------------------------------------
# start configuring tracker, loop over stations
#------------------------------------------------------------------------------
        self.client.odb_set(cmd_path+'/Status',CMD_STATUS_IN_PROGRESS)  # 1
        return

#------------------------------------------------------------------------------
if __name__ == "__main__":
    # We must call this function to parse the "-i" flag, so it is available
    # as `midas.frontend.frontend_index` when we init the frontend object.
    midas.frontend.parse_args()
    
    TRACE.Instance = "tracker_config".encode();

    # Now we just create and run the frontend like in the basic example.
    with TrackerConfigFrontend() as fe:
        fe.run()
