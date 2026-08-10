#!/usr/bin/bash
# to be sourced

xfce4-terminal --disable-server \
               --color-bg="#bbbbbb" \
               --tab --title trk01 --color-bg="#cccccc" -e "ssh -KX mu2e-trk-01 \"cd ~/daquser_002 ; . setup_daq.sh ; bash --login -i\" "  \
               --tab --title trk02 --color-bg="#cccccc" -e "ssh -KX mu2e-trk-02 \"cd ~/daquser_002 ; . setup_daq.sh ; bash --login -i\" "  \
               --tab --title trk03 --color-bg="#cccccc" -e "ssh -KX mu2e-trk-03 \"cd ~/daquser_002 ; . setup_daq.sh ; bash --login -i\" "  \
               --tab --title trk04 --color-bg="#cccccc" -e "ssh -KX mu2e-trk-04 \"cd ~/daquser_002 ; . setup_daq.sh ; bash --login -i\" "  \
               --tab --title trk05 --color-bg="#cccccc" -e "ssh -KX mu2e-trk-05 \"cd ~/daquser_002 ; . setup_daq.sh ; bash --login -i\" "  \
               --tab --title trk06 --color-bg="#cccccc" -e "ssh -KX mu2e-trk-06 \"cd ~/daquser_002 ; . setup_daq.sh ; bash --login -i\" "  \
               --tab --title trk07 --color-bg="#cccccc" -e "ssh -KX mu2e-trk-07 \"cd ~/daquser_002 ; . setup_daq.sh ; bash --login -i\" "  \
               --tab --title trk08 --color-bg="#cccccc" -e "ssh -KX mu2e-trk-08 \"cd ~/daquser_002 ; . setup_daq.sh ; bash --login -i\" "  \
               --tab --title trk09 --color-bg="#cccccc" -e "ssh -KX mu2e-trk-09 \"cd ~/daquser_002 ; . setup_daq.sh ; bash --login -i\" "  \
               --tab --title trk10 --color-bg="#cccccc" -e "ssh -KX mu2e-trk-10 \"cd ~/daquser_002 ; . setup_daq.sh ; bash --login -i\" "  \
               --tab --title trk11 --color-bg="#cccccc" -e "ssh -KX mu2e-trk-11 \"cd ~/daquser_002 ; . setup_daq.sh ; bash --login -i\" "  \
               --tab --title trk12 --color-bg="#cccccc" -e "ssh -KX mu2e-trk-12 \"cd ~/daquser_002 ; . setup_daq.sh ; bash --login -i\" "  \
               --tab --title trk13 --color-bg="#cccccc" -e "ssh -KX mu2e-trk-13 \"cd ~/daquser_002 ; . setup_daq.sh ; bash --login -i\" "  \
               --tab --title trk14 --color-bg="#cccccc" -e "ssh -KX mu2e-trk-14 \"cd ~/daquser_002 ; . setup_daq.sh ; bash --login -i\" "  \
               --tab --title trk15 --color-bg="#cccccc" -e "ssh -KX mu2e-trk-15 \"cd ~/daquser_002 ; . setup_daq.sh ; bash --login -i\" "  \
               --tab --title trk16 --color-bg="#cccccc" -e "ssh -KX mu2e-trk-16 \"cd ~/daquser_002 ; . setup_daq.sh ; bash --login -i\" "  \
               --tab --title trk17 --color-bg="#cccccc" -e "ssh -KX mu2e-trk-17 \"cd ~/daquser_002 ; . setup_daq.sh ; bash --login -i\" "  \
               --tab --title trk18 --color-bg="#cccccc" -e "ssh -KX mu2e-trk-18 \"cd ~/daquser_002 ; . setup_daq.sh ; bash --login -i\" " &
