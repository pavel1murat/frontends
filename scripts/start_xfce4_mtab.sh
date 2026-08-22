#!/usr/bin/bash
# to be sourced

xfce4-terminal --disable-server --color-bg="#bbbbbb" \
               --tab --title trk01 --color-bg="#cccccc" -e "ssh -KX mu2e-trk-01" \
               --tab --title trk02 --color-bg="#cccccc" -e "ssh -KX mu2e-trk-02" \
               --tab --title trk03 --color-bg="#cccccc" -e "ssh -KX mu2e-trk-03" \
               --tab --title trk04 --color-bg="#cccccc" -e "ssh -KX mu2e-trk-04" \
               --tab --title trk05 --color-bg="#cccccc" -e "ssh -KX mu2e-trk-05" \
               --tab --title trk06 --color-bg="#cccccc" -e "ssh -KX mu2e-trk-06" \
               --tab --title trk07 --color-bg="#cccccc" -e "ssh -KX mu2e-trk-07" \
               --tab --title trk08 --color-bg="#cccccc" -e "ssh -KX mu2e-trk-08" \
               --tab --title trk09 --color-bg="#cccccc" -e "ssh -KX mu2e-trk-09" \
               --tab --title trk10 --color-bg="#cccccc" -e "ssh -KX mu2e-trk-10" \
               --tab --title trk11 --color-bg="#cccccc" -e "ssh -KX mu2e-trk-11" \
               --tab --title trk12 --color-bg="#cccccc" -e "ssh -KX mu2e-trk-12" \
               --tab --title trk13 --color-bg="#cccccc" -e "ssh -KX mu2e-trk-13" \
               --tab --title trk14 --color-bg="#cccccc" -e "ssh -KX mu2e-trk-14" \
               --tab --title trk15 --color-bg="#cccccc" -e "ssh -KX mu2e-trk-15" \
               --tab --title trk16 --color-bg="#cccccc" -e "ssh -KX mu2e-trk-16" \
               --tab --title trk17 --color-bg="#cccccc" -e "ssh -KX mu2e-trk-17" \
               --tab --title trk18 --color-bg="#cccccc" -e "ssh -KX mu2e-trk-18" &
