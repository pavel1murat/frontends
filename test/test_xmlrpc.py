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
        self.host    = 'mu2edaq22-ctrl';
        self.port    = '21101';
        self.test    = 'undefined';
        self.url     = None;
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
                     ['diag_level=', 'host=', 'port=', 'test=' ] )
 
        except getopt.GetoptError:
            self.Print(name,0,'%s' % sys.argv)
            self.Print(name,0,'Errors arguments did not parse')
            return 110

        for key, val in optlist:

            # print('key,val = ',key,val)

            if   (key == '--diag_level'):
                self.verbose = int(val)
            elif (key == '--host'):
                self.host = val
            elif (key == '--port'):
                self.port = val
            elif (key == '--test'):
                self.test = val
            elif (key == '--url'):
                self.url = val

        if (self.url == None):
            self.url = 'http://'+self.host+'.fnal.gov:'+self.port;

        self.Print(name,1,'test      = %s' % self.test)
        self.Print(name,1,'verbose   = %s' % self.verbose)

#        if (self.fProject == None) :
#            self.Print(name,0,'Error: Project not defined - exiting !')
#            sys.exit(1)

        self.Print(name,1,'------------------------------------- Done')
        return 0

#------------------------------------------------------------------------------
    def test1(self):
        s = xmlrpc.client.ServerProxy(self.url)
        # xmlrpc http://mu2edaq22-ctrl.fnal.gov:21000/RPC2 get_state daqint 
        print(s.get_state("daqint"))
        a = 0;
#       print(s.get_messages(a))

#------------------------------------------------------------------------------
    def test2(self):
        s = xmlrpc.client.ServerProxy(self.url)
        # xmlrpc http://mu2edaq22-ctrl.fnal.gov:21000/RPC2 get_state daqint 
        # print(s.get_state("daqint"))
        print(s.state())
        # s.alarm("emoe")
        a = 0;
        print(s.get_messages(a))

#------------------------------------------------------------------------------
# print statistics reported by a given artdaq process
#------------------------------------------------------------------------------
    def test3(self):
        s   = xmlrpc.client.ServerProxy(self.url)
        res = s.daq.report("stats");
        lines = res.splitlines();
        for l in lines:
            nb = len(l.strip('\n').strip(' '));
            if (nb != 0):
                print(f'nb:{nb:4d}, l:{l}.');

#------------------------------------------------------------------------------
# print parsed BR metrics
#------------------------------------------------------------------------------
    def parse_br_metrics(self):
        name   = 'parse_br_metrics';
        s      = xmlrpc.client.ServerProxy(self.url)
        res    = s.daq.report("stats");
        lines  = res.splitlines();

        nf_evt = len(lines)-4;  # N(fragments/event)
#------------------------------------------------------------------------------
# nb: 69, l:br01 run number = 1179, Sent Fragment count = 41829, br01 statistics:.
# nb:231, l:  Fragments read: 600 fragments generated at 9.99923 getNext calls/sec, fragment rate = 9.99923 fragments/sec, monitor window = 60.0046 sec, min::max read size = 1::1 fragments Average times per fragment:  elapsed time = 0.100008 sec.
# nb:186, l:  Fragment output statistics: 600 fragments sent at 9.99923 fragments/sec, effective data rate = 0.269785 MB/sec, monitor window = 60.0046 sec, min::max event size = 0.0236282::0.031105 MB.
# nb:166, l:  Input wait time = 0.0999967 s/fragment, buffer wait time = 2.56538e-06 s/fragment, request wait time = 0.0999948 s/fragment, output wait time = 2.99493e-06 s/fragment.
# nb: 69, l:fragment_id:0 nfragments:1 nbytes:26312 max_nf:1000 max_nb:1048576000.
#-------v----------------------------------------------------------------------
# line 0: br01 run number = 1179, Sent Fragment count = 14271, br01 statistics:
        w       = lines[0].split();
        
        self.Print(name,1,f'line 0: w:{w}');
        rn      = int(w[4].strip(','))
        nf_sent = int(w[9].strip(','))
        self.Print(name,1,f'rn:{rn} nf_sent:{nf_sent}');

# line 1: Fragments read: 600 fragments generated at 9.99921 getNext calls/sec, fragment rate = 9.99921 fragments/sec, monitor window = 60.0047 sec, min::max read size = 1::1 fragments Average times per fragment:  elapsed time = 0.100008 sec
        w            = lines[1].split();
        self.Print(name,1,f'line 1: w:{w}');
        
        nf_read      = int(w[2])
        gn_rate      = float(w[6])
        fr_rate      = float(w[12])
        win          = float(w[17])
        min_nf       = w[23].split(':')[0];
        max_nf       = w[23].split(':')[2];
        elapsed_time = float(w[32]);
        
        self.Print(name,1,f'nf_read:{nf_read} gn_rate:{gn_rate} fr_rate:{fr_rate} win:{win} min_nf:{min_nf} max_nf:{max_nf} elapsed_time:{elapsed_time}');

# line 2: Fragment output statistics: 600 fragments sent at 9.99921 fragments/sec, effective data rate = 0.270146 MB/sec, monitor window = 60.0047 sec, min::max event size = 0.0236282::0.0309525 MB.

        w       = lines[2].split();
        self.Print(name,1,f'line 2: w:{w}');
        nf_sent     = int(w[3])
        of_rate     = float(w[7]);
        dt_rate     = float(w[13]);
        time_win2   = float(w[18]);
        min_ev_size = float(w[24].split(':')[0])
        max_ev_size = float(w[24].split(':')[2])

        self.Print(name,1,f'nf_sent:{nf_sent} of_rate:{of_rate} dt_rate:{dt_rate} time_win2:{time_win2} min_ev_size:{min_ev_size} max_ev_size:{max_ev_size}' )
        
# line 3: Input wait time = 0.099996 s/fragment, buffer wait time = 3.00805e-06 s/fragment, request wait time = 0.0999935 s/fragment, output wait time = 3.76304e-06 s/fragment.

        w       = lines[3].split();
        self.Print(name,1,f'line 3: w:{w}');
        inp_wait_time  = float(w[4]);
        buf_wait_time  = float(w[10]);
        req_wait_time  = float(w[16]);
        out_wait_time  = float(w[22]);

        self.Print(name,1,f'inp_wait_time:{inp_wait_time} buf_wait_time:{buf_wait_time} req_wait_time:{req_wait_time} out_wait_time:{out_wait_time}')


        line_0 = f'"rn":{rn}, "nf_evt":{nf_evt}, "nf_sent":{nf_sent}';
        line_1 = f'"nf_read":{nf_read}, "gn_rate":{gn_rate}, "fr_rate":{fr_rate}, "time_win":{win}, "min_nf":{min_nf}, "max_nf":{max_nf}, "elapsed_time":{elapsed_time}';
        line_2 = f'"nf_sent":{nf_sent}, "of_rate":{of_rate}, "dt_rate":{dt_rate}, "time_win2":{time_win2}, "min_ev_size":{min_ev_size}, "max_ev_size":{max_ev_size}';
        line_3 = f'"inp_wait_time":{inp_wait_time}, "buf_wait_time":{buf_wait_time}, "req_wait_time":{req_wait_time}, "out_wait_time":{out_wait_time}';

        output = f'{{{line_0}, {line_1}, {line_2}, {line_3}, "fids":['
#------------------------------------------------------------------------------
# lines 4..8 : fragment_id:0 nfragments:0 nbytes:0 max_nf:1000 max_nb:1048576000
#------------------------------------------------------------------------------
        n = min(nf_evt,5);
        for i in range(0,n):
            w       = lines[4+i].split();
            self.Print(name,1,f'line {4+i}: w:{w}');

            fid    = int(w[0].split(':')[1])
            nf     = int(w[1].split(':')[1])
            max_nf = int(w[2].split(':')[1])
            max_nb = int(w[3].split(':')[1])
        
            self.Print(name,1,f'{{fid:{fid} nf:{nf} max_nf:{max_nf} max_nb:{max_nb}}}')

            line = f'{{"fid":{fid}, "nf":{nf}, "max_nf":{max_nf}, "max_nb":{max_nb}}}'
            if (i < n-1):
                line += ','
            output += line;
#-------v----------------------------------------------------------------------
        output += ']}'
        
        print(output);
        return;

#-------^----------------------------------------------------------------------
# print parsed EB metrics
#eb01 statistics:
#  Event statistics: 600 events released at 9.99928 events/sec, effective data rate = 0.270092 MB/sec, monitor window = 60.0043 sec, min::max event size = 0.0236664::0.0309906 MB
#  Average time per event:  elapsed time = 0.100007 sec
#  Fragment statistics: 600 fragments received at 9.99928 fragments/sec, effective data rate = 0.26971 MB/sec, monitor window = 60.0043 sec, min::max fragment size = 0.0236282::0.0309525 MB
#  Event counts: Run -- 141600 Total, 0 Incomplete.  Subrun -- 0 Total, 0 Incomplete. 
#shm_nbb :50:1153440:50:0:0:0
#---v--------------------------------------------------------------------------
    def parse_eb_metrics(self):
        name   = 'parse_eb_metrics';
        s      = xmlrpc.client.ServerProxy(self.url)
        res    = s.daq.report("stats");
        
        self.Print(name,1,f'res:\n{res}');

        lines  = res.splitlines();

        output = '{'
        
        nlines = len(lines)
        if (nlines != 6):
            # process errors...TODO
            output='{';
            output += '"nev_read":-1, "evt_rate":-1,  "data_rate":-1.0, "time_window":-1.0';
            output += ', "min_evt_size":-1.0, "max_evt_size":-1.0, "etime":-1.0';
            output += ', "nfr_read":-1, "nfr_rate":-1.0, "fr_data_rate":-1.0, "min_fr_size":-1.0, "max_fr_size":-1.0';
            output += ', "nevt_tot_rn":-1, "nevt_bad_rn":-1, "nevt_tot_sr":-1, "nevt_bad_sr":-1';
            output += ', "nbuf_shm_tot":-1, "nbytes_shm_tot":-1,"nbuf_shm_empty":-1,"nbuf_shm_write":-1,"nbuf_shm_full":-1,"nbuf_shm_read":-1';
        else:
#------------------------------------------------------------------------------
# regular case
#-----------v------------------------------------------------------------------
            w       = lines[1].split();
            self.Print(name,1,f'line 1: w:{w}');
        
            output += f'"nev_read":{w[2]}'
            output += f', "evt_rate":{w[6]}'
            output += f', "data_rate":{w[12]}'
            output += f', "time_window":{w[17]}'
            min_evt_size = w[23].split(':')[0];
            max_evt_size = w[23].split(':')[2];
            output += f', "min_evt_size":{min_evt_size}'
            output += f', "max_evt_size":{max_evt_size}'

#-----------v------------------------------------------------------------------
            w       = lines[2].split();
            self.Print(name,1,f'line 3: w:{w}');
        
            output += f', "etime":{w[7]}'
#------------------------------------------------------------------------------
#  Fragment statistics: 600 fragments received at 9.99928 fragments/sec, effective data rate = 0.26971 MB/sec, monitor window = 60.0043 sec, min::max fragment size = 0.0236282::0.0309525 MB
#-----------v------------------------------------------------------------------
            w       = lines[3].split();
            self.Print(name,1,f'line 3: w:{w}');
        
            output += f', "nfr_read":{w[2]}'
            output += f', "nfr_rate":{w[6]}'
            output += f', "fr_data_rate":{w[12]}'

            min_fr_size = w[23].split(':')[0];
            max_fr_size = w[23].split(':')[2];
            output += f', "min_fr_size":{min_fr_size}'
            output += f', "max_fr_size":{max_fr_size}'
#------------------------------------------------------------------------------
#  Event counts: Run -- 141600 Total, 0 Incomplete.  Subrun -- 0 Total, 0 Incomplete. 
#-----------v------------------------------------------------------------------
            w       = lines[4].split();
            self.Print(name,1,f'line 5: w:{w}');

            output += f', "nevt_tot_rn":{w[4]}'
            output += f', "nevt_bad_rn":{w[6]}'
            output += f', "nevt_tot_sr":{w[10]}'
            output += f', "nevt_bad_sr":{w[12]}'
#------------------------------------------------------------------------------
#shm_nbb :50:1153440:50:0:0:0
#-----------v------------------------------------------------------------------
            w       = lines[5].split();
            self.Print(name,1,f'line 5: w:{w}');

            nbuf = w[1].split(':');
            output += f', "nbuf_shm_tot":{nbuf[1]}'
            output += f', "nbytes_shm_tot":{nbuf[2]}'
            output += f', "nbuf_shm_empty":{nbuf[3]}'
            output += f', "nbuf_shm_write":{nbuf[4]}'
            output += f', "nbuf_shm_full":{nbuf[5]}'
            output += f', "nbuf_shm_read":{nbuf[6]}'
#------------------------------------------------------------------------------
# done parsing
#------------------------------------------------------------------------------
        output += '}'
        print(output);
        return

#------------------------------------------------------------------------------
if __name__ == "__main__":

    x = TestXmlrpc();
    x.parse_parameters();

    if (x.test == "t1"):
        x.test1()
    elif (x.test == "t2"):
        x.test2()
    elif (x.test == "t3"):
        x.test3()
    elif (x.test == "parse_br_metrics"):
        x.parse_br_metrics()
    elif (x.test == "parse_eb_metrics"):
        x.parse_eb_metrics()
