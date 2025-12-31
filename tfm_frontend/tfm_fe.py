#!/usr/bin/env python
#------------------------------------------------------------------------------
# Namitha: TFM frontend - python implementation
# frontend name : python_tfm_fe
# with transition to spack, no longer need to update the PYTHONPATH
#------------------------------------------------------------------------------
import  ctypes, os, sys, datetime, random, time, traceback, subprocess
import  xmlrpc.client
import  inspect

import  midas
import  midas.frontend 
import  midas.event

import  TRACE
TRACE_NAME = "tfm_fe"

import tfm.rc.control.farm_manager as farm_manager

# sys.path.append(os.environ["FRONTENDS_DIR"])
from frontends.utils.runinfodb import RuninfoDB

#------------------------------------------------------------------------------
# TFM 'equipment' is just a placeholder
# 'client' is the Midas frontend client which connects to ODB
#------------------------------------------------------------------------------
class TfmEquipment(midas.frontend.EquipmentBase):
    def __init__(self, client):
        TRACE.TRACE(TRACE.TLVL_DBG,"-- START",TRACE_NAME)
#------------------------------------------------------------------------------
# Define the "common" settings of a frontend. These will appear in
# /Equipment/MyPeriodicEquipment/Common. 
#
# Note: The values set here are only used the very first time this frontend/equipment 
#       runs; after that the ODB settings are used.
# You MUST call midas.frontend.EquipmentBase.__init__ in your equipment's __init__ method!
#------------------------------------------------------------------------------
        settings              = midas.frontend.InitialEquipmentCommon()
        settings.equip_type   = midas.EQ_PERIODIC
        settings.buffer_name  = "SYSTEM"
        settings.trigger_mask = 0
        settings.event_id     = 1
        settings.period_ms    = 10000
        settings.read_when    = midas.RO_RUNNING
        settings.log_history  = 1

        equip_name            = "tfm_eq"
        midas.frontend.EquipmentBase.__init__(self, client, equip_name, settings)

        self._fm              = None;
#------------------------------------------------------------------------------
# set the status of the equipment (appears in the midas status page)
#------------------------------------------------------------------------------
        self.set_status("Initialized")
        TRACE.TRACE(TRACE.TLVL_LOG,":002: --- END equipment initialized",TRACE_NAME)
        return;

#-------^----------------------------------------------------------------------
# For a periodic equipment, this function will be called periodically
# (every 10 s in this case - see period_ms above). It should return either a `midas.event.Event`
# or None (if we shouldn't write an event).
#------------------------------------------------------------------------------
    def readout_func(self):
        TRACE.TRACE(TRACE.TLVL_DBG+1,":001: -- START",TRACE_NAME)
        # In this example, we just make a simple event with one bank.

        # event = midas.event.Event()

        # Create a bank (called "MYBK") which in this case will store 8 ints.
        # data can be a list, a tuple or a numpy array.
        # data = [1,2,3,4,5,6,TRACE.TLVL_LOG,8]
        # event.create_bank("MYBK", midas.TID_INT, data)

        TRACE.TRACE(TRACE.TLVL_DBG+1,"-- END",TRACE_NAME)
        return None;        # event

#------------------------------------------------------------------------------
# FE name : 'python_tfm_fe', to distinguish from the C++ frontend
#    A frontend contains a collection of equipment.
#    You can access self.client to access the ODB etc (see `midas.client.MidasClient`).
#------------------------------------------------------------------------------
class TfmFrontend(midas.frontend.FrontendBase):
#------------------------------------------------------------------------------
# TFM logfiles are stored in $TFM_LOGDIR/tfm.
# $TFM_LOGDIR is defined by tfm/bin/tfm_configure which is sourced by
# $MU2E_DAQ_DIR/daq_scripts/start_farm_manager, so it can be relied upon
# - redefines the sys.stdout 
#---v--------------------------------------------------------------------------
    def get_logfile(self,output_dir):
        TRACE.TRACE(TRACE.TLVL_LOG,"--- START")
        current_datetime = datetime.datetime.now()
        timestamp        = current_datetime.strftime("%Y-%m-%d_%H-%M-%S")

        tfm_logfile      = "undefined"
        tfm_logdir       = output_dir+'/logs/tfm'

        os.makedirs(tfm_logdir, exist_ok=True)
        if os.path.exists(tfm_logdir):
            tfm_logfile = os.path.join(tfm_logdir, f"python_tfm_fe_{timestamp}.log")
        else:
            print("Failed to create the log file directory %s." % tfm_logdir)

        TRACE.TRACE(TRACE.TLVL_LOG,"--- END tfm_logfile=%s"%tfm_logfile)
        return tfm_logfile

#------------------------------------------------------------------------------
# define needed env variables
#------------------------------------------------------------------------------
    def __init__(self):
        TRACE.TRACE(TRACE.TLVL_LOG,"0010: START")
        midas.frontend.FrontendBase.__init__(self, "tfm_fe")
        TRACE.TRACE(TRACE.TLVL_LOG,"0011: FrontendBase initialized")
#------------------------------------------------------------------------------
# determine active configuration
#------------------------------------------------------------------------------
        self._stop_run               = False;
        self.output_dir              = os.path.expandvars(self.client.odb_get("/Mu2e/OutputDir"));
        self.config_name             = self.client.odb_get("/Mu2e/ActiveRunConfiguration/Name")
        self.artdaq_partition_number = self.client.odb_get("/Mu2e/ActiveRunConfiguration/DAQ/PartitionID")
        self.cmd_top_path            = "/Mu2e/Commands/DAQ/Tfm"
        self.tfm_odb_path            = "/Mu2e/ActiveRunConfiguration/DAQ/Tfm"

        TRACE.INFO(f':0014: artdaq_partition_number={self.artdaq_partition_number}')

        config_path                  = "/Mu2e/RunConfigurations/"+self.config_name;
        self.use_runinfo_db          = self.client.odb_get(config_path+'/UseRunInfoDB')
        self.tfm_rpc_host            = self.client.odb_get(self.tfm_odb_path+'/RpcHost')
        self.artdaq_delay_ms         = self.client.odb_get(self.tfm_odb_path+'/artdaq_delay_ms')

        mu2e_config_dir              = os.path.expandvars(self.client.odb_get("/Mu2e/ConfigDir"));
        artdaq_config_dir            = mu2e_config_dir+'/artdaq';

        TRACE.INFO(f":0015:artdaq_config_dir={artdaq_config_dir} use_runinfo_db={self.use_runinfo_db} rpc_host={self.tfm_rpc_host}")

        os.environ["TFM_SETUP_FHICLCPP"] = f"{artdaq_config_dir}/.setup_fhiclcpp"

        self.tfm_logfile = self.get_logfile(self.output_dir)
        os.environ["TFM_LOGFILE"       ] = self.tfm_logfile
#------------------------------------------------------------------------------
# redefine STDOUT
#------------------------------------------------------------------------------
        sys.stdout  = open(self.tfm_logfile, 'w')
        TRACE.INFO("0016: after get_logfile")
#------------------------------------------------------------------------------
# You can add equipment at any time before you call `run()`, but doing
# it in __init__() seems logical.
#-------v----------------------------------------------------------------------
        self.add_equipment(TfmEquipment(self.client))
        TRACE.INFO(f'003: equipment added',TRACE_NAME)

        self._fm   = farm_manager.FarmManager(odb_client       =self.client,
                                              artdaq_config_dir=artdaq_config_dir,
                                              rpc_host         =self.tfm_rpc_host);
        
        TRACE.TRACE(TRACE.TLVL_LOG,"004: tfm instantiated, self.use_runinfo_db=%i"%(self.use_runinfo_db))

        cmd=f"cat {os.getenv('MIDAS_EXPTAB')} | awk -v expt={os.getenv('MIDAS_EXPT_NAME')} '{{if ($1==expt) {{print $2}} }}'"
        process = subprocess.Popen(cmd,stdout=subprocess.PIPE,stderr=subprocess.PIPE,shell=True)
        stdout, stderr = process.communicate();
        self.message_fn = stdout.decode('utf-8').split()[0]+'/tfm.log';
#------------------------------------------------------------------------------
# runinfo DB related stuff
#-------v----------------------------------------------------------------------
        self.runinfo_db   = None;
        if (self.use_runinfo_db):
            rdb_config_file = mu2e_config_dir+'/runinfodb.json';
            self.runinfo_db = RuninfoDB(midas_client=self.client,config_file=rdb_config_file);
            if (self.runinfo_db.error):
#------------------------------------------------------------------------------
# an error has been detected, parse
#-----------------------------------------------------------------------------
                self.client.msg(self.runinfo_db.error,is_error = True)
                raise Exception("tfm_fe::__init__ : problem connecting to Postgresql DB")
#------------------------------------------------------------------------------
# TFM frontend starts after the DTC-ctl frontends but before the cfo_emu_frontend(520)
#-----------------------------------------------------------------------------
        self.client.set_transition_sequence(midas.TR_START,510)
#------------------------------------------------------------------------------
# register hotlink
# try to change priority by re-registering the same callback
#------------------------------------------------------------------------------
        self.client.odb_watch(self.cmd_top_path+'/Run', self.process_command)
        # self.client.register_transition_callback(midas.TR_START, 502, self._tr_start_callback)
#------------------------------------------------------------------------------
# start screen process tailing the logfile
#-------v----------------------------------------------------------------------
        cmd = f'/usr/bin/screen -dmS tfm_{self.artdaq_partition_number} /usr/bin/bash -c "tail -f {self.tfm_logfile}"';
        p   = subprocess.Popen(cmd,shell=True)

        self._fm.do_boot()
        TRACE.TRACE(TRACE.TLVL_LOG,":005: --- END: boot done")
        return;

#------------------------------------------------------------------------------
# on exit, also kill the tail logfile process
#------------------------------------------------------------------------------
    def __del__(self):
        TRACE.TRACE(TRACE.TLVL_LOG,"001: destructor START",TRACE_NAME)
        self._fm.__del__();

        cmd = f"x=`ps -efl | grep \"tail -f {self.tfm_logfile}\" | " + "awk '{print $4}' | grep -v grep`;"
        cmd += " if [ -z \"$x\" ] ; then xargs kill -1 $x ; fi"
        TRACE.TRACE(TRACE.TLVL_LOG,f"002: executing {cmd}",TRACE_NAME)
        p   = subprocess.Popen(cmd,shell=True)

        TRACE.TRACE(TRACE.TLVL_LOG,"002: destructor END",TRACE_NAME)

#------------------------------------------------------------------------------
# This function will be called at the beginning of the run.
# You don't have to define it, but you probably should.
# You can access individual equipment classes through the `self.equipment`
# dict if needed.
#------------------------------------------------------------------------------
    def begin_of_run(self, run_number):

        if (self.use_runinfo_db):
            try:
                db = runinfo_db("aaa");
                rc = db.register_transition(run_number,runinfo.START,0);
            except:
                TRACE.ERROR("failed to register beginning of the START transition")

        self.set_all_equipment_status("Run starting", "yellow")

        self._fm.do_config(run_number=run_number)
        self._fm.do_start_running()
        TRACE.TRACE(TRACE.TLVL_LOG,"001:BEGIN_OF_RUN")

        if (self.use_runinfo_db):
#------------------------------------------------------------------------------
# for each transition need to know the transition time
# register end of the START transition
#------------------------------------------------------------------------------
            try:
                db = runinfo_db("aaa");
                rc = db.register_transition(run_number,runinfo.START,1);
            except:
                TRACE.ERROR("failed to register the end of the START transition")

        sleep_time = self.artdaq_delay_ms/1000;
        self.client.msg(f'TFM: wait for {sleep_time} seconds before starting run number {run_number}')
        time.sleep(sleep_time);
       
        self.set_all_equipment_status("Running", "greenLight")

        return midas.status_codes["SUCCESS"]

#------------------------------------------------------------------------------
#
#---v--------------------------------------------------------------------------
    def end_of_run(self, run_number):
        TRACE.TRACE(TRACE.TLVL_DBG,f'-- START: self.use_runinfo_db:{self.use_runinfo_db}')
        if (self.use_runinfo_db):
#------------------------------------------------------------------------------
# register beginning of the STOP transition
#------------------------------------------------------------------------------
            try:
                db = runinfo_db("aaa");
                rc = db.register_transition(run_number,runinfo.STOP,0);
            except:
                TRACE.ERROR("failed to register beginning of the END_RUN transition")

        self._fm.do_stop_running()

        if (self.use_runinfo_db):
#------------------------------------------------------------------------------
# register end of the STOP transition
#------------------------------------------------------------------------------
            try:
                db = runinfo_db("aaa");
                rc = db.register_transition(run_number,runinfo.STOP,1);
            except:
                TRACE.ERROR("failed to register end of the END_RUN transition")


        self.set_all_equipment_status("Finished", "greenLight")
        self.client.msg("Frontend has seen end of run number %d" % run_number)

        TRACE.TRACE(TRACE.TLVL_DBG,"-- END")
        return midas.status_codes["SUCCESS"]

#------------------------------------------------------------------------------
#        Most people won't need to define this function, but you can use
#        it for final cleanup if needed.
# need to break the loop of TfmFrontend::run
#---v--------------------------------------------------------------------------
    def frontend_exit(self):
        # breakpoint()
        TRACE.TRACE(TRACE.TLVL_LOG,"001:START : set self._stop_run = True")
        self._stop_run = True;
        self._fm.shutdown();
        TRACE.TRACE(TRACE.TLVL_LOG,"002: DONE")

    def send_message(self, message, message_type = midas.MT_INFO, facility="midas"):
        """
        Send a message into the midas message log.
        
        These messages are stored in a text file, and are visible on the the
        "messages" webpage.
        
        Args:
            * message (str) - The actual message.
            * is_error (bool) - Whether this message is informational or an 
                error message. Error messages are highlighted in red on the
                message page.
            * facility (str) - The log file to write to. Vast majority of
                messages are written to the "midas" facility.
        """
        
        # Find out where this function was called from. We go up
        # 1 frame in the stack to get to the lowest-level user
        # function that called us.
        # 0. fn_A()
        # 1. fn_B() # <--- Get this function
        # 2. midas.client.msg()
        caller     = inspect.getframeinfo(inspect.stack()[1][0])
        filename   = ctypes.create_string_buffer(bytes(caller.filename, "utf-8"))
        line       = ctypes.c_int(caller.lineno)
        routine    = ctypes.create_string_buffer(bytes(caller.function, "utf-8"))
        c_facility = ctypes.create_string_buffer(bytes(facility, "utf-8"))
        c_msg      = ctypes.create_string_buffer(bytes(message, "utf-8"))
        msg_type   = ctypes.c_int(message_type)
    
        self.client.lib.c_cm_msg(msg_type, filename, line, c_facility, routine, c_msg)


#------------------------------------------------------------------------------
    def process_cmd_configure(self,parameter_path):
        rc = 0;
        return rc;

#------------------------------------------------------------------------------
    def process_cmd_reset_output(self,parameter_path):
        file = open(self.message_fn, 'w');
        file.close();
        return 0;

#------------------------------------------------------------------------------
# TODO: handle parameters
# given that TFM is a data member, no real need to send messages
# so this is just an exercise
#------------------------------------------------------------------------------
    def process_cmd_get_state(self,parameter_path):
        rc = 0;
        
        rpc_port = self._fm.rpc_port();
        tfm_url = f'http://mu2edaq22-ctrl.fnal.gov:{rpc_port}';   ## TODO - fix URL
        s   = xmlrpc.client.ServerProxy(tfm_url)
        res = s.get_state("daqint")
        
        TRACE.TRACE(TRACE.TLVL_LOG,f'res:{res}',TRACE_NAME);
#-------^----------------------------------------------------------------------
# the remaining part - print output to the proper message stream ,
# reverting the line order
#-------v----------------------------------------------------------------------
        message = "";
        lines  = res.splitlines();
        for line in reversed(lines):
            message = message+line;

#        self.send_message(message,midas.MT_DEBUG,"tfm");
        self.client.msg(message,0,"tfm");
        return rc;
    
#------------------------------------------------------------------------------
# generate fcl's for a given number of artdaq processes for a given run configuration
# host    = 'all' : generate FCL's for all hosts
# process = 'all' : generate FCLs for all processes
#------------------------------------------------------------------------------
    def process_cmd_generate_fcl(self,parameter_path):
        rc = 0;
        
        TRACE.TRACE(TRACE.TLVL_INFO,f'-- START: parameter_path:{parameter_path}',TRACE_NAME);

        ppath          = parameter_path; # +'/generate_fcl'
        par            = self.client.odb_get(ppath);
        run_conf       = par["run_conf"];
        host           = par["host"    ];
        artdaq_process = par["process" ];
        diag_level     = par["print_level"];

        cmd=os.getenv('MU2E_DAQ_DIR')+f'/config/scripts/generate_artdaq_fcl.py --run_conf={run_conf} --host={host} --process={artdaq_process} --diag_level={diag_level}';
        p = subprocess.Popen(cmd,stdout=subprocess.PIPE,stderr=subprocess.PIPE,shell=True,text=True)
        stdout, stderr = p.communicate();
#------------------------------------------------------------------------------
# write output
#------------------------------------------------------------------------------
        lines = stdout.split('\n');
        with open(self.message_fn,"a") as logfile:
            for line in reversed(lines):
                    logfile.write(line+'\n')
#------------------------------------------------------------------------------
# write error output, if any
#------------------------------------------------------------------------------
        if (stderr != ''):
            lines = stderr.split('\n');
            with open(self.message_fn,"a") as logfile:
                for line in reversed(lines):
                    logfile.write(line+"\n")

        TRACE.TRACE(TRACE.TLVL_INFO,f'-- END: run_conf:{run_conf} host:{host} process:{artdaq_process}',TRACE_NAME);
        return rc;
    
#------------------------------------------------------------------------------
# FCL file is defined by the run configuration and the process, host is not needed
# usual steps:
# 1. set state to BUSY  (1:yellow)
# 2. print fcl file to tfm.log
# 3. set state to READY (0:green)
#------------------------------------------------------------------------------
    def process_cmd_print_fcl(self,parameter_path):
        rc = 0;
        
        TRACE.INFO(f'-- START: parameter_path:{parameter_path}',TRACE_NAME);

        ppath    = parameter_path; ##            +'/print_fcl'
        par      = self.client.odb_get(ppath);
        run_conf = par["run_conf"];
        host     = par["host"    ];
        process  = par["process" ];

        fcl_file = os.getenv("MU2E_DAQ_DIR")+f'/config/{run_conf}/{process}.fcl'
        
        TRACE.TRACE(TRACE.TLVL_DEBUG,f'fcl_file:{fcl_file} logfile:{self.message_fn}',TRACE_NAME);
#------------------------------------------------------------------------------
# remember that MIDAS displays the logfile in the reverse order
#------------------------------------------------------------------------------
        with open(fcl_file) as f:
            lines = f.readlines();
            with open(self.message_fn,"a") as logfile:
                for line in reversed(lines):
                    logfile.write(line)


        TRACE.TRACE(TRACE.TLVL_INFO,f'-- END',TRACE_NAME);
        return rc;
    
#-------v-----------------------------------------------------------------------
# process_command is called when odb['/Mu2e/Commands/DAQ/Tfm/Run'] = 1
# in the end, it should set is back to zero
# a caller chould be first checking id Doit == 0 
#---v--------------------------------------------------------------------------
    def process_command(self, client, path, new_value):
        """
        callback : 
        """
        run      = self.client.odb_get(self.cmd_top_path+'/Run' )
        cmd_name = self.client.odb_get(self.cmd_top_path+'/Name')
        
        TRACE.TRACE(TRACE.TLVL_DEBUG,f'path:{path} cmd_name:{cmd_name} run:{run}',TRACE_NAME);
        if (run != 1):
#-------^----------------------------------------------------------------------
# likely, self-resetting the request
#------------------------------------------------------------------------------
            TRACE.TRACE(TRACE.TLVL_WARNING,f'{self.cmd_top_path}/Run:{run}, BAIL OUT',TRACE_NAME);
            return
#-------v----------------------------------------------------------------------
        parameter_path = self.client.odb_get(self.cmd_top_path+'/ParameterPath')
#------------------------------------------------------------------------------
# mark TFM as busy
#------------------------------------------------------------------------------
        self.client.odb_set(self.tfm_odb_path+'/Status',1)

        rc = 0;
        if   (cmd_name.upper() == 'CONFIGURE'):
            rc = self.process_cmd_configure(parameter_path);
        elif (cmd_name.upper() == 'STOP_RUN'):
            rc = self.client.stop_run(True);
        elif (cmd_name.upper() == 'GENERATE_FCL'):
            rc = self.process_cmd_generate_fcl(parameter_path);
        elif (cmd_name.upper() == 'GET_STATE'):
            rc = self.process_cmd_get_state(parameter_path);
        elif (cmd_name.upper() == 'PRINT_FCL'):
            rc = self.process_cmd_print_fcl(parameter_path);
        elif (cmd_name.upper() == 'RESET_OUTPUT'):
            rc = self.process_cmd_reset_output(parameter_path);
#------------------------------------------------------------------------------
# when done, set state to READY
#------------------------------------------------------------------------------
        self.client.odb_set(self.tfm_odb_path+'/Status',0)

        return
    


if __name__ == "__main__":
#------------------------------------------------------------------------------
# The main executable is very simple:
# just create the frontend object, and call run() on it.
#---v--------------------------------------------------------------------------
    TRACE.Instance = "tfm_fe".encode();

    TRACE.TRACE(TRACE.TLVL_LOG,"000: TRACE.Instance : %s"%TRACE.Instance,TRACE_NAME)
    with TfmFrontend() as fe:
        # breakpoint()
        TRACE.LOG("001: in the loop",TRACE_NAME)
        fe.run()
        TRACE.LOG("002: after frontend::run",TRACE_NAME)
        

    TRACE.LOG("003: DONE, exiting")

        
