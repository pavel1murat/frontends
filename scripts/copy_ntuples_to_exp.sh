#!/usr/bin/bash 
#------------------------------------------------------------------------------
# file: otsdaq-mu2e-tracker/scripts/copy_ntuples_to_exp.sh
# copies data from local /scratch/mu2e/.. area on an online machine 
# to offline /exp/mu2e/data/projects/...
# so far , used only for the tracker
#
# call format: copy_ntuples_to_exp.sh dsid run1 run2 [doit] 
#
# if run2>run1 is defined, the range of runs is copied
# if "doit" is undefined, the script only prints the commands to be executed
#------------------------------------------------------------------------------
dsid=$1
rn1=`printf "%06i" $2`
if [ ".$3" != "." ] ; then rn2=$3 ; else rn2=$rn1 ; fi

doit=$4

echo dsid=$dsid rn1=$rn1 rn2=$rn2 doit=$doit
name_stub=${USER}_`echo $MU2E_DAQ_DIR | awk -F / '{print $NF}'`

for rn in `seq $rn1 $rn2` ; do
    irn=`printf "%06i" $rn`
    for f in `ls /data/tracker/vst/$dsid/nts.mu2e.trk.$dsid.* | grep $rn` ; do
        bn=`basename $f`
        dsconf=`echo $bn | awk -F . '{print $4}'`
        cmd="scp $f murat@mu2egpvm06:/exp/mu2e/data/projects/tracker/vst/$dsid/."
        echo "$cmd"
        if [ ".$doit" != "." ] ; then 
            # echo doit=$doit
            $cmd ; echo rc=$? ; 
        fi
    done
done
