#!/usr/bin/bash
#------------------------------------------------------------------------------
# backup ODB of MIDAS_EXPT_NAME experiment into a .json file 
#------------------------------------------------------------------------------
if [ ."$MU2E_DAQ_DIR" == "." ] ; then
    echo '$MU2E_DAQ_DIR is not defined, bail out'
    exit -1;
fi
#------------------------------------------------------------------------------
# looks that the DAQ env is setup, continue
#------------------------------------------------------------------------------
echo "env(MU2E_DAQ_DIR)":$MU2E_DAQ_DIR
pushd $MU2E_DAQ_DIR > /dev/null
#
backup_fn=`date +"%Y-%m-%d"`-`hostname -s`-${MIDAS_EXPT_NAME}-backup.json
echo $backup_fn
# 
tmp_fn=/tmp/$$_odb_backup.cmd  # ; echo tmp_fn:$tmp_fn
cat  <<EOF > $tmp_fn
save $backup_fn
EOF
# 
# cat $tmp_fn
# which odbedit
#                                save ODB
cmd="odbedit -c @$tmp_fn"
echo ... executing $cmd ; $cmd
#                                move backup file to backup directory
if [ $? == "0" ] ; then
    cmd="mv $backup_fn $HOME/backup/."
    echo ... executing $cmd ; $cmd
fi
#                                cleanup
if [ $? == "0" ] ; then
    rm $tmp_fn
fi

# 
popd > /dev/null
