#!/user/bin/env python
#------------------------------------------------------------------------------
# Namitha: TFM launch frontend - python implementation
# frontend name : python_tfm_launch_fe
#------------------------------------------------------------------------------
import  ctypes, os, sys, datetime, random, time, traceback, subprocess
import  TRACE

sys.path.append(os.environ["MIDASSYS"]+'/python')
import  midas
import  midas.frontend
import  midas.event

sys.path.append(os.environ["TFM_DIR"])
# from   rc.control.component import Component
# import rc.control.farm_manager as farm_manager
import rc.control.farm_manager as farm_manager

#------------------------------------------------------------------------------
# TFM 'equipment' is just a placeholder
# 'client' is the Midas frontend client which connects to ODB
#------------------------------------------------------------------------------
class TfmEquipment(midas.frontend.EquipmentBase):
    def __init__(self, client):
        # The name of our equipment. This name will be used on the midas status
        # page, and our info will appear in /Equipment/MyPeriodicEquipment the ODB.
        TRACE.TRACE(7,":001:START")
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

        equip_name            = "tfm_launch_fe"
        midas.frontend.EquipmentBase.__init__(self, client, equip_name, settings)
#------------------------------------------------------------------------------
# set the status of the equipment (appears in the midas status page)
#------------------------------------------------------------------------------
        self.set_status("Initialized")
        TRACE.TRACE(7,":002:END equipment initialized")
        return;

#-------^----------------------------------------------------------------------
# For a periodic equipment, this function will be called periodically
# (every 10 s in this case - see period_ms above). It should return either a `midas.event.Event`
# or None (if we shouldn't write an event).
#------------------------------------------------------------------------------
    def readout_func(self):
        TRACE.TRACE(7,":001")
        # In this example, we just make a simple event with one bank.

        # event = midas.event.Event()

        # Create a bank (called "MYBK") which in this case will store 8 ints.
        # data can be a list, a tuple or a numpy array.
        # data = [1,2,3,4,5,6,7,8]
        # event.create_bank("MYBK", midas.TID_INT, data)

        TRACE.TRACE(7,":002:exit")
        return None;        # event

#------------------------------------------------------------------------------
# FE name : 'python_tfm_launch_fe', to distinguish from the C++ frontend
#    A frontend contains a collection of equipment.
#    You can access self.client to access the ODB etc (see `midas.client.MidasClient`).
#------------------------------------------------------------------------------
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
            sys.stdout  = open(tfm_logfile, 'w')
        else:
            print("Failed to create the log file directory %s." % tfm_logdir)

        TRACE.TRACE(7,"END tfm_logfile=%s"%tfm_logfile)
        return tfm_logfile

#------------------------------------------------------------------------------
# define needed env variables
#------------------------------------------------------------------------------
    def __init__(self):
        TRACE.TRACE(7,"0010: START")
        midas.frontend.FrontendBase.__init__(self, "python_tfm_launch_fe")
        TRACE.TRACE(7,"0011: FrontendBase initialized")
#------------------------------------------------------------------------------
# determine active configuration
#------------------------------------------------------------------------------
        _config_name                 = self.client.odb_get("/Mu2e/ActiveRunConfiguration")
        self.artdaq_partition_number = self.client.odb_get("/Mu2e/ARTDAQ_PARTITION_NUMBER")
        TRACE.TRACE(7,f":0014: artdaq_partition_number={self.artdaq_partition_number}")
        config_path          = "/Mu2e/RunConfigurations/"+_config_name;
        _use_runinfo_db      = self.client.odb_get(config_path+'/UseRuninfoDB')
        _output_dir          = self.client.odb_get("/Mu2e/OutputDir")

        config_dir           = os.path.join(os.environ.get('MU2E_DAQ_DIR', ''), 'config', _config_name)
        TRACE.TRACE(7,":0014:config_dir=%s use_runinfo_db=%i" % (config_dir,_use_runinfo_db))

        os.environ["TFM_SETUP_FHICLCPP"] = f"{config_dir}/.setup_fhiclcpp"

        self.tfm_logfile = self.get_logfile(_output_dir)
        os.environ["TFM_LOGFILE"       ] = self.tfm_logfile
#------------------------------------------------------------------------------
# strip '/dev/' from tty 
# TODO : TFM_TTY is not needed, remove....
#------------------------------------------------------------------------------
        TRACE.TRACE(7,"0016: after get_logfile")
        try:
            fd  = open("/dev/stdout");
            os.environ["TFM_TTY"           ] = os.ttyname(fd.fileno())[5:];
        except:
            os.environ["TFM_TTY"           ] = "undefined"

#        TRACE.TRACE(7,"0017: after open /dev/stdout TFM_TTY=%s"%(os.environ["TFM_TTY"]))
#------------------------------------------------------------------------------
# You can add equipment at any time before you call `run()`, but doing
# it in __init__() seems logical.
#-------v----------------------------------------------------------------------
        self.add_equipment(TfmEquipment(self.client))
        TRACE.TRACE(7,"003: equipment added")

        self.fm   = farm_manager.FarmManager(config_dir=config_dir)
        TRACE.TRACE(7,"004: tfm instantiated")

#------------------------------------------------------------------------------
# strt screen process tailing the logfile
#-------v----------------------------------------------------------------------
        cmd = "/usr/bin/screen -dmS tfm_%i /usr/bin/bash -c \"tail -f %s\"" % (
            self.artdaq_partition_number,self.tfm_logfile) ;
        p   = subprocess.Popen(cmd,shell=True)

        self.fm.do_boot()
        TRACE.TRACE(7,"005: boot done")
        return;

#------------------------------------------------------------------------------
# This function will be called at the beginning of the run.
# You don't have to define it, but you probably should.
# You can access individual equipment classes through the `self.equipment`
# dict if needed.
#------------------------------------------------------------------------------
    def begin_of_run(self, run_number):

        self.set_all_equipment_status("Running", "greenLight")
        self.client.msg("Frontend has seen start of run number %d" % run_number)
        self.fm.do_config(run_number=run_number)
        self.fm.do_start_running()
        TRACE.TRACE(7,"001:BEGIN_OF_RUN")

        return midas.status_codes["SUCCESS"]

#------------------------------------------------------------------------------
#
#---v--------------------------------------------------------------------------
    def end_of_run(self, run_number):
        self.fm.do_stop_running()
        self.set_all_equipment_status("Finished", "greenLight")
        self.client.msg("Frontend has seen end of run number %d" % run_number)
        TRACE.TRACE(7,"001:END_OF_RUN")
        return midas.status_codes["SUCCESS"]

#------------------------------------------------------------------------------
#        Most people won't need to define this function, but you can use
#        it for final cleanup if needed.
#---v--------------------------------------------------------------------------
    def frontend_exit(self):
        print("Goodbye from user code!")
        TRACE.TRACE(7,"001:END")



if __name__ == "__main__":
#------------------------------------------------------------------------------
# The main executable is very simple:
# just create the frontend object, and call run() on it.
#---v--------------------------------------------------------------------------
    TRACE.Instance = sys.argv[0].encode()
    with TfmLaunchFrontend() as tfm_launch:
        # breakpoint()
        tfm_launch.run()
