#!/usr/bin/env python
# examples :
# xmlrpc http://xxx-yyyy.fnal.gov:21000/RPC2 len aaa
# xmlrpc http://xxx-yyyy.fnal.gov:21000/RPC2 rmndr i/12 i/5
# xmlrpc http://xxx-yyyy.fnal.gov:21000/RPC2 modl i/7 i/3
# xmlrpc http://xxx-yyyy.fnal.gov:21000/RPC2 system.listMethods
#------------------------------------------------------------------------------
import subprocess, shutil, datetime
import sys, string, getopt, glob, os, time, re, array
import json
import xmlrpc.client
import inspect

class MonitorHost:
    
    def __init__(self):
        self.host       = 'mu2edaq22-ctrl';
        self.job        = None;
        self.dir        = None;
        self.diag_level = 0;
        
# ---------------------------------------------------------------------
    def Print(self,Name,level,Message):
        if (level > self.diag_level): return 0;
        now = time.strftime('%Y/%m/%d %H:%M:%S',time.localtime(time.time()))
        message = now+' [ TestXmlrpc::'+Name+' ] '+Message
        print(message)

#------------------------------------------------------------------------------
    def parse_parameters(self):
        name = 'parse_parameters'
        
        self.Print(name,2,'Starting')
        self.Print(name,2, '%s' % sys.argv)

        try:
            optlist, args = getopt.getopt(sys.argv[1:], '',
                     ['diag_level=', 'dir=', 'job=' ] )
 
        except getopt.GetoptError:
            self.Print(name,0,'%s' % sys.argv)
            self.Print(name,0,'Errors arguments did not parse')
            return 110

        for key, val in optlist:

            # print('key,val = ',key,val)

            if   (key == '--diag_level'):
                self.diag_level = int(val)
            if   (key == '--dir'):
                self.dir = val
            elif (key == '--job'):
                self.job = val

        self.Print(name,1,'job       = %s' % self.job)
        self.Print(name,1,'verbose   = %s' % self.diag_level)

#        if (self.fProject == None) :
#            self.Print(name,0,'Error: Project not defined - exiting !')
#            sys.exit(1)

        self.Print(name,1,'------------------------------------- Done')
        return 0

#------------------------------------------------------------------------------
    def test1(self):
        a = 0;

#------------------------------------------------------------------------------
    def test2(self):
        a = 0;

#------------------------------------------------------------------------------
# print statistics reported by a given artdaq process
#------------------------------------------------------------------------------
    def test3(self):
        a = 0

#------------------------------------------------------------------------------
# returns: ctime_sec fsize_gb space_used space_avail
#------------------------------------------------------------------------------
    def mon_disk_io(self):
        name = 'mon_disk_io';
        
        res = '{'

        ctime_sec = time.time();
        res += f'"ctime_sec":{ctime_sec}'; # float

        cmd = f'ls -altr {self.dir}/data | grep RootDAQOut | tail -n 1 | awk "{{print $5}}"';
        p = subprocess.Popen(cmd,
                             executable="/bin/bash",
                             shell=True,
                             stderr=subprocess.PIPE,
                             stdout=subprocess.PIPE,
                             encoding="utf-8")
        (out, err) = p.communicate();

        self.Print(name,1,f'output:{out}')
        one_gb = 1024.*1024;
        if (out != ''):
            w = out.split();
            self.Print(name,1,f'line 1:{w}')

            fsize_gb = float(w[4])/one_gb/1024.;  # in MBytes
            res += f', "fsize_gb":{fsize_gb}';
        else:
            res += f', "fsize_gb":0';
#------------------------------------------------------------------------------
# finally, space available in write partition
#-------v----------------------------------------------------------------------
#        cmd = f'df `echo {self.dir} | awk -F / ''{print "/"$2}''`'
        cmd="df `echo %s | awk -F / '{print \"/\"$2}'`"%(self.dir)
        p = subprocess.Popen(cmd,
                             executable="/bin/bash",
                             shell=True,
                             stderr=subprocess.PIPE,
                             stdout=subprocess.PIPE,
                             encoding="utf-8")
        (out, err) = p.communicate();
        self.Print(name,1,f'out_2:{out}')

        space_used  = 0;
        space_avail = 0;
        if (out != ''):
            w = out.splitlines()[1].split();
            self.Print(name,1,f'line 2:{w}')

            space_used  = float(w[2])/one_gb;
            space_avail = float(w[3])/one_gb;

        res += f', "space_used":{space_used}';
        res += f', "space_avail":{space_avail}';
        res += '}';

        print(res);
        return;
    
#------------------------------------------------------------------------------
if __name__ == "__main__":

    x = MonitorHost();
    x.parse_parameters();

    if (x.job == "t1"):
        x.test1()
    elif (x.job == "t2"):
        x.test2()
    elif (x.job == "t3"):
        x.test3()
    elif (x.job == "disk_io"):
        x.mon_disk_io()

