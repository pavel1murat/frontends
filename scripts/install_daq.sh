#!/usr/bin/bash
#------------------------------------------------------------------------------
dir_name=test
env_name=v001                                # replace v001 with your own name, i.e. 'v001'

if [ $1 != "" ] ; then dir_name=$1 ; fi
if [ $2 != "" ] ; then env_name=$1 ; fi

mkdir $dir_name
cd $dir_name

unset SPACK_ROOT
git clone https://github.com/spack/spack.git
cd spack; git checkout v0.23.1; cd ..
export SPACK_DISABLE_LOCAL_CONFIG=true
source spack/share/spack/setup-env.sh
spack env create   $env_name
spack env activate $env_name
ln -s spack/var/spack/environments/$env_name
#------------------------------------------------------------------------------
# interfaces to mu2e_pcie_utils
#------------------------------------------------------------------------------
export   BUILD_ROOT_INTERFACE=1
export BUILD_PYTHON_INTERFACE=0           # not yet ready
#------------------------------------------------------------------------------
# repositories
#------------------------------------------------------------------------------
mkdir spack-repos
pushd spack-repos
git clone https://github.com/FNALssi/fnal_art.git
git clone https://github.com/marcmengel/scd_recipes.git
git clone https://github.com/art-daq/artdaq-spack.git
git clone https://github.com/Mu2e/mu2e-spack.git
git clone https://github.com/pavel1murat/spack_packages murat-spack
for d in `ls` ; do echo $d; spack repo add $d ; done
popd
#------------------------------------------------------------------------------
# now fix the recipes:                          
#------------------------------------------------------------------------------
#  artdaq-spack/packages/art-suite (@s132)
#------------------------------------------------------------------------------

#        depends_on("libxml2@2.9")
        depends_on("libxml2")
#        depends_on("root@6.30.06 +http+mlp+root7+spectrum+tmva+tmva-sofie cxxstd=20", when="+root")
        depends_on("root@6.32.06 +http+mlp+root7+spectrum+tmva+tmva-sofie cxxstd=20", when="+root")
#------------------------------------------------------------------------------
# keep using gcc 13.1.0 , problems otherwise
#------------------------------------------------------------------------------
spack add gcc@13.1.0
spack concretize -f
spack install
spack compiler find                # add GCC to the list of compilers
#------------------------------------------------------------------------------
# after that ;
#------------------------------------------------------------------------------
spack add py-psycopg2              # python interface to postgresql
spack add emacs+X # the rest is for spack 1.0 ... %gcc@13.1.0 gui=x11
spack add elog@2024-09-25
# spack add midas@2025-04-25+sqlite  #  +postgresql - link problems with postgres
spack add     midas@2025-05-03+sqlite
spack develop midas@2025-05-03+sqlite
#------------------------------------------------------------------------------
# art packages
#------------------------------------------------------------------------------
spack add     artdaq@develop

spack add     artdaq-core-mu2e@develop
spack develop artdaq-core-mu2e@develop
# 2025-05-04: for now, don't forget merge the fix with the added accessor ...
# will not be needed when merged
pushd $env_name/artdaq-core-mu2e ; git merge origin/pasha/ewtag  ; popd ;

spack add     mu2e-pcie-utils@develop
spack develop mu2e-pcie-utils@develop

spack add     Offline@develop
spack develop Offline@develop

spack add     otsdaq-mu2e-tracker@develop
spack develop otsdaq-mu2e-tracker@develop
pushd $env_name/otsdaq-mu2e-tracker ; git fetch --all; git checkout 2025       ; popd ;

spack add     tfm@main
spack develop tfm@main

spack add     frontends@main
spack develop frontends@main

#  pushd $env_name/otsdaq-mu2e-stm     ; git fetch --all; git checkout stm_develop; popd ;
export SPACK_VIEW=$SPACK_ENV/.spack-env/view
spack concretize -f
spack install
#------------------------------------------------------------------------------
# configuration (this part is incomplete, it includes only the script copy part,
# but not the configuration of mhttpd and ODB
#------------------------------------------------------------------------------
cp $env_name/frontends/scripts/setup_daq.sh .
cp $env_name/frontends/scripts/rootlogon.C  .
cp $env_name/frontends/scripts/.rootrc      .

mkdir -p config/scripts

for s in cleanup_partition      \
         get_next_run_number.py \
         artdaq_xmlrpc.py       \
         start_node_frontend.sh \
         start_rpi_frontend.py  ; do
    cp $env_name/frontends/scripts/$s config/scripts/.
done

mkdir    config/midas
cp $env_name/frontends/scripts/test_025.exptab  config/midas/.

mkdir    config/dtc_gui
node=`hostname -s`
cp $env_name/otsdaq-mu2e-tracker/scripts/${node}_pcie0.C  config/scripts/dtc_gui/.
cp $env_name/otsdaq-mu2e-tracker/scripts/${node}_pcie1.C  config/scripts/dtc_gui/.
