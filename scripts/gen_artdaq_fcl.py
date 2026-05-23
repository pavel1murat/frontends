#!/usr/bin/env python
#------------------------------------------------------------------------------
# generates FCL for a given artdaq process
# gen_artdaq_fcl.py --run_conf=tracker_mc2 --process=eb08 --host=mu2e-trk-08
#------------------------------------------------------------------------------
# import subprocess, shutil, datetime
import sys, string, glob, os, time, re, array
from datetime import datetime
from zoneinfo import ZoneInfo
from   pathlib import Path
import argparse

# import json
# import xmlrpc.client
# import inspect, logging

import  midas.client
# import  psycopg2

import  TRACE  # midas
TRACE_NAME='gen_artdaq_fcl'

# logger = logging.getLogger('generate_fcl')

#------------------------------------------------------------------------------
# write single FCL to an already open file
# artdaqLabel = 'pname' - artdaq process label , for example, 'br01'
# proc : dict, corresponding to the artdaq process record in ODB
#------------------------------------------------------------------------------
def write_fcl(client,artdaqLabel,proc,fcl_template):

    config_dir=os.path.expandvars(client.odb_get('/Mu2e/ConfigDir'));
    template_fn = config_dir+f'/artdaq/common/{fcl_template}.fcl'

    TRACE.INFO(f'-- START: fcl_template:{fcl_template} config_dir:{config_dir} template_fn:{template_fn}',TRACE_NAME);

    output_dir = config_dir+f'/artdaq/{args.run_conf}'
    fn         = f'{output_dir}/{artdaqLabel}.fcl'

    TRACE.INFO(f'output_dir:{output_dir} fn:{fn}',TRACE_NAME);

    lines = []
    with open(template_fn) as f:
        lines = f.readlines()

        TRACE.INFO(f'len(lines): {len(lines)}',TRACE_NAME);
#------------------------------------------------------------------------------
# now - additions, different for different templates
#-------v----------------------------------------------------------------------
    if   (fcl_template == 'tracker_brdr'):
        with open(fn,'w') as fout:
            for line in lines:
                fout.write(line);
                
        # and append 
        with open(fn,'a') as fout:
            fout.write(f'daq.fragment_receiver.artdaqLabel  : {artdaqLabel}\n');
            fout.write(f'daq.fragment_receiver.fragment_ids : [ {proc["fragment_ids"]} ]\n');
                
    elif (fcl_template == 'mu2e_subevent_receiver'):
        with open(fn,'w') as fout:
            for line in lines:
                fout.write(line);
                
        with open(fn,'a') as fout:
            # this one takes only one fragment, so fragment_ids should be one ID
            fout.write(f'daq.fragment_receiver.fragment_id : {proc["fragment_ids"]}\n' );
#------------------------------------------------------------------------------
# CFO reader
#------------------------------------------------------------------------------
    elif (fcl_template == 'cfo_fragment_receiver'):
        with open(fn,'w') as fout:
            for line in lines:
                fout.write(line);
                
        with open(fn,'a') as fout:
            # this one takes only one fragment, so fragment_ids should be one ID
            fout.write(f'daq.fragment_receiver.rollover_subrun_interval : {proc["rollover_subrun_interval"]}\n' );
#------------------------------------------------------------------------------
# TOY generator
#------------------------------------------------------------------------------
    elif (fcl_template == 'toysim_fragment_receiver'):
        with open(fn,'w') as fout:
            for line in lines:
                fout.write(line);
        # append fragment types
        with open(fn,'a') as fout:
            fout.write(f'daq.fragment_receiver.fragment_type : "{proc["fragment_type"]}"\n');
            fout.write(f'daq.fragment_receiver.fragment_ids  : [ {proc["fragment_ids"]} ]\n' );
                
    elif ('event_builder' in fcl_template):
        trigger_table = client.odb_get("/Mu2e/ActiveRunConfiguration/Trigger/Table");

        TRACE.INFO(f'event_builder: {fcl_template} nlines:{len(lines)} trigger_table:{trigger_table}',TRACE_NAME)
        
        # in case of the event builder, do not append, override
        with open(fn,'w') as fout:
            for line in lines:
                if (re.search(r'\s*process_name\s*:',line)):
                    fout.write(f'    process_name : {artdaqLabel}\n');
                    continue;

                if (re.search(r'\s*physics\s*:\s*\{\s*\}',line)):
                    fout.write(f'    physics      : {{ @table::{trigger_table}.physics }}\n');
                    continue;

                if (re.search(r'\s*outputs\s*:\s*\{\s*\}',line)):
                    fout.write(f'    outputs      : {{ @table::{trigger_table}.outputs }}\n');
                    continue

                fout.write(line);

    elif ('data_logger' in fcl_template):
        with open(fn,'w') as fout:
            for line in lines:
                fout.write(line);
                
        with open(fn,'a') as fout:
            fout.write(f'art.process_name : {artdaqLabel}\n');

    elif (fcl_template == 'dispatcher'):
        with open(fn,'w') as fout:
            for line in lines:
                fout.write(line);
                
        with open(fn,'a') as fout:
            fout.write(f'art.process_name : {artdaqLabel}\n');

    TRACE.INFO(f'-- END:');

#------------------------------------------------------------------------------
# print statistics reported by a given artdaq process
#------------------------------------------------------------------------------
def generate_fcl(args):
    name = 'generate_fcl';
    TRACE.INFO('-- START',TRACE_NAME);

    client  = midas.client.MidasClient("gen_artdaq_fcl", None,"tracker",None)
#-----------------------------------------------------------------------------
# use None for the host , otherwise bools become ints - WHY ???
#------------------------------------------------------------------------------
       
    daq_nodes_path = f'/Mu2e/RunConfigurations/{args.run_conf}/DAQ/Nodes/'
    TRACE.INFO(f'------------- daq_nodes_path:{daq_nodes_path}',TRACE_NAME)
    daq_nodes = client.odb_get(daq_nodes_path)
    TRACE.DEBUG(1,f'------------- daq_nodes_dir:\n{daq_nodes}',TRACE_NAME)

    for host,params in daq_nodes.items():
        TRACE.INFO(f'host:{host:12} ----------- enabled:{params["Enabled"]} status:{params["Status"]}',TRACE_NAME);
        if ((args.host    != 'all') and (args.host   != host )): continue;
            
        if (params["Enabled"] == 0):                             continue
        artdaq = params["Artdaq"];       # should be a dict (subdirectory)
        if (artdaq["Enabled"] == 0):                             continue
        TRACE.DEBUG(1,f'artdaq:{artdaq}',TRACE_NAME)
        # pname - parameter name
        for pname,pdata in artdaq.items():
            TRACE.INFO(f'host:{host} pname:{pname} is_dict:{isinstance(pdata,dict)}',TRACE_NAME)
            if ((args.process != 'all') and (args.process != pname)): continue;
            if (not isinstance(pdata,dict)):                          continue;
            
            TRACE.INFO(f'pname:{pname} pdata["Enabled"]:{pdata["Enabled"]}',TRACE_NAME)
            
            if (pdata['Enabled'] == 0):                          continue;
            #-----------------------------------------------------------------------------
            # this is a link
            #------------------------------------------------------------------------------
            fcl_template_path = pdata['fcl_template']
            fcl_template_name = client.odb_get(fcl_template_path); 
            #                fcl_template_name = pdata['fcl_template']
            ## print(f'     fcl_template:{fcl_template_name}')
            #---------------^--------------------------------------------------------------
            # found template name, templates are stored in config/artdaq/common
            # now only need to check what is requested
            #---------------v--------------------------------------------------------------
            TRACE.INFO(f'generating fcl for run_conf:{args.run_conf} host:{host} process:{pname} using template:{fcl_template_name}',TRACE_NAME)
            #---------------^--------------------------------------------------------------
            # templates are stored in /Mu2e/RunConfigurations/{run_conf}/DAQ/FclTemplates
            # step 1: save existing FCL file
            #---------------v--------------------------------------------------------------
            config_dir = os.path.expandvars(client.odb_get('/Mu2e/ConfigDir'))+f'/artdaq/{args.run_conf}'
            fcl_fn = f'{config_dir}/{pname}.fcl'
            fpath = Path(fcl_fn)
            TRACE.INFO(f'config_dir:{config_dir} fcl_fn:{fcl_fn}',TRACE_NAME);
            if (fpath.exists()):
                # FHICL file exists, save it
                tstamp = datetime.now().strftime("%Y-%m-%d-%H-%M-%S")
                cmd    = f'cp {fcl_fn} {fcl_fn}.save.{tstamp}'
                os.system(cmd)
#---------------^--------------------------------------------------------------
# step 2: generate new fcl and save it, error handling to be added
#         proc is a dict coresponding to the artdaq process record in ODB
#---------------v--------------------------------------------------------------
            TRACE.INFO(f'fcl_template_name:{fcl_template_name}',TRACE_NAME)
            write_fcl(client,pname,pdata,fcl_template_name)

    client.disconnect();
    
    TRACE.INFO('-- END',TRACE_NAME);
    return;
    
#------------------------------------------------------------------------------
if __name__ == "__main__":
    
    parser = argparse.ArgumentParser()

    parser.add_argument('--process'   , default='all',         help="label of the artdaq process")
    parser.add_argument('--diag_level', default=0,             help="diag level")
    parser.add_argument('--run_conf'  , default='tracker_mc2', help="run configuration name")
    parser.add_argument('--host'      , default='all'        , help="process host, do we need it ?")

    args = parser.parse_args();

    TRACE.INFO(f'args.process:{args.process} args.run_conf:{args.run_conf} args.diag_level:{args.diag_level}',TRACE_NAME);

    TRACE.INFO('after constructor',TRACE_NAME)
    
    generate_fcl(args);

    TRACE.INFO('-- END: after generate_fcl',TRACE_NAME)

  
