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
        # If using the frontend_index, encode the index in your equipment name.
        fe_name = "mu2e_config" #  % midas.frontend.frontend_index
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
        self.elog            = json.loads(open(f'{self.mu2e_config_dir}/elog.json').read());

        TRACE.TRACE(TRACE.TLVL_DEBUG,f'self.elog:{self.elog}',TRACE_NAME);
#------------------------------------------------------------------------------
# mu2e_config is called the latest - it sends BOR and EOR messages...
#-----------------------------------------------------------------------------
        self.client.set_transition_sequence(midas.TR_START, 700)
        self.client.set_transition_sequence(midas.TR_STOP , 700)

        self.client.odb_watch(self.cmd_top_path+"/Run", self.process_command)

        TRACE.TRACE(TRACE.TLVL_DEBUG,f'constructor END',TRACE_NAME)
        print("constructor end");

#------------------------------------------------------------------------------
#
#------------------------------------------------------------------------------
    def start_dqm_processes(self,run_number):
        config_name         = self.client.odb_get("/Mu2e/ActiveRunConfiguration/Name")
        partition_id        = self.client.odb_get('/Mu2e/ActiveRunConfiguration/DAQ/PartitionID')
        base_port_number    = self.client.odb_get('/Mu2e/ActiveRunConfiguration/DAQ/Tfm/base_port_number')
        ports_per_partition = self.client.odb_get('/Mu2e/ActiveRunConfiguration/DAQ/Tfm/ports_per_partition')

        subsystems =  ['Tracker', 'Calorimeter', 'CRV', 'STM', 'EXM', 'Trigger' ]
        for ss in subsystems:
            ss_enabled  = self.client.odb_get(f'/Mu2e/ActiveRunConfiguration/{ss}/Enabled')
            if (ss_enabled):
                dqm_enabled = self.client.odb_get(f'/Mu2e/ActiveRunConfiguration/DQM/{ss}/Enabled')
                if (dqm_enabled):
                    fcl_file = self.client.odb_get(f'/Mu2e/ActiveRunConfiguration/DQM/{ss}/FclFile')
                    cmd      = f'export ARTDAQ_RUN_NUMBER={run_number};'
                    cmd     += f' export ARTDAQ_PARTITION_NUMBER={partition_id};'
                    cmd     += f' export ARTDAQ_PORTS_PER_PARTITION={ports_per_partition};'
                    cmd     += f' export ARTDAQ_BASE_PORT_NUMBER={base_port_number};'
                    cmd     += f' mu2e -c config/{config_name}/{fcl_file} >| dq01_{run_number}.log  2>&1 &';  
                    TRACE.TRACE(TRACE.TLVL_DEBUG,f'subsystem:{ss} start DQM client:{cmd}')
                    proc = subprocess.Popen(cmd, shell=True,stdout=subprocess.PIPE,stderr=subprocess.PIPE,encoding="utf-8")
                    TRACE.TRACE(TRACE.TLVL_DEBUG,f'DQM client for subsystem:{ss} started')

#------------------------------------------------------------------------------
#
#------------------------------------------------------------------------------
    def send_begin_run_elog_message(self,run_number):

        config_name   = self.client.odb_get("/Mu2e/ActiveRunConfiguration/Name")
        
        nev_per_train = self.client.odb_get('/Mu2e/ActiveRunConfiguration/DAQ/CFO/NeventsPerTrain')
        ew_length     = self.client.odb_get('/Mu2e/ActiveRunConfiguration/DAQ/CFO/EventWindowSize')
        sleep_time_ms = self.client.odb_get('/Mu2e/ActiveRunConfiguration/DAQ/CFO/SleepTimeMs')
        cfo_emu_mode  = self.client.odb_get('/Mu2e/ActiveRunConfiguration/DAQ/CFO/EmulatedMode')

        fn = f'/tmp/begin_run_msg_{run_number}.txt'
        f = open(fn, "w")
        f.write(f'begin run:{run_number} configuration:{config_name}')
        f.write(f' CFO: emulated_mode:{cfo_emu_mode} run:{nev_per_train}/{ew_length}/{sleep_time_ms}')
        f.close()
#
        cmd = "elog  -x -s -n 1 -h " + self.elog["host"] + " -p "+self.elog['port'] \
        + ' -d elog -l ' + self.elog['logbook'] + ' -u ' + self.elog['user'] + ' ' + self.elog['passwd'] \
        + f' -a author=murat -a type=routine -a category="data taking"' \
        + f' -a subject="new run: {run_number} config:{config_name}"' \
        + f' -m {fn}'

        TRACE.TRACE(TRACE.TLVL_DEBUG,f'begin_of_run command:{cmd}')
        proc = subprocess.Popen(cmd, shell=True,stdout=subprocess.PIPE,stderr=subprocess.PIPE,encoding="utf-8")

# search for 'Message successfully transmitted, ID=512', parse out the message ID

        output = proc.stdout.readlines()
        self.elog['start_run_message_id'] = None;
        for line in output:
            print(line.strip());
            if (line.find('Message successfully transmitted') == 0):
                 self.elog['start_run_message_id'] = line.strip().split('=')[1]
                 break;
             
        msg_id = self.elog['start_run_message_id']
        TRACE.TRACE(TRACE.TLVL_DEBUG,f'-- END: run_number:{run_number} message_id:{msg_id}')
        
#------------------------------------------------------------------------------
# the configuration may change from one run to another,
# don't need to restart this frontend only because of that
# for now, this frontend also starts DQM 
#------------------------------------------------------------------------------
    def begin_of_run(self, run_number):
        TRACE.TRACE(TRACE.TLVL_DEBUG,f'-- BEGIN: run_number:{run_number}')

        self.use_runinfo_db = self.client.odb_get('/Mu2e/ActiveRunConfiguration/UseRunInfoDB')
#------------------------------------------------------------------------------
# if requested, start DQM processes for all subsystems
# TODO: can do multithreading
# first, check is the subsystem is enabled
# if it is, check if the DQM for this subsystem is enabled 
#------------------------------------------------------------------------------
        TRACE.TRACE(TRACE.TLVL_DEBUG,f'-- before_dqm:{run_number}')
        self.start_dqm_processes(run_number)
#------------------------------------------------------------------------------
# begin run message in elog
# however send it only if the DB is used - otherwise assume scrap running
#------------------------------------------------------------------------------
        TRACE.TRACE(TRACE.TLVL_DEBUG,f'-- before_elog:{run_number}')

        if (self.use_runinfo_db != 0):
            self.send_begin_run_elog_message(run_number);

        self.set_all_equipment_status("Running", "greenLight")
        self.client.msg("CONFIG_FE: run number %d started" % run_number)
        TRACE.TRACE(TRACE.TLVL_DEBUG,f'-- END: run_number:{run_number}')

        
    def end_of_run(self, run_number):

        TRACE.TRACE(TRACE.TLVL_DEBUG,f'-- start END_OF_RUN: {run_number}')

        if (self.use_runinfo_db):
            config_name  = self.client.odb_get("/Mu2e/ActiveRunConfiguration/Name")
            partition_id = self.client.odb_get('/Mu2e/ActiveRunConfiguration/DAQ/PartitionID')
        
            message_id = self.elog['start_run_message_id'];

            TRACE.TRACE(TRACE.TLVL_DEBUG,f'run_number:{run_number} config_name:{config_name} partition_id{partition_id} message_id:{message_id}',TRACE_NAME)

            # message_id = "512"
            if (message_id):
                # reply to a given message id
                print(f'replying to message ID:{message_id}')
            
                cmd = f'elog -r {message_id}' \
                    + ' -x -s -n 1 -h ' + self.elog['host'] + ' -p ' + self.elog['port']\
                    + ' -d elog -l ' + self.elog['logbook'] + ' -u ' + self.elog['user'] + ' ' + self.elog['passwd']\
                    + f' -a author=murat -a type=routine -a category="data taking"'\
                    + f' -a subject="end of run {run_number} config:{config_name}"'\
                    + f' " end of run {run_number} config:{config_name}"'
            
                TRACE.TRACE(TRACE.TLVL_DEBUG,f'end_of_run command:{cmd}',TRACE_NAME)
                proc = subprocess.Popen(cmd, shell=True,stdout=subprocess.PIPE,stderr=subprocess.PIPE,encoding="utf-8")
        
            self.set_all_equipment_status("Okay", "greenLight")
            self.client.msg("CONFIG_FE: end of run number %d" % run_number)
                
            TRACE.TRACE(TRACE.TLVL_DEBUG,f'-- END END_OF_RUN: {run_number}')
        return;

#---v--------------------------------------------------------------------------
# process 'configure' command
#------------------------------------------------------------------------------
    def cmd_configure(self):
        status  = 0;
        hkey    = self.client._odb_get_hkey("/Mu2e/ActiveRunConfiguration");
        subkeys = self.client._odb_enum_key(hkey);
        TRACE.TRACE(TRACE.TLVL_DEBUG,f'subkeys:{subkeys}')
        
        for key in subkeys: 
            print(key, key[1], ".....",key[1].name,".....",key[1].type)
            if (key[1].type == midas.TID_KEY):
#-----------^------------------------------------------------------------------
# this is a subsystem, check if it is enabled
#---------------v--------------------------------------------------------------
                subsystem      = key[1].name.decode("utf-8");
                subsystem_path = '/Mu2e/ActiveRunConfiguration/'+subsystem;
                enabled        = self.client.odb_get(path+'/Enabled')
                if (enabled == 0): continue
#------------------------------------------------------------------------------
# subsystem is enabled, set status to "undefined"
# and set 1 to  /Mu2e/Commands/$subsystem/Configure/Run
# also may want to check if status allowed issuing a new command
# for the moment, don't process parameters
#---------------v--------------------------------------------------------------
                subsystem_cmd_path = '/Mu2e/Commands/'+subsystem;
                subsystem_finished = self.client.odb_get(subsystem_cmd_path+'/Finished');
                if (subsystem_finished != 1):
#------------------------------------------------------------------------------
# subsystem didn't complete the previous command, ignore it and continue with other subsystems
#---------------v--------------------------------------------------------------
                    prev_cmd = self.client.odb_get(subsystem_cmd_path+'/Name');
                    TRACE.ERROR(f'subsystem:{subsystem} didnt complete previous command:{prev_cmd}')
                    continue
#------------------------------------------------------------------------------
# subsystem did complete the previous command, proceed
#---------------v--------------------------------------------------------------
                self.client.odb_set(subsystem_cmd_path+'/Name'         ,cmd_name);
                self.client.odb_set(subsystem_cmd_path+'/ParameterPath',cmd_parameter_path);
                self.client.odb_set(subsystem_cmd_path+'/Finished'     ,       0);
                self.client.odb_set(subsystem_cmd_path+'/Run'          ,       1);
#---------------^--------------------------------------------------------------
# return, wait part is common for all commands
#------------------------------------------------------------------------------
        return

#-------v-----------------------------------------------------------------------
# process_command is called when odb['/Mu2e/Commands/Global/Run'] is set to 1.
# before that:
#
# odb['/Mu2e/Commands/Global/Name'         ] : should be set to the command name
# odb['/Mu2e/Commands/Global/ParameterPath'] : path in ODB to the parameter record of this command
# odb['/Mu2e/Commands/Global/Finished'     ] : 0, will be set to 1 by the frontend when the command is executed
# odb['/Mu2e/Commands/Global/ReturnCode'   ] : 0, will be set by the frontend when the command is executed
#
# the caller chould be first checking for Finished != 0 
#---v--------------------------------------------------------------------------
    def process_command(self, client, path, new_value):
        """
        callback : configure the detector
        """
        TRACE.TRACE(TLVL_DEBUG,f'path:{path}',TRACE_NAME);
        run = self.client.odb_get(self.top_cmd_path+'/Run')
# supposed to be 1, set by the caller
        if (run != 1):
#-------^----------------------------------------------------------------------
# likely, self-resetting the request
#------------------------------------------------------------------------------
            TRACE.TRACE(TRACE.TLVL_ERROR,f'/Mu2e/Command/Doit:{doit}, BAIL OUT',TRACE_NAME);
            return
#-----------^------------------------------------------------------------------
# otherwise, loop over all subdetectors in /Mu2e/ActiveConfig and pass the Configure
# command to subsystems
#-------v----------------------------------------------------------------------
        cmd_name       = self.client.odb_get("/Mu2e/Commands/Global/Name")
        parameter_path = self.client.odb_get("/Mu2e/Commands/Global/ParameterPath")

        if (cmd_name.upper() == 'CONFIGURE'):
            process_configure();

#------------------------------------------------------------------------------
# the command passed to the subsystems, wait for 30 sec, monitor results every second
# the wait part is common, the timeout might be different though...
#-------v----------------------------------------------------------------------
        TRACE.TRACE(TRACE.TLVL_DEBUG,'-- the command passed on, wait for completion')
        hkey    = self.client._odb_get_hkey("/Mu2e/ActiveRunConfiguration");
        subkeys = self.client._odb_enum_key(hkey);

        TRACE.TRACE(TRACE.TLVL_DEBUG,f'subkeys:{subkeys}')
        nfailed = 0;
        nleft   = 0;
        for i in range(0,30):
            time.sleep(1.0)       # sleep for one second
            TRACE.TRACE(TRACE.TLVL_DEBUG,f'-- wait iteration:{i}')
            nleft = 0;
            for key in subkeys: 
                TRACE.TRACE(TRACE.TLVL_DEBUG,f'checking subsystem:{subsystem} enabled:{enabled}')
                if (key[1].type == midas.TID_KEY):              # subsystem
                    subsystem = key[1].name.decode("utf-8");
                    subsystem_path     = '/Mu2e/ActiveRunConfiguration/'+subsystem;
                    subsystem_cmd_path = '/Mu2e/Commands/'+subsystem;
                    enabled            = self.client.odb_get(subsystem_path+'/Enabled')
                    TRACE.TRACE(TRACE.TLVL_DEBUG,f'checking subsystem:{subsystem} enabled:{enabled}')
                    if ( enabled == 0): continue;

                    finished = self.client.odb_get(cmd_path+"/Finished");
                    if (finished == 0):
                        # the command is still being executed
                        TRACE.TRACE(TRACE.TLVL_DEBUG,f'subsystem:{subsystem} is still executing the command')
                        nleft += 1
                        continue
                    else:
                        # the command has finished, check status
                        return_code = self.client.odb_get(subsystem_cmd_path+"/ReturnCode");
                        if (return_code == 0):
                            result_checked = 1
                            status         = self.client.odb_get(cmd_path+"/Status")
                            TRACE.TRACE(TRACE.TLVL_DEBUG,f'subsystem:{subsystem} configured, status:{status}')
                            if (status < 0):
                                nfailed += 1
#------------------------------------------------------------------------------
# check the number of not-finished configure processes and set their status to -1
#-----------v------------------------------------------------------------------
            TRACE.TRACE(TRACE.TLVL_DEBUG,f'end of iteration:{i} nleft:{nleft} nfailed:{nfailed}')
            if (nleft == 0) : break;
#------------------------------------------------------------------------------
# wait period finished, check status
#-------v----------------------------------------------------------------------
        TRACE.TRACE(TRACE.TLVL_DEBUG,f'nleft:{nleft} nfailed:{nfailed}')
        rc = 0;
        if (nleft == 0) and (nfailed == 0):
#------------------------------------------------------------------------------
# all subsystems successfully executed the command
#------------------------------------------------------------------------------
            self.client.odb_set(self.cmd_top_path+'/ReturnCode',0)
        else:
            rc = -nleft-nfailed;
            TRACE.TRACE(TRACE.TLVL_ERROR,f'nleft:{nleft} nfailed:{nfailed}')
            self.client.odb_set(self.cmd_top_path+'/ReturnCode',-nleft-nfailed)
#------------------------------------------------------------------------------
# and mark command execution as finished, done
#-------v----------------------------------------------------------------------
        self.client.odb_set(self.cmd_top_path+'/Finished'     ,1 )
        self.client.odb_set(self.cmd_top_path+'/ReturnCode'   ,rc)
        TRACE.TRACE(TRACE.TLVL_DEBUG,"FINISHED")
            
        return

if __name__ == "__main__":
    # We must call this function to parse the "-i" flag, so it is available
    # as `midas.frontend.frontend_index` when we init the frontend object.
    midas.frontend.parse_args()
    
    TRACE.Instance = "mu2e_config".encode();

    # Now we just create and run the frontend like in the basic example.
    with MyMultiFrontend() as my_fe:
        my_fe.run()
