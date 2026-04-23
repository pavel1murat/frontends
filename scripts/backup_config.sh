#!/usr/bin/bash
#------------------------------------------------------------------------------
# backup ODB of the $MU2E_DAQ_DIR/config area
#------------------------------------------------------------------------------
if [ ."$MU2E_DAQ_DIR" == "." ] ; then
    echo '$MU2E_DAQ_DIR is not defined, bail out'
    exit -1;
fi

backup_fn=`date +%Y-%m-%d`-config.tgz

pushd $MU2E_DAQ_DIR > /dev/null

cmd="tar -czf  $backup_fn config" ;
echo ... executing $cmd ; $cmd ;
rc=$?

#                                move backup file to backup directory
if [ $rc == "0" ] ; then
    cmd="mv $backup_fn $HOME/backup/."
    echo ... executing $cmd ; $cmd ;
    rc=$?
fi
# 
popd > /dev/null
