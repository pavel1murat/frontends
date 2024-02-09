#------------------------------------------------------------------------------
# P.M. an attempt of a Python implementation of the TFM launch frontend
#------------------------------------------------------------------------------
import midas
import midas.frontend
import midas.event
import random
import ctypes

from   rc.control.farm_manager import FarmManager

class TfmLaunchFrontend(midas.frontend.FrontendBase):
    """
    This periodic equipment is very similar to the one in `examples/basic_frontend.py`
    """
    def __init__(self, client):
        # If using the frontend_index, you should encode the index in your
        # equipment name.
        equip_name = "MyMultiPeriodicEquipment_%s" % midas.frontend.frontend_index
        
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
        default_common              = midas.frontend.InitialEquipmentCommon()
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

        _tfm = FarmManager()
        
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
            
class MyMultiPolledEquipment(midas.frontend.EquipmentBase):
    """
    Here we're creating a "polled" equipment rather than a periodic one.
    
    You should define an extra function (`poll_func()`) which will be called 
    very often and should say whether there is a new event ready or not. Only 
    then will `readout_func()` be called.
    """
    def __init__(self, client):
        # The differences from the periodic equipment above are:
        # * a different name
        # * a different equipment type (EQ_POLLED)
        # * different event IDs (can be anything, but make sure they're
        #   unique for each equipment!).
        equip_name = "MyMultiPolledEquipment_%s" % midas.frontend.frontend_index
        
        default_common = midas.frontend.InitialEquipmentCommon()
        default_common.equip_type = midas.EQ_POLLED
        default_common.buffer_name = "SYSTEM"
        default_common.trigger_mask = 0
        default_common.event_id = 600 + midas.frontend.frontend_index
        default_common.period_ms = 100
        default_common.read_when = midas.RO_RUNNING
        default_common.log_history = 0
        
        midas.frontend.EquipmentBase.__init__(self, client, equip_name, default_common)
        
        # Example of registering a callback function on an ODB value.
        # The equipment Settings have pre-defined callbacks (settings_changed_func()
        # and (detailed_settings_changed_func()), but you can register to watch other
        # directories as well if you want.
        # See the odb_watch() documentation for meaning of the 3rd argument (and
        # why the 2 callbacks have different signatures).
        self.client.odb_watch("/Logger/Data dir", self.data_dir_changed)
        self.client.odb_watch("/WebServer/Host list", self.host_list_changed, True)
        
        self.set_status("Initialized")
        
    def data_dir_changed(self, client, path, new_value):
        """
        Example callback for watching whether an ODB value changed.
        """
        print("%s is now %s" % (path, new_value))
        
    def host_list_changed(self, client, path, idx, new_value):
        """
        Example callback for watching whether an ODB value changed.
        """
        print("%s[%d] is now %s" % (path, idx, new_value))
        
    def poll_func(self):
        """
        This function is called very frequently and should return True/False
        depending on whether an event is ready to be read out. It should be
        a quick function. 
        
        In this case we're just choosing random number as we don't have any
        actual device hooked up.
        """
        return random.uniform(1, 100) < 2
        
    def readout_func(self):
        """
        For this polled equipment, readout_func will only be called when 
        poll_func() has returned True (and at transitions if "Common/Read on"
        includes `midas.RO_BOR`, for example).
        """
        event = midas.event.Event()
        data = [4.5, 6.7, 8.9]
        
        event.create_bank("MYFL", midas.TID_FLOAT, data)
        
        return event

class TfmLaunchFrontend(midas.frontend.FrontendBase):
    def __init__(self):
        # If using the frontend_index, you should encode the index in your
        # equipment name.
        fe_name = "multife_%i" % midas.frontend.frontend_index
        midas.frontend.FrontendBase.__init__(self, fe_name)
        
        # add equipment
        self.add_equipment(MyMultiPeriodicEquipment(self.client))
        
        # You can change the order in which our frontend's begin_of_run/
        # end_of_run function are called during transitions.
        self.client.set_transition_sequence(midas.TR_START, 300)
        self.client.set_transition_sequence(midas.TR_STOP , 700)
#------------------------------------------------------------------------------
# neeed to access ODB, get ARTDAQ PARTITION number, and instantiate the farm manager
# source $TFM_DIR/bin/tfm_configure $artdaq_config $partition
# $TFM_DIR/rc/control/farm_manager.py --config-dir=$config_dir
#------------------------------------------------------------------------------
        
    def begin_of_run(self, run_number):
        self.set_all_equipment_status("Running", "greenLight")
        self.client.msg("Frontend has seen start of run number %d" % run_number)
        
    def end_of_run(self, run_number):
        self.set_all_equipment_status("Okay", "greenLight")
        self.client.msg("Frontend has seen end of run number %d" % run_number)
        
if __name__ == "__main__":
    # We must call this function to parse the "-i" flag, so it is available
    # as `midas.frontend.frontend_index` when we init the frontend object.
    midas.frontend.parse_args()
    
    # Now we just create and run the frontend like in the basic example.
    with TfmLaunchFrontend() as fe:
        fe.run()
