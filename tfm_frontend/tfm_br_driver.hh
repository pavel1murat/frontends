///////////////////////////////////////////////////////////////////////////////
// P.Murat: cloned from nulldev.h by Stefan Ritt
// ROC slow control driver
///////////////////////////////////////////////////////////////////////////////
/* Store any parameters the device driver needs in following 
   structure. Edit the DRIVER_SETTINGS_STR accordingly. This 
   contains usually the address of the device. For a CAMAC device
   this could be crate and station for example. */

typedef struct {
  int link;
  int active;
} TFM_BR_DRIVER_SETTINGS;


/* following structure contains private variables to the device
   driver. It is necessary to store it here in case the device
   driver is used for more than one device in one frontend. If it
   would be stored in a global variable, one device could over-
   write the other device's variables. */

typedef struct {
  TFM_BR_DRIVER_SETTINGS  driver_settings;
  float            *array;
  INT              num_channels;
  INT              (*bd) (INT cmd, ...);    /* bus driver entry function   */
  void             *bd_info;                /* private info of bus driver  */
  HNDLE            hkey;                    /* ODB key for bus driver info */
} TFM_BR_DRIVER_INFO;

INT tfm_br_driver(INT cmd, ...);
