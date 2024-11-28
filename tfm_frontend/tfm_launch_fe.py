#!/usr/bin/env python
#------------------------------------------------------------------------------
# Namitha: TFM launch frontend - python implementation
# frontend name : python_tfm_launch_fe
# with transition to spack, no longer need to update the PYTHONPATH
#------------------------------------------------------------------------------
import  ctypes, os, sys, datetime, random, time, traceback, subprocess

import  midas
import  midas.frontend 
import  midas.event

import  TRACE
TRACE_NAME = "tfm_launch"

import tfm.rc.control.farm_manager as farm_manager

# sys.path.append(os.environ["FRONTENDS_DIR"])
from frontends.utils.runinfodb import RuninfoDB

#------------------------------------------------------------------------------
# TFM 'equipment' is just a placeholder
# 'client' is the Midas frontend client which connects to ODB
#------------------------------------------------------------------------------
class TfmEquipment(midas.frontend.EquipmentBase):
    def __init__(self, client):
        TRACE.TRACE(7,":001:START",TRACE_NAME)
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

        equip_name            = "tfm_launch_eq"
        midas.frontend.EquipmentBase.__init__(self, client, equip_name, settings)

        self._fm              = None;
#------------------------------------------------------------------------------
# set the status of the equipment (appears in the midas status page)
#------------------------------------------------------------------------------
        self.set_status("Initialized")
        TRACE.TRACE(7,":002:END equipment initialized",TRACE_NAME)
        return;

#-------^----------------------------------------------------------------------
# For a periodic equipment, this function will be called periodically
# (every 10 s in this case - see period_ms above). It should return either a `midas.event.Event`
# or None (if we shouldn't write an event).
#------------------------------------------------------------------------------
    def readout_func(self):
        TRACE.TRACE(7,":001",TRACE_NAME)
        # In this example, we just make a simple event with one bank.

        # event = midas.event.Event()

        # Create a bank (called "MYBK") which in this case will store 8 ints.
        # data can be a list, a tuple or a numpy array.
        # data = [1,2,3,4,5,6,7,8]
        # event.create_bank("MYBK", midas.TID_INT, data)

        TRACE.TRACE(7,":002:exit",TRACE_NAME)
        return None;        # event

#------------------------------------------------------------------------------
# FE name : 'python_tfm_launch_fe', to distinguish from the C++ frontend
#    A frontend contains a collection of equipment.
#    You can access self.client to access the ODB etc (see `midas.client.MidasClient`).
#------------------------------------------------------------------------------
# class TfmLaunchFrontend(frontends.tfm_frontend.m_frontend.FrontendBase):
class TfmLaunchFrontend(midas.frontend.FrontendBase):
#------------------------------------------------------------------------------
# TFM logfiles are stored in $TFM_LOGDIR/tfm.
# $TFM_LOGDIR is defined by tfm/bin/tfm_configure which is sourced by
# $MU2E_DAQ_DIR/daq_scripts/start_farm_manager, so it can be relied upon
# - redefines the sys.stdout 
#---v--------------------------------------------------------------------------
    def get_logfile(self,output_dir):
        TRACE.TRACE(7,"START")
        current_datetime = datetime.datetime.now()
        timestamp        = current_datetime.strftime("%Y-%m-%d_%H-%M-%S")

        tfm_logfile      = "undefined"
        tfm_logdir       = output_dir+'/logs/tfm'

        os.makedirs(tfm_logdir, exist_ok=True)
        if os.path.exists(tfm_logdir):
            tfm_logfile = os.path.join(tfm_logdir, f"python_tfm_launch_fe_{timestamp}.log")
        else:
            print("Failed to create the log file directory %s." % tfm_logdir)

        TRACE.TRACE(7,"END tfm_logfile=%s"%tfm_logfile)
        return tfm_logfile

#------------------------------------------------------------------------------
# define needed env variables
#------------------------------------------------------------------------------
    def __init__(self):
        TRACE.TRACE(7,"0010: START")
#        frontends.tfm_frontend.m_frontend.FrontendBase.__init__(self, "tfm_launch_fe")
        midas.frontend.FrontendBase.__init__(self, "tfm_launch")
        TRACE.TRACE(7,"0011: FrontendBase initialized")
#------------------------------------------------------------------------------
# determine active configuration
#------------------------------------------------------------------------------
        self._stop_run               = False;
        self.output_dir              = self.client.odb_get("/Mu2e/OutputDir")
        self.config_name             = self.client.odb_get("/Mu2e/ActiveRunConfiguration")
        self.artdaq_partition_number = self.client.odb_get("/Mu2e/ARTDAQ_PARTITION_NUMBER")

        TRACE.TRACE(7,f":0014: artdaq_partition_number={self.artdaq_partition_number}")

        config_path                  = "/Mu2e/RunConfigurations/"+self.config_name;
        self.use_runinfo_db          = self.client.odb_get(config_path+'/UseRunInfoDB')
        self.tfm_rpc_host            = self.client.odb_get(config_path+'/DAQ/TfmRpcHost'  )

        config_dir                   = os.path.join(self.client.odb_get("/Mu2e/ArtdaqConfigDir"), self.config_name)

        TRACE.TRACE(7,f":0014:config_dir={config_dir} use_runinfo_db={self.use_runinfo_db} rpc_host={self.tfm_rpc_host}")

        os.environ["TFM_SETUP_FHICLCPP"] = f"{config_dir}/.setup_fhiclcpp"

        self.tfm_logfile = self.get_logfile(self.output_dir)
        os.environ["TFM_LOGFILE"       ] = self.tfm_logfile
#------------------------------------------------------------------------------
# redefine STDOUT
#------------------------------------------------------------------------------
        sys.stdout  = open(self.tfm_logfile, 'w')
        TRACE.TRACE(7,"0016: after get_logfile")

#------------------------------------------------------------------------------
# You can add equipment at any time before you call `run()`, but doing
# it in __init__() seems logical.
#-------v----------------------------------------------------------------------
        self.add_equipment(TfmEquipment(self.client))
        TRACE.TRACE(7,"003: equipment added , config_dir=%s"%(config_dir))

        self._fm   = farm_manager.FarmManager(config_dir=config_dir,rpc_host=self.tfm_rpc_host)
        TRACE.TRACE(7,"004: tfm instantiated, self.use_runinfo_db=%i"%(self.use_runinfo_db))
#------------------------------------------------------------------------------
# runinfo DB related stuff
#-------v----------------------------------------------------------------------
        self.runinfo_db   = None;
        if (self.use_runinfo_db):
            self.runinfo_db   = RuninfoDB(self.client);
#------------------------------------------------------------------------------
# try to change priority by re-registering the same callback
#------------------------------------------------------------------------------
        # self.client.register_transition_callback(midas.TR_START, 502, self._tr_start_callback)
#------------------------------------------------------------------------------
# start screen process tailing the logfile
#-------v----------------------------------------------------------------------
        cmd = "/usr/bin/screen -dmS tfm_%i /usr/bin/bash -c \"tail -f %s\"" % (
            self.artdaq_partition_number,self.tfm_logfile) ;
        p   = subprocess.Popen(cmd,shell=True)

        self._fm.do_boot()
        TRACE.TRACE(7,"005: boot done")
        return;

#------------------------------------------------------------------------------
# on exit, also kill the tail logfile process
#------------------------------------------------------------------------------
    def __del__(self):
        TRACE.TRACE(7,"001: destructor START",TRACE_NAME)
        self._fm.__del__();

        cmd = f"x=`ps -efl | grep \"tail -f {self.tfm_logfile}\" | " + "awk '{print $4}' | grep -v grep`;"
        cmd += " if [ -z \"$x\" ] ; then xargs kill -1 $x ; fi"
        TRACE.TRACE(7,f"002: executing {cmd}",TRACE_NAME)
        p   = subprocess.Popen(cmd,shell=True)

        TRACE.TRACE(7,"002: destructor END",TRACE_NAME)

#------------------------------------------------------------------------------
# This function will be called at the beginning of the run.
# You don't have to define it, but you probably should.
# You can access individual equipment classes through the `self.equipment`
# dict if needed.
#------------------------------------------------------------------------------
    def begin_of_run(self, run_number):

        self.client.msg("Frontend has seen start of run number %d" % run_number)

        if (self.use_runinfo_db):
            try:
                db = runinfo_db("aaa");
                rc = db.register_transition(run_number,runinfo.START,0);
            except:
                TRACE.ERROR("failed to register beginning of the START transition")

        self.set_all_equipment_status("Run starting", "yellow")

        self._fm.do_config(run_number=run_number)
        self._fm.do_start_running()
        TRACE.TRACE(7,"001:BEGIN_OF_RUN")

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

        self.set_all_equipment_status("Running", "greenLight")

        return midas.status_codes["SUCCESS"]

#------------------------------------------------------------------------------
#
#---v--------------------------------------------------------------------------
    def end_of_run(self, run_number):

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
        TRACE.TRACE(7,"001:END_RUN")

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

        return midas.status_codes["SUCCESS"]

#------------------------------------------------------------------------------
#        Most people won't need to define this function, but you can use
#        it for final cleanup if needed.
# need to break the loop of TfmLaunchFrontend::run
#---v--------------------------------------------------------------------------
    def frontend_exit(self):
        # breakpoint()
        TRACE.TRACE(7,"001:START : set self._stop_run = True")
        self._stop_run = True;
        self._fm.shutdown();
        TRACE.TRACE(7,"002: DONE")

        
    def should_stop_run(self):
        return self._stop_run;


if __name__ == "__main__":
#------------------------------------------------------------------------------
# The main executable is very simple:
# just create the frontend object, and call run() on it.
#---v--------------------------------------------------------------------------
    TRACE.Instance = "tfm_launch".encode();

    TRACE.TRACE(7,"000: TRACE.Instance : %s"%TRACE.Instance)
    with TfmLaunchFrontend() as fe:
        # breakpoint()
        TRACE.TRACE(7,"001: in the loop",TRACE_NAME)
        fe.run()
        TRACE.TRACE(7,"002: after frontend::run",TRACE_NAME)
        

    TRACE.TRACE(7,"003: DONE, exiting")

        
