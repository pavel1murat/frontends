#!/usr/bin/bash
# to be sourced

xfce4-terminal --disable-server \
               --title PSU-XX \
               --tab --title psu0  -e "ssh -KX mu2e@mu2e-trk-psu0"  \
               --tab --title psu1  -e "ssh -KX mu2e@mu2e-trk-psu1"  \
               --tab --title psu2  -e "ssh -KX mu2e@mu2e-trk-psu2"  \
               --tab --title psu3  -e "ssh -KX mu2e@mu2e-trk-psu3"  \
               --tab --title psu4  -e "ssh -KX mu2e@mu2e-trk-psu4"  \
               --tab --title psu5  -e "ssh -KX mu2e@mu2e-trk-psu5"  \
               --tab --title psu6  -e "ssh -KX mu2e@mu2e-trk-psu6"  \
               --tab --title psu7  -e "ssh -KX mu2e@mu2e-trk-psu7"  \
               --tab --title psu8  -e "ssh -KX mu2e@mu2e-trk-psu8"  \
               --tab --title psu9  -e "ssh -KX mu2e@mu2e-trk-psu9"  \
               --tab --title psu10 -e "ssh -KX mu2e@mu2e-trk-psu10"  \
               --tab --title psu11 -e "ssh -KX mu2e@mu2e-trk-psu11"  \
               --tab --title psu12 -e "ssh -KX mu2e@mu2e-trk-psu12"  \
               --tab --title psu13 -e "ssh -KX mu2e@mu2e-trk-psu13"  \
               --tab --title psu14 -e "ssh -KX mu2e@mu2e-trk-psu14"  \
               --tab --title psu15 -e "ssh -KX mu2e@mu2e-trk-psu15"  \
               --tab --title psu16 -e "ssh -KX mu2e@mu2e-trk-psu16"  \
               --tab --title psu17 -e "ssh -KX mu2e@mu2e-trk-psu17" &
