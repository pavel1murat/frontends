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

CMD_STATUS_FINISHED_OK = 0
CMD_STATUS_IN_PROGRESS = 1

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
        default_common.equip_type = midas.EQ_PERIODIC
        default_common.buffer_name = "SYSTEM"
        default_common.trigger_mask = 0
        default_common.event_id = 500 + midas.frontend.frontend_index
        default_common.period_ms = 100
        default_common.read_when = midas.RO_RUNNING
        default_common.log_history = 1

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
            
class MyMultiFrontend(midas.frontend.FrontendBase):
    def __init__(self):
        # If using the frontend_index, encode the index in your equipment name.
        fe_name = "mu2e_config" #  % midas.frontend.frontend_index
        midas.frontend.FrontendBase.__init__(self, fe_name)
        
        # add equipment. Can add as many as needed.
        # Whereas the C frontend system only allows one polled
        # equipment per frontend, the python system allows multiple.
        self.add_equipment(PeriodicEquipment(self.client))
        
#------------------------------------------------------------------------------
# elog configuration
#-------v-----------------------------------------------------------------------
        self.elog = json.loads(open("config/elog.json").read());

        print(self.elog);

        # print("calling begin_of_run");
        # self.begin_of_run(105122);
        
        # print("calling end_of_run");
        # self.end_of_run(105122);
#------------------------------------------------------------------------------
# mu2e_config is called the latest - it sends BOR and EOR messages...
#-----------------------------------------------------------------------------
        self.client.set_transition_sequence(midas.TR_START, 700)
        self.client.set_transition_sequence(midas.TR_STOP , 700)

        self.client.odb_watch("/Mu2e/Commands/Configure/Run", self.process_command)
        TRACE.TRACE(7,f'constructor END',TRACE_NAME)
        print("constructor end");
        
    def begin_of_run(self, run_number):
        
        config_name             = self.client.odb_get("/Mu2e/ActiveRunConfiguration/Name")
        artdaq_partition_number = self.client.odb_get("/Mu2e/ARTDAQ_PARTITION_NUMBER")

        # cmd = '~/products/elog/elog  -x -s -n 1 -h ' + self.elog["host"] + ' -p '+self.elog['port'] \
        cmd = "elog  -x -s -n 1 -h " + self.elog["host"] + " -p "+self.elog['port'] \
        + ' -d elog -l ' + self.elog['logbook'] + ' -u ' + self.elog['user'] + ' ' + self.elog['passwd'] \
        + f' -a author=murat -a type=routine -a category="data taking"' \
        + f' -a subject="new run: {run_number} config:{config_name}"' \
        + f' "begin run {run_number} configuration:{config_name}"'

        TRACE.TRACE(7,f'begin_of_run command:{cmd}')
        proc = subprocess.Popen(cmd, shell=True,stdout=subprocess.PIPE,stderr=subprocess.PIPE,encoding="utf-8")

# search for 'Message successfully transmitted, ID=512', parse out ID

        output = proc.stdout.readlines()
        self.elog['start_run_message_id'] = None;
        for line in output:
            print(line.strip());
            if (line.find('Message successfully transmitted') == 0):
                 self.elog['start_run_message_id'] = line.strip().split('=')[1]
                 break;

        self.set_all_equipment_status("Running", "greenLight")
        self.client.msg("CONFIG_FE: run number %d started" % run_number)

        
    def end_of_run(self, run_number):

        config_name             = self.client.odb_get("/Mu2e/ActiveRunConfiguration/Name")
        artdaq_partition_number = self.client.odb_get("/Mu2e/ARTDAQ_PARTITION_NUMBER")
        
        message_id = self.elog['start_run_message_id'];
        # message_id = "512"
        if (message_id):
            # reply to a given message id
            print(f'replying to message ID:{message_id}')
            
#            cmd = f'~/products/elog/elog -r {message_id}'\
            cmd = f'elog -r {message_id}' \
            + ' -x -s -n 1 -h ' + self.elog['host'] + ' -p ' + self.elog['port']\
            + ' -d elog -l ' + self.elog['logbook'] + ' -u ' + self.elog['user'] + ' ' + self.elog['passwd']\
            + f' -a author=murat -a type=routine -a category="data taking"'\
            + f' -a subject="end of run {run_number} config:{config_name}"'\
            + f' " end of run {run_number} config:{config_name}"'
            
            TRACE.TRACE(8,f'end_of_run command:{cmd}',TRACE_NAME)
            proc = subprocess.Popen(cmd, shell=True,stdout=subprocess.PIPE,stderr=subprocess.PIPE,encoding="utf-8")
        
        self.set_all_equipment_status("Okay", "greenLight")
        self.client.msg("CONFIG_FE: end of run number %d" % run_number)
        
#---v--------------------------------------------------------------------------
# Configure command
#------------------------------------------------------------------------------
    def process_command(self, client, path, new_value):
        """
        callback : configure the detector, set 'Configure' back to zero when done
        """
        run = self.client.odb_get("/Mu2e/Commands/Configure/Run")

        print(">>> in process_command run = ",run);
        
                    #        TLOG(TLVL_INFO) << "Process Command:" << key.name << "Value:" << run
#        << " Parameters: "    << params   << " Status:" << status;

        if (run != CMD_EXECUTION_REQUEST):
#-------^----------------------------------------------------------------------
# likely, self-resetting the request
            
            print("finished....");
            return
#-----------^------------------------------------------------------------------
# otherwise, loop over all subdetectors in /Mu2e/ActiveConfig and pass the Configure
# command to subsystems
#-------v----------------------------------------------------------------------
        self.client.odb_set("/Mu2e/Commands/Configure/Status",CMD_STATUS_IN_PROGRESS)

        status  = 0;
        hkey    = self.client._odb_get_hkey("/Mu2e/ActiveConfig");
        subkeys = self.client._odb_enum_key(hkey);
        print(subkeys);
        for key in subkeys: 
            print(key, key[1], ".....",key[1].name,".....",key[1].type)
            if (key[1].type == midas.TID_KEY):
#-----------^------------------------------------------------------------------
# this is a subsystem, check if it is enabled
#---------------v--------------------------------------------------------------
                subsystem = key[1].name.decode("utf-8");
                path="/Mu2e/ActiveConfig/"+subsystem+"/Enabled"
                enabled = self.client.odb_get(path)
                if (enabled == 0): continue
#------------------------------------------------------------------------------
# subsystem is enabled, set status to "undefined"
# and set 1 to  /Mu2e/Commands/$subsystem/Configure/Run
# also may want to check if status allowed issuing a new command
#---------------v--------------------------------------------------------------
                cmd_path    = "/Mu2e/Commands/Configure/"+subsystem
                status_path = "/Mu2e/Commands/Configure/"+subsystem+"/Status"
                status      = self.client.odb_get(status_path)
                if (status == CMD_STATUS_IN_PROGRESS):
                    TRACE.ERROR("configure is already executed for subsystem="+subsystem)
                    continue;
                self.client.odb_set(cmd_path+"/Run",CMD_EXECUTION_REQUEST)
#---------------^--------------------------------------------------------------
# all configuration commands passed on, monitor results
# every second 
#-------v----------------------------------------------------------------------
        nleft = 0;
        for i in range(0,30):
            time.sleep(1.0)       # sleep for one second
            TRACE.TRACE(5,"eeeee "+subsystem)
            nleft = 0;
            for key in subkeys: 
                if (key[1].type == midas.TID_KEY):              # subsystem
                    subsystem = key[1].name.decode("utf-8");
                    TRACE.TRACE(4,"eeeee "+subsystem)
                    if (self.client.odb_get("/Mu2e/ActiveConfig/"+subsystem+"/Enabled") == 0): continue;

                    cmd_path  = "/Mu2e/Commands/Configure/"+subsystem;
                    run       = self.client.odb_get(cmd_path+"/Run");
                    if (run == CMD_EXECUTION_REQUEST):
                        # the command is still being executed
                        nleft += 1
                        continue
                    # apparently the command has finished, check status
                    status    = self.client.odb_get(cmd_path+"/Status")
                    TRACE.TRACE(4,"status : %i" % (status))
                    if (status == CMD_STATUS_IN_PROGRESS):
                        nleft += 1
#------------------------------------------------------------------------------
# check the number of not-finished configure processes
#-----------v------------------------------------------------------------------
            if (nleft == 0) : break;
#------------------------------------------------------------------------------
# check the number of not-finished configure processes
#-------v----------------------------------------------------------------------
        TRACE.TRACE(4,"nleft:%i"%(nleft))
        if (nleft == 0):
            # configure finished successfully
            self.client.odb_set("/Mu2e/Commands/Configure/Status",CMD_STATUS_FINISHED_OK)
        else:
            self.client.odb_set("/Mu2e/Commands/Configure/Status",-nleft)
#------------------------------------------------------------------------------
# and mark command execution as finished, done
#-------v----------------------------------------------------------------------
        self.client.odb_set(cmd_path+"/Run",CMD_EXECUTION_FINISHED)
        TRACE.TRACE(4,"FINISHED")
        return

if __name__ == "__main__":
    # We must call this function to parse the "-i" flag, so it is available
    # as `midas.frontend.frontend_index` when we init the frontend object.
    midas.frontend.parse_args()
    
    TRACE.Instance = "mu2e_config".encode();

    # Now we just create and run the frontend like in the basic example.
    with MyMultiFrontend() as my_fe:
        my_fe.run()
