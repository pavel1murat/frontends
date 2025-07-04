"""
initial version of the Mu2e RPI frontend 

See `examples/multi_frontend.py` for an example that uses more
features (frontend index, polled equipment, ODB settings etc). 
"""
import os, sys, socket, subprocess, time

import midas
import midas.frontend
import midas.event

from PowerSupplyServerConnection import PowerSupplyServerConnection

class RpiPeriodicEquipment(midas.frontend.EquipmentBase):
    """
    We define an "equipment" for each logically distinct task that this frontend
    performs. For example, you may have one equipment for reading data from a
    device and sending it to a midas buffer, and another equipment that updates
    summary statistics every 10s.
    
    Each equipment class you define should inherit from 
    `midas.frontend.EquipmentBase`, and should define a `readout_func` function.
    If you're creating a "polled" equipment (rather than a periodic one), you
    should also define a `poll_func` function in addition to `readout_func`.
    """
    def __init__(self, client, name):
        # The name of our equipment. This name will be used on the midas status
        # page, and our info will appear in /Equipment/MyPeriodicEquipment in
        # the ODB.
        equip_name = name;
        
        # Define the "common" settings of a frontend. These will appear in
        # /Equipment/MyPeriodicEquipment/Common. The values you set here are
        # only used the very first time this frontend/equipment runs; after 
        # that the ODB settings are used.
        default_common              = midas.frontend.InitialEquipmentCommon()
        default_common.equip_type   = midas.EQ_PERIODIC
        default_common.buffer_name  = "SYSTEM"
        default_common.trigger_mask = 0
        default_common.event_id     = 1
        default_common.period_ms    = 20000
        default_common.read_when    = midas.RO_ALWAYS; ## midas.RO_RUNNING
        default_common.log_history  = 1
        
        # You MUST call midas.frontend.EquipmentBase.__init__ in your equipment's __init__ method!
        midas.frontend.EquipmentBase.__init__(self, client, equip_name, default_common)
        
        # You can set the status of the equipment (appears in the midas status page)
        self.set_status("Initialized")


    def readout_func(self):
        """
        For a periodic equipment, this function will be called periodically
        (every 20s = 20000ms in this case). 
        It should return either a `midas.event.Event` or None if we shouldn't write an event
        """
        # self.client.msg("readout_func called")
        lv_data = []
        supply = PowerSupplyServerConnection('localhost', 12000)
        for channel in range(6):
            supply.EnableLowVoltage(channel)
            time.sleep(1)
            readback = supply.QueryPowerVoltage(channel)
            # print(f'channel:{channel} readback:{readback}')
            lv_data.append(readback);
#------------------------------------------------------------------------------
# In this example, we just make a simple event with one bank.
# Create a bank (called "LV00") which in this case will store 4 floats
# data can be a list, a tuple or a numpy array.
# teh abnk name should be 4 chars long
# If performance is a strong factor (and you have large bank sizes), 
# you should use a numpy array instead of raw python lists. In
# that case you would have `data = numpy.ndarray(8, numpy.int32)`
# and then fill the ndarray as desired. The correct numpy data type
# for each midas TID_xxx type is shown in the `midas.tid_np_formats` dict.
#------------------------------------------------------------------------------
        event = midas.event.Event()
        event.create_bank("LV00", midas.TID_FLOAT, lv_data)

        return event # None #  event

#------------------------------------------------------------------------------
# RPI assigned to a certain station
#------------------------------------------------------------------------------
class RpiFrontend(midas.frontend.FrontendBase):
    """
    A frontend contains a collection of equipment.
    You can access self.client to access the ODB etc (see `midas.client.MidasClient`).
    """
    def __init__(self):
#------------------------------------------------------------------------------
# You must call __init__ from the base class. So far, assume only one frontend per PI,
# so name it after the PI
#------------------------------------------------------------------------------
        self.name = socket.gethostname();
        
        midas.frontend.FrontendBase.__init__(self, self.name)
        
        # You can add equipment at any time before you call `run()`, but doing
        # it in __init__() seems logical.
        self.add_equipment(RpiPeriodicEquipment(self.client,self.name))

        self.cmd_top_path = '/Mu2e/Commands/Tracker/RPI/'+self.name;
        self.client.odb_watch(self.cmd_top_path+"/Run", self.process_command)

    def begin_of_run(self, run_number):
        """
        This function will be called at the beginning of the run.
        You don't have to define it, but you probably should.
        You can access individual equipment classes through the `self.equipment`
        dict if needed.
        """
        self.set_all_equipment_status("Running", "greenLight")
        # self.client.msg("Frontend has seen start of run number %d" % run_number)
        return midas.status_codes["SUCCESS"]
        
    def end_of_run(self, run_number):
        self.set_all_equipment_status("Finished", "greenLight")
        # self.client.msg("Frontend has seen end of run number %d" % run_number)
        return midas.status_codes["SUCCESS"]
    
    def frontend_exit(self):
        """
        Most people won't need to define this function, but you can use
        it for final cleanup if needed.
        """
        print("Goodbye from user code!")


#---v--------------------------------------------------------------------------
# Process command: execution gets here when /Mu2e/Commands/Tracker/RPI/$piname/Run is set to 1
#------------------------------------------------------------------------------
    def cmd_reset_station_lv(self,print_level):
        cmd = 'pinctrl set 25 op dl; pinctrl set 25 op dh';
        p = subprocess.Popen(cmd,executable="/bin/bash",shell=True,stderr=subprocess.PIPE,stdout=subprocess.PIPE,encoding="utf-8")
        (out, err) = p.communicate();
        if (err != ''):
            self.client.msg(f'ERROR:reset_station_lv:{err}')

        if (print_level != 0):
            self.client.msg(f'executed:{cmd}')

        return;

#---v--------------------------------------------------------------------------
# Process command: execution gets here when /Mu2e/Commands/Tracker/RPI/$piname/Run is set to 1
#------------------------------------------------------------------------------
    def process_command(self, client, path, new_value):
        print(f'path:{path}');
#------------------------------------------------------------------------------
# a sanity check: return of the whole tracker is disabled
#------------------------------------------------------------------------------
        cmd_run      = self.client.odb_get(self.cmd_top_path+'/Run')
        if (cmd_run != 1):
#-------^----------------------------------------------------------------------
# likely, self-resetting the request
#------------------------------------------------------------------------------
            print(f'WARNING: /Mu2e/Commands/Tracker/RPI/{self.name}/Run:{cmd_run}');
            return

        cmd_name     = self.client.odb_get(self.cmd_top_path+'/Name')
        cmd_par_path = self.cmd_top_path+'/'+cmd_name;
        print_level  = self.client.odb_get(cmd_par_path+'/print_level')
        
        if   (cmd_name.upper() == 'RESET_STATION_LV'):
            # self.client.msg(f'executing RESET_STATION_LV: dont forget to uncomment the command');
            self.cmd_reset_station_lv(print_level)
        else:
            print(f'unknown command:{cmd_name}');

        self.client.odb_set(self.cmd_top_path+'/Finished',1);

        
if __name__ == "__main__":
    # The main executable is very simple - just create the frontend object,
    # and call run() on it.
    with RpiFrontend() as rpi_fe:
        rpi_fe.run()
