#!/usr/bin/bash
#------------------------------------------------------------------------------
# call signature: mu2e_install_ots.sh top [partition] [otsdaq_bundle] [qual]
# ---------------
# parameters   :
# --------------
# top      : name of the created OTS working area 
#
# partition: partition number to be used, find it at
#            https://github.com/mu2e/otsdaq_mu2e/blob/develop/doc/otsdaq_mu2e.org#partitions
#
# bundle   : otsdaq bundle to use for Mu2e build, find it at 
#            https://github.com/mu2e/otsdaq_mu2e/blob/develop/doc/build_instructions.org
#
# example  : mu2e_install_ots.sh pasha_007 8
# 
# make sure no variables defined outside get redefined - make it a function
# and declare all internal variables as local
#------------------------------------------------------------------------------
install_ots() {
    local           top=$PWD/$1       ;
    local     partition=$2            ; if [ ".$2" != "." ] ; then partition=$2 ; fi
    local        bundle=v2_07_00      ; if [ ".$3" != "." ] ; then    bundle=$3 ; fi
    local          qual=e28_s126_prof ; if [ ".$4" != "." ] ; then      qual=$4 ; fi
    local         equal=`echo $qual | awk -F _ '{print $1}'`  #
    local         squal=`echo $qual | awk -F _ '{print $2}'`  # 
    local         oqual=`echo $qual | awk -F _ '{print $3}'`  # optimization

    echo $LINENO : bundle:$bundle  qual:$qual 
#------------------------------------------------------------------------------
# $top may need to be created
#------------------------------------------------------------------------------
    if [ ! -d $top ] ; then mkdir $top ; fi ; cd $top
#------------------------------------------------------------------------------
#                   mu2e-dsc cloned in top directory
#------------------------------------------------------------------------------
# git clone https://cdcvs.fnal.gov/projects/mu2e-dcs 

    if [ ! -d srcs ] ; then mkdir srcs ; fi
    pushd srcs 
#------------------------------------------------------------------------------
# pull code to be used, add more packages if needed
#------------------------------------------------------------------------------
#    git clone https://github.com/mu2e/otsdaq_mu2e
    git clone https://github.com/mu2e/otsdaq_mu2e_dqm
    git clone https://github.com/mu2e/otsdaq_mu2e_tracker
#    git clone https://github.com/mu2e/otsdaq_mu2e_trigger
#------------------------------------------------------------------------------
# otsdaq_mu2e_config is a private Mu2e repo
#------------------------------------------------------------------------------
#    git clone https://github.com/mu2e/otsdaq_mu2e_config
#------------------------------------------------------------------------------
# 2023-08-12 P.M. : today, need otsdaq and mu2e_pcie_utils. That may change later
#------------------------------------------------------------------------------
#    git clone https://github.com/art-daq/otsdaq
    git clone https://github.com/mu2e/mu2e_pcie_utils
#------------------------------------------------------------------------------
# 2023-10-30 P.M. : finally, check out Offline , today - from Eric's pull
#------------------------------------------------------------------------------
    git clone https://github.com/eflumerf/Offline.git -b eflumerf/AddCMake
#------------------------------------------------------------------------------
# fixes for v2_06_10
#------------------------------------------------------------------------------
#    if [ $bundle == v2_06_10 ] ; then    
#        git clone https://github.com/mu2e/artdaq_mu2e
#        git clone https://github.com/mu2e/artdaq_core_mu2e
#    fi
    popd
#------------------------------------------------------------------------------
# pull remote products, help : ./pullProducts --help
#------------------------------------------------------------------------------
    wget http://scisoft.fnal.gov/scisoft/bundles/tools/pullProducts
    chmod a+x pullProducts
    mkdir remoteProducts_mu2e_${bundle}_${qual}
    ./pullProducts remoteProducts_mu2e_${bundle}_${qual} slf7 otsdaq-${bundle} ${squal}-${equal} $oqual
    rm *.tar.bz2
#------------------------------------------------------------------------------
# start build
#------------------------------------------------------------------------------
    source /cvmfs/mu2e.opensciencegrid.org/setupmu2e-art.sh
    setup mrb 

    export MRB_PROJECT=mu2e
    mrb n -v $bundle -q $equal:$squal:$oqual -f
    source localProducts_${MRB_PROJECT}_${bundle}_${qual}/setup
    mrb uc

    cp srcs/otsdaq_mu2e_config/setup_ots.sh ./setup_ots.sh ; chmod 444 ./setup_ots.sh
    source ./setup_ots.sh $partition

    mrbsetenv
    mrb b
}
#------------------------------------------------------------------------------
# just call install_ots and pass all parameters to it
#------------------------------------------------------------------------------
install_ots $*
