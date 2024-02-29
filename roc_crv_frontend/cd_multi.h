/********************************************************************\

  Name:         multi.h
  Created by:   Stefan Ritt

  Contents:     Multimeter Class Driver Header File

  $Id:$

\********************************************************************/

/* class driver routines */
INT cd_multi(INT cmd, PEQUIPMENT pequipment);
INT cd_multi_read(char *pevent, int);
INT cd_multi_set_register(PEQUIPMENT pequipment, int32_t add, int32_t val);
INT cd_multi_get_register(PEQUIPMENT pequipment, int32_t add, int32_t* val);
