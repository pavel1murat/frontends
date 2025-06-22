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

class TestXmlrpc:
    
    def __init__(self):
        self.port    = 21000;
        self.test    = 'undefined';
        self.verbose = 0;
        
# ---------------------------------------------------------------------
    def Print(self,Name,level,Message):
        if (level > self.verbose): return 0;
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
                     ['port=', 'test=', 'verbose=' ] )
 
        except getopt.GetoptError:
            self.Print(name,0,'%s' % sys.argv)
            self.Print(name,0,'Errors arguments did not parse')
            return 110

        for key, val in optlist:

            # print('key,val = ',key,val)

            if key == '--port':
                self.port = val
            if key == '--test':
                self.test = val
            elif key == '--verbose':
                self.verbose = int(val)

        self.Print(name,1,'test      = %s' % self.test)
        self.Print(name,1,'verbose   = %s' % self.verbose)

#        if (self.fProject == None) :
#            self.Print(name,0,'Error: Project not defined - exiting !')
#            sys.exit(1)

        self.Print(name,1,'------------------------------------- Done')
        return 0

#------------------------------------------------------------------------------
    def test1(self):
        s = xmlrpc.client.ServerProxy('http://mu2edaq22-ctrl.fnal.gov:21000')
        # xmlrpc http://mu2edaq22-ctrl.fnal.gov:21000/RPC2 get_state daqint 
        print(s.get_state("daqint"))
        print(s.state())
        # s.alarm("emoe")
        a = 0;
        print(s.get_messages(a))

#------------------------------------------------------------------------------
# print statistics reported by a given artdaq process
#------------------------------------------------------------------------------
    def test2(self):
        url = f'http://mu2edaq22-ctrl.fnal.gov:{self.port}'
        s = xmlrpc.client.ServerProxy(url)
        print(s.daq.report("stats"))

#------------------------------------------------------------------------------
if __name__ == "__main__":

    x = TestXmlrpc();
    x.parse_parameters();

    if (x.test == "t1"):
        x.test1()
    elif (x.test == "t2"):
        x.test2()
