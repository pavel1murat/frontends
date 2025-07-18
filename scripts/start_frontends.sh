#!/usr/bin/bash
#------------------------------------------------------------------------------
# minimal configuration to start after the build:
# 1) node_frontend - to be started on every node
#    
#    in the MIDAS /Program config, it should be started by config/scripts/start_node_frontend.sh
#------------------------------------------------------------------------------
node_frontend
#------------------------------------------------------------------------------
# emulated CFO frontend, for now, runs on the local node, will evolve
#------------------------------------------------------------------------------
cfo_emu_frontend
#------------------------------------------------------------------------------
# TFM frontend, normally runs on the same node with mhttpd
#------------------------------------------------------------------------------
tfm_fe.py
#------------------------------------------------------------------------------
# mu2e_config_fe: sends messages to ELOG and handles (dispatches) commands
#                 to be executed by all subsystems
#                 definitely sends messages, but not everything else is fully implemented
#------------------------------------------------------------------------------
mu2e_config_fe.py
#------------------------------------------------------------------------------
# there will be a configuration/health monitoring frontend per subsystem
# handling high-level logic
#------------------------------------------------------------------------------
# trk_cfg_frontend
