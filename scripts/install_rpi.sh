#!/usr/bin/bash
#------------------------------------------------------------------------------
# initial setup of RPI:
# 1) create a ~mu2e/daq subdirectory
# 2) copy frontend itself, a setup script and a script starting the frontend
# 3) copy a midas RPI tarball to be untarred in /opt:
#    cd /opt ; tar -xzf ~/daq/midas_rpi.tgz
#
# the scripts and the tarball may need to be updated more often than the tarball
# don't rush with the implementation, wait a bit
#------------------------------------------------------------------------------

nodes="mu2e-trk-psu0 mu2e-trk-psu1 mu2e-trk-psu2 mu2e-trk-psu3  mu2e-trk-psu4  mu2e-trk-psu5"
nodes=$nodes"  mu2e-trk-psu6  mu2e-trk-psu7  mu2e-trk-psu8  mu2e-trk-psu9  mu2e-trk-psu10  mu2e-trk-psu11"
nodes=$nodes"  mu2e-trk-psu12  mu2e-trk-psu13  mu2e-trk-psu14  mu2e-trk-psu15  mu2e-trk-psu16  mu2e-trk-psu17"

for node in $nodes ; do
    # echo $node
#    cmd='if [ ! -d daq ] ; then mkdir daq ; fi'
#    ssh -KX mu2e@$node $cmd
#    for f in start_frontend.sh setup_midas.sh rpi_frontend.py ; do
#        scp v001/frontends/rpi/$f  mu2e@$node:~/daq/.
#    done
    scp  midas_rpi.tgz  mu2e@$node:~/daq/.
done
