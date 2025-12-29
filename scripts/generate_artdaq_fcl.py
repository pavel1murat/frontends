#!/usr/bin/env python
#------------------------------------------------------------------------------
# generates FCL for a given artdaq process
#------------------------------------------------------------------------------
# import subprocess, shutil, datetime
import sys, string, getopt, glob, os, time, re, array
from   pathlib import Path
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
        message = now+' [ GenerateArtdaqFcl::'+Name+' ] '+Message
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

        self.Print(name,1,'run_conf  = %s' % self.run_conf)
        self.Print(name,1,'process   = %s' % self.process)
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
# print dictionary
#------------------------------------------------------------------------------
    def print_dict(self,dictionary,offset=''):
        for key,val in dictionary.items(): 
            self.Print('print_dict',1, f'  {offset} key:{key} val:{val}')
            if (key == 'Enabled') or (key == 'Status') : continue;
            if isinstance(val,dict):
                print(f'{offset}{key}: {{');
                self.print_dict(val,offset+'  ')
                print(f'{offset}}}');
            else:
                if (isinstance(val,str)):
                    print(f'{offset}{key}: "{val}"');
                else:
                    print(f'{offset}{key}: {val}');

#------------------------------------------------------------------------------
# write single FCL to an already open file
# 'pname' - artdaq process label , for example, 'br01'
#------------------------------------------------------------------------------
    def write_fcl(self,file,artdaqLabel,fcl_template,key_dict,offset=''):
            for key,val in fcl_template.items():
                k_key = key;
                self.Print('write_fcl',1, f'  {offset} key:{key} val:{val}')
#------------------------------------------------------------------------------
# skip 'Enabled' and 'Status' - those are purely internal.. do we need to do that ?
#------------------------------------------------------------------------------
                if (key == 'Enabled') or (key == 'Status') : continue;
                if (key[0] == '$'):
                    # substitute key with the dictionary value (the word is lnger 32 chars)
                    k_key = key_dict[key];
                if isinstance(val,dict):
                    file.write(f'{offset}{k_key}: {{\n');
                    self.write_fcl(file,artdaqLabel,val,key_dict,offset+'  ')
                    file.write(f'{offset}}}\n');
                else:
                    if (isinstance(val,str)):
                        if (key == 'artdaqLabel'):
#------------------------------------------------------------------------------
# for a boardreader, its artdaqLabel is defined by the process label
#------------------------------------------------------------------------------
                            file.write(f'{offset}{k_key}: "{artdaqLabel}"\n');
                        elif (key == 'process_name'):
                            file.write(f'{offset}{k_key}: "{artdaqLabel}"\n');
#------------------------------------------------------------------------------
# process_name comes from the process name
#------------------------------------------------------------------------------
                        else:
                            file.write(f'{offset}{k_key}: "{val}"\n');
                    else:
                        file.write(f'{offset}{k_key}: {val}\n');

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
            self.Print('generate_fcl',1,f'---- host:{host:12} enabled:{params["Enabled"]} status:{params["Status"]}');
            if ((self.host    != 'all') and (self.host   != host )): continue;
            
            if (params["Enabled"] == 0):                             continue
            artdaq = params["Artdaq"]; # should be a dict (subdirectory)
            if (artdaq["Enabled"] == 0):                             continue
            # print(f'artdaq:{artdaq}')
            for pname,proc in artdaq.items():
                if ((self.process != 'all') and (self.process != pname)): continue;
                if ((pname == "Enabled") or (pname == "Status")):    continue;
                self.Print('generate_fcl',1,f'     ---- pname:{pname}');
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
                fpath = Path(fcl_fn)
                if (fpath.exists()):
                    # FHICL file exists, save it
                    cmd    = f'cp {fcl_fn} {fcl_fn}.save'
                    os.system(cmd)
#---------------^--------------------------------------------------------------
# step 2: generate new fcl and save it, error handling to be added
#---------------v--------------------------------------------------------------
                fcl_template_path = f'/Mu2e/RunConfigurations/{self.run_conf}/DAQ/FclTemplates/{fcl_template_name}'
                key_dict_path     = f'/Mu2e/RunConfigurations/{self.run_conf}/DAQ/FclTemplates/Dictionary'
                self.Print('generate_fcl',1,f'fcl_template_path:{fcl_template_path}')
                fcl_template      = self.client.odb_get(fcl_template_path)
                key_dict          = self.client.odb_get(key_dict_path);
                self.Print('generate_fcl',1,f'key_dict:{key_dict}');
                with open(fcl_fn, "w") as f:
                    self.write_fcl(f,pname,fcl_template,key_dict)
                    
        return;
    
#------------------------------------------------------------------------------
if __name__ == "__main__":

    x = GenerateArtdaqFcl();
    x.parse_parameters();
    x.generate_fcl();
  
