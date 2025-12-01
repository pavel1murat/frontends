#!/usr/bin/env python
#------------------------------------------------------------------------------
# source: frontends/scripts/generate_artdaq_fcl.py
# call: ./test_generate_artdaq_fcl.py [--diag_level=1] --run_conf=tracker_mc2 --host=mu2e-trk-01 --process=br01
#       if 'host' is not defined, generates FCL files for all enabled hosts included into the configuration
#       if 'process; is not defined, generate FCL files for all processes
# output FCL files are stored in $MU2E_DAQ_DIR/config/{run_conf}
# protection against errors: previous version of an FCL file 'aaa.fcl' is copied into 'aaa.fcl.save' 
#------------------------------------------------------------------------------
# import subprocess, shutil, datetime
import sys, string, getopt, glob, os, time, re, array
# import json
# import xmlrpc.client
# import inspect, logging

# import  midas,TRACE
import  midas.client
# import  psycopg2

# logger = logging.getLogger('generate_fcl')
#------------------------------------------------------------------------------
#
#------------------------------------------------------------------------------
class GenerateArtdaqFcl:
    
    def __init__(self):
        self.diag_level   = 0;
        self.client       = None;
        self.run_conf     = None;
        self.process      = 'all';
        self.host         = 'all'
        
# ---------------------------------------------------------------------
    def Print(self,Name,level,Message):
        if (level > self.diag_level): return 0;
        now = time.strftime('%Y/%m/%d %H:%M:%S',time.localtime(time.time()))
        message = now+' GenerateArtdaqFcl::'+Name+': '+Message
        print(message)

#------------------------------------------------------------------------------
# job: 'br' or 'eb' or 'ds' or 'dl'
#------------------------------------------------------------------------------
    def parse_parameters(self):
        name = 'parse_parameters'
        
        self.Print(name,2,'Starting')
        self.Print(name,2, '%s' % sys.argv)

        try:
            optlist, args = getopt.getopt(sys.argv[1:], '',
                     ['diag_level=', 'process=', 'host=', 'run_conf=' ] )
 
        except getopt.GetoptError:
            self.Print(name,0,'%s' % sys.argv)
            self.Print(name,0,'Errors arguments did not parse')
            return 110

        for key, val in optlist:

            # print('key,val = ',key,val)

            if   (key == '--diag_level'):
                self.diag_level = int(val)
            elif (key == '--host'):  # defines the output file name
                self.host = val
            elif (key == '--process'):  # defines the output file name
                self.process = val
            elif (key == '--run_conf'):  # defines the output directory
                self.run_conf = val

        self.Print(name,1,f'run_conf = {self.run_conf}')
        self.Print(name,1,f'process  = {self.process}' )
        self.Print(name,1,f'verbose  = {self.diag_level}')

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
# print dictionary
#------------------------------------------------------------------------------
    def print_dict(self,dictionary,offset=''):
        indent = '    ';
        for key,val in dictionary.items(): 
            self.Print('print_dict',1, f'  {offset} key:{key} val:{val}')
            if (key == 'Enabled') or (key == 'Status') : continue;
            if isinstance(val,dict):
                print(f'{offset}{key}: {{');
                self.print_dict(val,offset+indent)
                print(f'{offset}}}');
            else:
                if (isinstance(val,str)):
                    print(f'{offset}{key}: "{val}"');
                else:
                    print(f'{offset}{key}: {val}');

#------------------------------------------------------------------------------
# if the key starts from '$', it is an abbreviation: in this case,
# the length of the ARTDAQ key is > 32, so the ODB key needs to be substituted
#------------------------------------------------------------------------------
    def checked_key(self,key):
        k = key;
        if (k[0] == '$'):
            print(f'--- key:{key} - to be substituted')
            dict_path = f'/Mu2e/RunConfigurations/{self.run_conf}/DAQ/FclTemplates/Dictionary'
            d         = self.client.odb_get(dict_path)
            k         = d[key];

        return k
        
#------------------------------------------------------------------------------
# write single FCL to an already open file
# 'pname' - artdaq process label , for example, 'br01'
#------------------------------------------------------------------------------
    def write_fcl(self,file,artdaqLabel,fcl_template,offset=''):
        indent = '    ';
        for key,val in fcl_template.items(): 
            self.Print('write_fcl',1, f'  {offset} key:{key} val:{val}')
            if (key == 'Enabled') or (key == 'Status') : continue;
            if isinstance(val,dict):
                k = self.checked_key(key);
                file.write(f'{offset}{k}: {{\n');
                self.write_fcl(file,artdaqLabel,val,offset+'  ')
                file.write(f'{offset}}}\n');
            else:
#------------------------------------------------------------------------------
# for a boardreader, its artdaqLabel is defined by the process label
# also, constrain the artdaq process name to be the same as the label
#------------------------------------------------------------------------------
                if (key == "artdaqLabel"):
                    file.write(f'{offset}{key}: {artdaqLabel}\n');
                if (key == "process_name"):
                    file.write(f'{offset}{key}: {artdaqLabel}\n');
                else:
#------------------------------------------------------------------------------
# check whether the key itself is too long and needs to be substituted
# if it it too long, it starts from a '$' sign
#-------------------v----------------------------------------------------------
                    k        = self.checked_key(key)
                    real_val = val;
                    if   (isinstance(val,bool)):
#---------------------------^--------------------------------------------------
# Python 'True' and 'False' start from a capitals, can't use automated conversion
#-----------------------v----------------------------------------------
                        if (val == True): real_val = 'true'
                        else            : real_val = 'false'
                        file.write(f'{offset}{k}: {real_val}\n');
                    elif (isinstance(val,str)):
                        if (val[0] == '['):
#-----------------------^--------------------------------------------------
# not a string, but an array, write it out 
#---------------------------v----------------------------------------------
                            file.write(f'{offset}{k}: [');
                            s = val[1:len(val)-1]
                            real_val = []
                            words = s.split(',')
                            nw    = len(words);
                            for i in range(nw):
                                w = words[i].strip()
                                if (w != ''): file.write(f' {w}');
                                if (i < nw-1): file.write(', ')
                                else         : file.write(' ')
                                
                            file.write(f']\n');
                        else:
                            file.write(f'{offset}{k}: {real_val}\n');
                    else:
                        file.write(f'{offset}{k}: {real_val}\n');

#------------------------------------------------------------------------------
# print statistics reported by a given artdaq process
#------------------------------------------------------------------------------
    def generate_fcl(self):
        name = 'generate_fcl';
#-----------------------------------------------------------------------------
# use None for the host , otherwise bools become ints - WHY ???
#------------------------------------------------------------------------------
        self.client = midas.client.MidasClient("generate_fcl", None, "tracker", None)
        
        daq_nodes_path = f'/Mu2e/RunConfigurations/{self.run_conf}/DAQ/Nodes/'
        self.Print('generate_fcl',1,f'------------- daq_nodes_path:{daq_nodes_path}')
        daq_nodes_dir = self.client.odb_get(daq_nodes_path)
        # print(f'------------- daq_nodes_dir:\n{daq_nodes_dir}')

        for host,params in daq_nodes_dir.items():
            self.Print('generate_fcl',1,f'-- host:{host:12} enabled:{params["Enabled"]} status:{params["Status"]}');
            if ((self.host    != 'all') and (self.host   != host )): continue;
            
            if (params["Enabled"] == 0):                             continue
            artdaq = params["Artdaq"]; # should be a dict (subdirectory)
            if (artdaq["Enabled"] == 0):                             continue
            # print(f'artdaq:{artdaq}')
            for pname,proc in artdaq.items():
                if ((self.process != 'all') and (self.process != pname)): continue;
                if ((pname == "Enabled") or (pname == "Status")):    continue;
                self.Print('generate_fcl',1,f'-- pname:{pname}');
                if (proc['Enabled'] == 0):                           continue;
                
                fcl_template_name = proc['fcl_template']
                # print(f'     fcl_template:{fcl_template_name}')
#---------------^--------------------------------------------------------------
# now only need to check what is requested
#---------------v--------------------------------------------------------------
                self.Print('generate_fcl',0,f'generating fcl for run_conf:{self.run_conf} host:{host} process:{pname} using template:{fcl_template_name}')
#---------------^--------------------------------------------------------------
# templates are stored in /Mu2e/RunConfigurations/{self.run_conf}/DAQ/FclTemplates
# step 1: save existing FCL file
#---------------v--------------------------------------------------------------
                config_dir = os.path.expandvars(self.client.odb_get('/Mu2e/ConfigDir'))+f'/{self.run_conf}'
                self.Print('generate_fcl',1,f'config_dir:{config_dir}');
                fcl_fn = f'{config_dir}/{pname}.fcl'
                cmd    = f'cp {fcl_fn} {fcl_fn}.save'
                os.system(cmd)
#---------------^--------------------------------------------------------------
# step 2: generate new fcl and save it, error handling to be added
#---------------v--------------------------------------------------------------
                fcl_template_path = f'/Mu2e/RunConfigurations/{self.run_conf}/DAQ/FclTemplates/{fcl_template_name}'
                self.Print('generate_fcl',1,f'fcl_template_path:{fcl_template_path}')
                fcl_template      = self.client.odb_get(fcl_template_path)
                with open(fcl_fn, "w") as f:
                    self.write_fcl(f,pname,fcl_template)
                    
        return;
    
#------------------------------------------------------------------------------
if __name__ == "__main__":

    x = GenerateArtdaqFcl();
    x.parse_parameters();
    x.generate_fcl();
  
