#!/usr/bin/bash 
#------------------------------------------------------------------------------
# file: otsdaq-mu2e-tracker/scripts/copy_data_to_exp.sh
# copies data from local /scratch/mu2e/.. area on an online machine 
# to offline /exp/mu2e/data/projects/...
# so far , used only for the tracker
#
# call format: copy_data_to_exp.sh run1 run2 [doit] 
#
# if run2>run1 is defined, the range of runs is copied
# if "doit" is undefined, the script only prints the commands to be executed
#------------------------------------------------------------------------------
rn1=`printf "%06i" $1`
if [ ".$2" != "." ] ; then rn2=$2 ; else rn2=$rn1 ; fi

doit=$3

echo rn1=$rn1 rn2=$rn2 doit=$doit
name_stub=${USER}_`echo $MU2E_DAQ_DIR | awk -F / '{print $NF}'`

for rn in `seq $rn1 $rn2` ; do
    irn=`printf "%06i" $rn`
    for f in `ls /data/tracker/vst/$name_stub/data/raw.mu2e.trk.vst.*.art | grep $rn` ; do
        bn=`basename $f`
        dsconf=`echo $bn | awk -F . '{print $4}'`
        dsid=raw.mu2e.trk.vst.art
        cmd="scp $f murat@mu2egpvm06:/exp/mu2e/data/projects/vst/datasets/$dsid/."
        echo "$cmd"
        if [ ".$doit" != "." ] ; then 
            # echo doit=$doit
            $cmd ; echo rc=$? ; 
        fi
    done
done
