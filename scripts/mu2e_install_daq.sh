#!/usr/bin/bash
#------------------------------------------------------------------------------
# call signature: mu2e_install_daq.sh top [partition] [otsdaq_bundle] [qual]
# ---------------
# parameters   :
# --------------
# top      : name of the created OTS working area 
#
# partition: partition number to be used, find it at
#            https://github.com/mu2e/otsdaq_mu2e/blob/develop/doc/otsdaq_mu2e.org#partitions
#
# bundle   : otsdaq bundle to use for Mu2e build, find it at 
#            https://github.com/mu2e/pavel1murat/blob/main/doc/build_instructions.org
#
# example  : mu2e_install_daq.sh sara_002 8 v2_07_00 e28_s126_debug
# 
# make sure no variables defined outside get redefined - make it a function
# and declare all internal variables as local
#------------------------------------------------------------------------------
install_daq() {
    local           top=$PWD/$1        ;
    local     partition=$2             ; if [ ".$2" != "." ] ; then partition=$2 ; fi
    local        bundle=v2_07_00       ; if [ ".$3" != "." ] ; then    bundle=$3 ; fi
    local          qual=e28_s126_debug ; if [ ".$4" != "." ] ; then      qual=$4 ; fi
    local         equal=`echo $qual | awk -F _ '{print $1}'`  #
    local         squal=`echo $qual | awk -F _ '{print $2}'`  # 
    local         oqual=`echo $qual | awk -F _ '{print $3}'`  # optimization

    echo $LINENO : bundle:$bundle  qual:$qual 
#------------------------------------------------------------------------------
# $top may need to be created, it becomes MU2E_DAQ_DIR
#------------------------------------------------------------------------------
    if [ ! -d $top ] ; then mkdir $top ; fi ; cd $top
    export MU2E_DAQ_DIR=$top
#------------------------------------------------------------------------------
#   mu2e-dsc cloned in top directory
#------------------------------------------------------------------------------
# git clone https://cdcvs.fnal.gov/projects/mu2e-dcs 

    if [ ! -d srcs ] ; then mkdir srcs ; fi
    pushd srcs 
#------------------------------------------------------------------------------
# pull code to be used, add more packages if needed
#------------------------------------------------------------------------------
    git clone https://github.com/pavel1murat/artdaq
    cd artdaq; git checkout pasha/update_metrics ; cd .. ;

    git clone https://github.com/pavel1murat/artdaq_core
    cd artdaq_core; git checkout pasha/add_run_number ; cd .. ;

    git clone https://github.com/pavel1murat/artdaq_demo

    cd artdaq_core; git checkout pasha/rootwebgui ; cd .. ;
    git clone https://github.com/art-daq/artdaq_core_demo

    git clone https://github.com/mu2e/artdaq_mu2e
    git clone https://github.com/mu2e/artdaq_core_mu2e

    git clone https://github.com/pavel1murat/tfm
    git clone https://github.com/pavel1murat/frontends
#------------------------------------------------------------------------------
# 2023-08-12 P.M. : today, need mu2e_pcie_utils. That may change later
#------------------------------------------------------------------------------
    git clone https://github.com/mu2e/mu2e_pcie_utils
#------------------------------------------------------------------------------
# 2023-10-30 P.M. : finally, check out Offline , today - from Eric's pull
#------------------------------------------------------------------------------
    git clone https://github.com/eflumerf/Offline.git
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
# copy various back-end scripts to their operational locations
#------------------------------------------------------------------------------
    cp srcs/frontends/scripts/setup_daq.sh   ./setup_daq.sh   ; chmod 444 ./setup_daq.sh
    cp srcs/frontends/scripts/setup_midas.sh ./setup_midas.sh
    cp srcs/frontends/scripts/source_me      ./source_me

    mkdir $MU2E_DAQ_DIR/daq_scripts
    cp srcs/frontends/scripts/start_farm_manager   $MU2E_DAQ_DIR/daq_scripts/.
    cp srcs/frontends/scripts/get_output_file_size $MU2E_DAQ_DIR/daq_scripts/.
#------------------------------------------------------------------------------
# 2024-02-19 PM: perhaps, at this point it makes sense to stop and reassess
# 1. fix Offline/ups/product_deps
# 2. comment out everything in TEveEventDisplay/CmakeLists.txt
# 3. setup MIDAS, defining MIDAS experiment at this point may not be needed 
# 4. start build
#------------------------------------------------------------------------------
    source /cvmfs/mu2e.opensciencegrid.org/setupmu2e-art.sh
    setup mrb 

    export MRB_PROJECT=mu2e
    mrb n -v $bundle -q $equal:$squal:$oqual -f 
    source localProducts_${MRB_PROJECT}_${bundle}_${qual}/setup
    mrb uc

    source ./setup_daq.sh   $partition
    source ./setup_midas.sh xxxx

    mrbsetenv
    mrb b --generator=ninja
}
#------------------------------------------------------------------------------
# just call install_daq and pass all parameters to it
#------------------------------------------------------------------------------
install_daq $*
