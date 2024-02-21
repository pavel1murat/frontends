#!/usr/bin/sh
# -----------------------------------------------------------------------------
# this script is also sourced by the ARTDAQ supervisor process 
# so having it sourcing /cvmfs/... before any other setups is critical
# example: source ./setup_ots.sh 8
# it has to be sourced in $MU2E_DAQ_DIR
#------------------------------------------------------------------------------
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"
export MU2E_DAQ_DIR=$SCRIPT_DIR
# otsdir=$SCRIPT_DIR

echo $LINENO"     MU2E_DAQ_DIR=$MU2E_DAQ_DIR"
#------------------------------------------------------------------------------
# moving to a single user space with different partitions
#------------------------------------------------------------------------------
       subsystem=mu2e
artdaq_partition=$1
     status_file=$SCRIPT_DIR/.ots_setup_type.txt
#------------------------------------------------------------------------------
# create a status file caching the partition
#------------------------------------------------------------------------------
echo $LINENO"     artdaq_partition=$artdaq_partition"

if   [ ".$artdaq_partition" == ".for_running" ] ; then
# call from somewhere python, protect from Iris, assume the status file exists 
    artdaq_partition=`cat $status_file`;
elif   [ ".$artdaq_partition" != "." ] ; then
#------------------------------------------------------------------------------
# save partition defined on the command line
#------------------------------------------------------------------------------
    echo $artdaq_partition > $status_file
else 
#------------------------------------------------------------------------------
# artdaq partition is not defined
#------------------------------------------------------------------------------
    if [ -e $status_file ] ; then
#------------------------------------------------------------------------------
# subsystem is not defined on the command line, status file exists
# read the partition number from there 
#------------------------------------------------------------------------------
        artdaq_partition=`cat $status_file`;
    else 
	      echo $LINENO" partition number undefined. Specify it on the command line"
	      return -1
    fi
fi

export ARTDAQ_PARTITION_NUMBER=$artdaq_partition
export LOGNAME=$USER                          # ksu might have messed up LOGNAME

echo $LINENO"     ARTDAQ_PARTITION_NUMBER=$ARTDAQ_PARTITION_NUMBER subsystem=$subsystem"
#------------------------------------------------------------------------------
# 'subsystem' has to be either defined explicitly, or defined previously
# it has the meaning of a 'subsystem'
# if not, bail out
#------------------------------------------------------------------------------
if [ ".$subsystem" == "." ]; then
    echo -e "setup [${LINENO}]  \t --> You are user $USER on $HOSTNAME in directory `pwd`"
    echo -e "setup [${LINENO}]  \t ================================================="
    echo -e "setup [${LINENO}]  \t usage:  source setup_ots.sh <subsystem>"
    echo -e "setup [${LINENO}]  \t ... where  subsystem = (mu2e,sync,stm,stmdbtest,calo,trigger,02,dcs,hwdev,tracker,shift,dqmcalo)"
    echo -e "setup [${LINENO}]  \t ================================================="
    return 1;
fi

repository="notGoodRepository"
# will look like 'mu2etrk_pasha_020', removing useless 'mu2e' in between
export OTS_USER_STUB=${USER}_`echo $SCRIPT_DIR | awk -F / '{print $NF}' | sed 's/ots//'`
export DAQ_USER_STUB=${USER}_`echo $SCRIPT_DIR | awk -F / '{print $NF}' | sed 's/ots//'`

# if   [ $subsystem == "mu2e" ]; then
# #------------------------------------------------------------------------------
# # figure out which ports to use; is the CONSOLE_SUPERVISOR_IP used for anything now ? 
# #------------------------------------------------------------------------------
#     if   [ $ARTDAQ_PARTITION_NUMBER == 2 ] ; then # old style: subsystem=sync why ports are different ?
#         export          OTS_MAIN_PORT=2015
#         export OTS_WIZ_MODE_MAIN_PORT=3015
#         repository="otsdaq_mu2e"
#     elif [ $ARTDAQ_PARTITION_NUMBER == 4 ] ; then # old style: subsystem=calo
#         export          OTS_MAIN_PORT=3025
#         export OTS_WIZ_MODE_MAIN_PORT=3025
#         repository="otsdaq_mu2e_calorimeter"
#     elif [ $ARTDAQ_PARTITION_NUMBER == 5 ] ; then # old style: subsystem=stm
#         export          OTS_MAIN_PORT=3035
#         export OTS_WIZ_MODE_MAIN_PORT=3035
#         repository="otsdaq_mu2e_stm"
#     elif [ $ARTDAQ_PARTITION_NUMBER == 6 ] ; then # old style: subsystem=trigger
#         export          OTS_MAIN_PORT=3045
#         export OTS_WIZ_MODE_MAIN_PORT=3045
#         repository="otsdaq_mu2e_trigger"
#     elif [ $ARTDAQ_PARTITION_NUMBER == 7 ] ; then # old style: subsystem=hwdev
#         export          OTS_MAIN_PORT=3055
#         export OTS_WIZ_MODE_MAIN_PORT=3055
#         repository="otsdaq_mu2e_hwdev"
#     elif [ $ARTDAQ_PARTITION_NUMBER == 8 ] ; then # Pasha
#         export          OTS_MAIN_PORT=3065
#         export OTS_WIZ_MODE_MAIN_PORT=3065
#         export        FEWRITE_RUNFILE=1
#         repository="otsdaq_mu2e_tracker"
#     elif [ $ARTDAQ_PARTITION_NUMBER == 9 ] ; then # old style: subsystem=crv
#         export          OTS_MAIN_PORT=3085
#         export OTS_WIZ_MODE_MAIN_PORT=3085
#         repository="otsdaq_mu2e_crv"
#     elif [ $ARTDAQ_PARTITION_NUMBER == 10 ] ; then # Kamal
#         export          OTS_MAIN_PORT=3085
#         export OTS_WIZ_MODE_MAIN_PORT=3085
#         repository="otsdaq_mu2e_crv"
#     elif [ $ARTDAQ_PARTITION_NUMBER == 14 ] ; then # Antonio, old style: subsystem=dqmtrk
#         export          OTS_MAIN_PORT=3070
#         export OTS_WIZ_MODE_MAIN_PORT=3070
#         ##export      FEWRITE_RUNFILE=1
#         repository="otsdaq_mu2e_tracker"
#     elif [ $ARTDAQ_PARTITION_NUMBER == 15 ] ; then # Sara, mu2edaq09
#         export          OTS_MAIN_PORT=3075
#         export OTS_WIZ_MODE_MAIN_PORT=3075
#         repository="otsdaq_mu2e_tracker"
#     elif [ $ARTDAQ_PARTITION_NUMBER == 16 ] ; then # Gennagiy, mu2edaq09
#         export          OTS_MAIN_PORT=3080
#         export OTS_WIZ_MODE_MAIN_PORT=3080
#         repository="otsdaq_mu2e_tracker"
#     fi
# elif [ $subsystem == "HWDev" ]; then
#     export  CONSOLE_SUPERVISOR_IP=192.168.157.5
#     export          OTS_MAIN_PORT=3055
#     export OTS_WIZ_MODE_MAIN_PORT=3055
#     export       ARTDAQ_PARTITION_NUMBER=7
#     repository="otsdaq_mu2e"
# elif [ $subsystem == "HWDev2" ]; then
#     export  CONSOLE_SUPERVISOR_IP=192.168.157.5
#     export          OTS_MAIN_PORT=3055
#     export OTS_WIZ_MODE_MAIN_PORT=3055
#     export       ARTDAQ_PARTITION_NUMBER=7
#     repository="otsdaq_mu2e"
# else
#     echo -e "Invalid parameter!"
#     return 1;
# fi
# ------------------------------------------------------------------------------
echo -e "setup [${LINENO}]  \t ======================================================"
echo -e "setup [${LINENO}]  \t Initially your products path was PRODUCTS=${PRODUCTS}"

source /cvmfs/mu2e.opensciencegrid.org/setupmu2e-art.sh 

unsetup_all >/dev/null 2>&1

PRODUCTS=/cvmfs/mu2e.opensciencegrid.org/artexternals:/cvmfs/fermilab.opensciencegrid.org/products/artdaq:/mu2e/ups

setup mrb 
#------------------------------------------------------------------------------
# 2022-08-09 P.Murat: assume that pullProducts places 'remote' UPS products into a directory 
# named remoteProducts<scisoft_bundle> 
# also assume that there could be only one directory named 'localProducts_BLAH'
# use long names to avoid conflicts
#------------------------------------------------------------------------------
     local_products_dir=`ls $MU2E_DAQ_DIR | grep localProducts`
    local_products_qual=`echo $local_products_dir | sed s/localProducts//`
    remote_products_dir="remoteProducts"$local_products_qual

echo $LINENO local_product_dir:$local_products_dir  remote_products_dir:$remote_products_dir
unset local_products_qual

source $MU2E_DAQ_DIR/$local_products_dir/setup
source $MU2E_DAQ_DIR/$remote_products_dir/setup

# P.M. make remote_products an environment variable - need in artdaq settings...
export REMOTE_PRODUCTS_DIR=$MU2E_DAQ_DIR/$remote_products_dir
#------------------------------------------------------------------------------
# P.Murat: add setup gdb - it is helpful to have it by default, 
#          the system gdb is useless - too old
#------------------------------------------------------------------------------
setup gdb

while true;do
  mrbsetenv >/dev/null
  test -x $TRACE_BIN/trace_cntl && break || { sleep 1; echo Retrying mrbsetenv; }
done

ulimit -c unlimited

echo -e "setup [${LINENO}]  \t Now your products path is PRODUCTS=${PRODUCTS}"
echo
echo -e "setup [${LINENO}]  \t To use trace, do \"tshow | grep . | tdelta -d 1 -ct 1\" with appropriate grep re to"
echo -e "setup [${LINENO}]  \t filter traces. Piping into the tdelta command to add deltas and convert"
echo -e "setup [${LINENO}]  \t the timestamp."

# MRB should set this itself
# export CETPKG_INSTALL=/home/mu2edaq/sync_demo/ots/products
 
#make the number of build threads dependent on the number of cores on the machine:
export CETPKG_J=$((`cat /proc/cpuinfo|grep processor|tail -1|awk '{print $3}'` + 1))
#------------------------------------------------------------------------------
# all temporary files reside in /scratch/mu2e/otsdaq_$OTS_USER_STUB
# $OTS_USER_STUB includes username, subdetector, and the local directory
# scratch configuration, scratch is 'per-working-area'
#------------------------------------------------------------------------------
export       OTS_OWNER=Mu2e
export      MU2E_OWNER=$subsystem
export DISABLE_DOXYGEN=1            # speed up the code builds

# export ARTDAQ_DATABASE_URI=filesystemdb://$configDir/databases_$subsystem/filesystemdb/test_db
export ARTDAQ_DATABASE_URI=mongodb://un:pw@localhost:27017/teststand_db?authSource=admin
# don't delete. -- Pasha
# -----BEGIN PGP MESSAGE-----
# 
# jA0ECQMC+4/3osEzwwTz0k0BMFl1mqQqANYTPkZ7EZTsOHScQs8yv1X9ar3KsoPY
# CbCH7FsbdK0z+UgEyCpf8gKXhkY4NVEjkJB6uRBCMkds/DDy5QUenAMZnOeoIQ==
# =9H/u
# -----END PGP MESSAGE-----
#------------------------------------------------------------------------------
# 2021-11-30 P.Murat: EPICS configuration
#   the easiest solution is to check out mu2e-dcs locally - this automatically makes 
#   the checked out data unique for a user, and a modified version would 
#   for now, assume that mu2e-dcs is checked out in read-only mode
#   do it during the installation, not every time - the same script is executed by artdaq
#------------------------------------------------------------------------------
# export OTSDAQ_EPICS_DATA=/home/mu2edcs/mu2e-dcs/apps/OTSReader/db

export         OTS_EPICS_PV_DB=$MRB_TOP/mu2e-dcs
export       OTSDAQ_EPICS_DATA=$MRB_TOP/mu2e-dcs/apps/OTSReader/db

export       OTSDAQ_EPICS_DATA=/home/mu2edcs/mu2e-dcs/apps/OTSReader/db
export   EPICS_CA_NAME_SERVERS=mu2e-dcs-01.fnal.gov:5064
export EPICS_CA_AUTO_ADDR_LIST=NO
export      EPICS_CA_ADDR_LIST=''
export          CERT_DATA_PATH=/home/mu2edaq/artdaq-utilities-node-server/certs/authorized_users

# export           USER_WEB_PATH=$otsdir/srcs/$repository/UserWebGUI

               offlineFhiclDir=$OFFLINE_DIR
              triggerEpilogDir=$OFFLINE_DIR # may need to be fixed

export  FHICL_FILE_PATH=$FHICL_FILE_PATH:$USER_DATA:$offlineFhiclDir:$triggerEpilogDir:$dataFilesDir
export MU2E_SEARCH_PATH=$MU2E_SEARCH_PATH:/cvmfs/mu2e.opensciencegrid.org/DataFiles:$MRB_SOURCE

# alias rawEventDump="art -c ${otsdir}/srcs/otsdaq/artdaq-ots/ArtModules/fcl/rawEventDump.fcl"

 alias  mb='pushd $MRB_BUILDDIR; ninja -j$CETPKG_J; popd'
 alias mbb='mrb b --generator ninja'
 alias  mz='mrb z; mrbsetenv; mrb b --generator ninja'
 alias  mt='pushd $MRB_BUILDDIR;ninja -j$CETPKG_J;CTEST_PARALLEL_LEVEL=${CETPKG_J} ninja -j$CETPKG_J test;popd'
 alias mtt='mrb t --generator ninja'
 alias  mi='pushd $MRB_BUILDDIR; ninja -j$CETPKG_J install;popd'
 alias mii='mrb i --generator ninja'

shopt -u progcomp                         #let environment variables be tabbed
#------------------------------------------------------------------------------
# Trace setup and helpful commented lines
#------------------------------------------------------------------------------
export                TRACE_FILE=/tmp/trace_file.$$
export            TRACE_LIMIT_MS=0,50,50
export              TRACE_MSGMAX=0        # Activating TRACE
export OTS_DISABLE_TRACE_DEFAULT=1        # do not setup the trace defaults that ots activates.
#echo Turning on all memory tracing via: tonMg 0-63 
#tonMg 0-63

tonMg  0-4   # enable trace to memory
tonSg  0-7   # enable trace to slow path (i.e. UDP)
toffSg 8-63  # apparently not turned off by default?
#tonSg 8-63   # have everything on

# enable kernel trace to memory buffer:
#+test -f /proc/trace/buffer && { export TRACE_FILE=/proc/trace/buffer; tlvls | grep 'KERNEL 0xffffffff00ffffff' >/dev/null || { tonMg 0-63; toffM 24-31 -nKERNEL; }; }

#tlvls             # to see what is enabled by name
#tonS -N DTC* 0-63 # to enable by name
#tshow | grep DTC  # to see memory printouts by name

# end Trace helpful info
#============================
if [ -d ${TRACE_BIN} ]; then 
  echo -e "setup [${LINENO}]  \t     Setting up TRACE defaults..."
  # echo Turning on all memory tracing via: tonMg 0-63 
  
  # Default setup moved from setup_trace to here so it can be disabled
  #${TRACE_BIN}/trace_cntl lvlmskg 0xfff 0x1ff 0  # <memory> <slow> <trigger>, tonSg 0-8 (debug is 8 := TLVL_{FATAL,ALERT,CRIT,ERROR,WARNING,NOTICE,INFO,LOG,DEBUG}) OBSOLETE as of 8/08/2023
  ${TRACE_BIN}/trace_cntl mode 3   # ton*
  
  # Disable some very verbose trace outputs 
  TRACE_NAMLVLSET="\
CONF:LdStrD_C    0x1ff 0 0
FileDB:RDWRT_C   0x1ff 0 0
CONF:CrtCfD_C    0x1ff 0 0
COFS:DpFle_C     0x1ff 0 0
PRVDR:FileDBIX_C 0x1ff 0 0
JSNU:DocUtils_C  0x1ff 0 0
JSNU:Document_C  0x1ff 0 0
CONF:OpLdStr_C   0x1ff 0 0
PRVDR:FileDB_C   0x1ff 0 0
CONF:OpBase_C    0x1ff 0 0
" $TRACE_BIN/trace_cntl namlvlset
fi
#------------------------------------------------------------------------------
# TFM and friends .. assume this script is sourced in the top DAQ directory
# before the mhttpd server is launched
#------------------------------------------------------------------------------
export TFM_CONFIG_DIR=$MU2E_DAQ_DIR/config
return 0
