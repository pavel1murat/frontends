#!/usr/bin/env python
#------------------------------------------------------------------------------
# PM:
# - read lists of good runs from ODB
# - check data of which runs have not been saved to ODB
# - save them, opdate ODB
#
# call signature:
#                     copy_data_to_dropbox.py [--dry-run] 
#------------------------------------------------------------------------------
import  midas,TRACE
import  midas.client
import  os, psycopg2, socket, sys, getopt, time, copy, glob
import  shutil
import  argparse
import  json
import  logging;
from    pathlib                  import Path
from    datetime                 import datetime
from    zoneinfo                 import ZoneInfo

import TRACE ; TRACE_NAME='copy_data_to_dropbox'

logger = logging.getLogger('midas')
#------------------------------------------------------------------------------
# plane : 25 : plane_25
#         21 : plane_21
# example : set thresholds(21,'MN101')
#           set thresholds(25,'MN262')
#------------------------------------------------------------------------------
class CopyDataToDropbox:
    
    def __init__(self):
        self.diag_level = 0;
        
#------------------------------------------------------------------------------
    def parse_parameters(self):
        name = 'parse_parameters'
        
#        logger.info('Starting')
#        logger.info(f'sys.argv:{sys.argv}')
        
        parser = argparse.ArgumentParser()

        parser.add_argument('--dry-run'         , action='store_true',    help="dry run if set")

        args = parser.parse_args()

        # logger.info(f'self.diag_level = {args.diag_level}'   )

        # some parameters need parsing

#        logger.info(f'------------------------------------- Done')
        return args
    
#------------------------------------------------------------------------------

    def print_channel_mask(self, chmask):
        for i in range(0,6):
            for j in range(0,16):
                print (f'{chmask[6*i+j]}',end='');
            print(" ",end='')        
        print('')
        return

    def record_saved(self,odb_path,saved):
        # for some reason, can't re-use 'client' variable name, so use 'client1'
        node            = socket.gethostname().split('.')[0];
        experiment_name = "tracker";
        client = midas.client.MidasClient("dropbox",node,experiment_name,None)
        TRACE.INFO(f'client connected...')
        client.odb_set(odb_path+'/saved',saved)
        client.disconnect();

#------------------------------------------------------------------------------
# different good channel maps for different thresholds
# location: $thresholds_dir/{panel_name}_channel_mask.json
#---v--------------------------------------------------------------------------
    def copy_data(self,args):

        TRACE.INFO(f'-- START');

        base       = '/Mu2e/DataHandling/GoodRuns';
        
        source_dir = '/data/mu2e/mu2etrk/daquser_002_v001/data'
        dest_dir   = '/data/DAQ/dropbox'

        node            = socket.gethostname().split('.')[0];
        experiment_name = "tracker";
        
        client          = midas.client.MidasClient("dropbox",node,experiment_name,None)
        d1000 = client.odb_get(base)
        client.disconnect();

        for k1000 in d1000.keys():
            print(" -", k1000)
            list_of_runs = d1000[k1000]
            for run_number in list_of_runs:
                run = list_of_runs[run_number]
                # dont always need to print this info
                TRACE.DEBUG(0,f'  - run:{run_number} saved:{run["saved"]}')
                if (run['saved'] == True): continue
                
                TRACE.INFO(f'saving run {run_number} data')

                pattern = os.path.join(source_dir, f'raw.mu2e.trk.vst.{run_number:06}_*.art')
                matches = glob.glob(pattern)
                
                if matches:
                    nfiles   = len(matches)
                    n_copied = 0
                    for src_file in matches:
                        bn = Path(src_file).name
                        dest_file = os.path.join(dest_dir,bn);
                        print(f'run {run_number}: copying {src_file} to {dest_file}')
                        try:
                            out = "test"
                            if (not args.dry_run):
                                out = shutil.copyfile(src_file, dest_file)
                                
                            print(f"SUCCESS: {src_file} copied to {out}")
                            n_copied += 1
                            
                        except OSError as e:
                            print(f"ERROR: copy of {src_file} failed:", e)

                    TRACE.INFO(f'nfiles:{nfiles} n_copied:{n_copied}')

                    if (n_copied == nfiles):
                        if (not args.dry_run):
                            odb_path = f'{base}/{k1000}/{run_number}'
                            saved = True
                            
                            TRACE.INFO(f'saving saved:{saved} to odb_path:{odb_path}')

                            self.record_saved(odb_path,saved);
                            
                            TRACE.INFO(f'run:{run_number} is saved')
                        else:
                            TRACE.INFO(f'DRY_RUN: run:{run_number} is saved')
                    
                    else:
                        print(f'{run}: WARNING: no matching files found: {pattern}')

        TRACE.INFO(f'-- END:')
        return

#------------------------------------------------------------------------------
if __name__ == "__main__":
    x = CopyDataToDropbox();
    
    args = x.parse_parameters();
    x.copy_data(args)
        
    sys.exit(0)
