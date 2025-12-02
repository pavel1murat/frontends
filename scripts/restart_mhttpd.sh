#!/usr/bin/bash
# to be sourced, not executed
# kill
p=`ps -efl | grep mhttpd | grep $MIDAS_EXPT_NAME | grep -v grep | awk '{print $4}'`
if [ ."$p" != . ] ; then kill -9 $p ; fi
# restart as a daemon
mhttpd -D -e $MIDAS_EXPT_NAME
