import midas
import midas.frontend
import midas.event
import random
import ctypes
import time
import os, sys
import datetime
sys.path.append(os.environ["TFM_DIR"])
from rc.control.component import Component
import rc.control.farm_manager as farm_manager
import traceback

def set_logfile_loc():
    current_datetime = datetime.datetime.now()
    timestamp = current_datetime.strftime("%Y-%m-%d_%H-%M-%S")
    client2 = midas.client.MidasClient("Mu2e")
    active_config = client2.odb_get("/Mu2e/ActiveRunConfiguration/")
    output_dir = f"/Mu2e/RunConfigurations/{active_config}/OutputDir/"
    log_path = client2.odb_get(output_dir)
    client2.disconnect()
    log_directory = f"{log_path}/logs/"
    os.makedirs(log_directory, exist_ok=True)
    if os.path.exists(log_directory):
      print("Directory created successfully.")
      output_filename = os.path.join(log_directory, f"tfm_launch_{timestamp}.log")
      sys.stdout = open(output_filename, 'w')
    else:
      print("Failed to create the log file directory.") 

set_logfile_loc()

class MyPeriodicEquipment(midas.frontend.EquipmentBase):
    def __init__(self, client):
        # The name of our equipment. This name will be used on the midas status
        # page, and our info will appear in /Equipment/MyPeriodicEquipment in
        # the ODB.
        equip_name = "tfm_launch_fe_python"
        
        # Define the "common" settings of a frontend. These will appear in
        # /Equipment/MyPeriodicEquipment/Common. The values you set here are
        # only used the very first time this frontend/equipment runs; after 
        # that the ODB settings are used.
        default_common = midas.frontend.InitialEquipmentCommon()
        default_common.equip_type = midas.EQ_PERIODIC
        default_common.buffer_name = "SYSTEM"
        default_common.trigger_mask = 0
        default_common.event_id = 1
        default_common.period_ms = 100
        default_common.read_when = midas.RO_RUNNING
        default_common.log_history = 1
        
        # You MUST call midas.frontend.EquipmentBase.__init__ in your equipment's __init__ method!
        midas.frontend.EquipmentBase.__init__(self, client, equip_name, default_common)
        #breakpoint()
        # You can set the status of the equipment (appears in the midas status page)
        self.set_status("Initialized")
        
    def readout_func(self):
        """
        For a periodic equipment, this function will be called periodically
        (every 100ms in this case). It should return either a `midas.event.Event`
        or None (if we shouldn't write an event).
        """
        #breakpoint()
        # In this example, we just make a simple event with one bank.
        event = midas.event.Event()
        
        # Create a bank (called "MYBK") which in this case will store 8 ints.
        # data can be a list, a tuple or a numpy array.
        #data = [1,2,3,4,5,6,7,8]
        #event.create_bank("MYBK", midas.TID_INT, data)
        
        return event

class MyFrontend(midas.frontend.FrontendBase):
    """
    A frontend contains a collection of equipment.
    You can access self.client to access the ODB etc (see `midas.client.MidasClient`).
    """
    def __init__(self):
        config_dir = MyFrontend.get_client_state()
        global active_conf
        active_conf = config_dir
        midas.frontend.FrontendBase.__init__(self, "tfm_launch_python")
        # You can add equipment at any time before you call `run()`, but doing
        # it in __init__() seems logical.
        self.add_equipment(MyPeriodicEquipment(self.client))
        fm = farm_manager.FarmManager(config_dir=config_dir)
        #breakpoint()
        fm.do_boot()

    def get_client_state():
        client2 = midas.client.MidasClient("Mu2e")
        active_config = client2.odb_get("/Mu2e/ActiveRunConfiguration")       
        client2.disconnect()
        #breakpoint()
        tfm_dir = os.environ.get('TFM_DIR', '')
        config_dir = os.path.join(os.environ.get('MU2E_DAQ_DIR', ''), 'config', active_config)
        #print(config_dir)
        return config_dir

    def begin_of_run(self, run_number):
        """
        This function will be called at the beginning of the run.
        You don't have to define it, but you probably should.
        You can access individual equipment classes through the `self.equipment`
        dict if needed.
        """
        self.set_all_equipment_status("Running", "greenLight")
        self.client.msg("Frontend has seen start of run number %d" % run_number)
        #print(active_conf)
        fm = farm_manager.FarmManager(config_dir=active_conf)
        fm.do_config(run_number=run_number)
        fm.do_start_running()
        return midas.status_codes["SUCCESS"]
        
    def end_of_run(self, run_number):
        self.set_all_equipment_status("Finished", "greenLight")
        self.client.msg("Frontend has seen end of run number %d" % run_number)
        return midas.status_codes["SUCCESS"]
    
    def frontend_exit(self):
        """
        Most people won't need to define this function, but you can use
        it for final cleanup if needed.
        """
        print("Goodbye from user code!")
        
if __name__ == "__main__":
    # The main executable is very simple - just create the frontend object,
    # and call run() on it.
    with MyFrontend() as tfm_launch:
        tfm_launch.run()
