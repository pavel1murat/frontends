#!/usr/bin/env python
###############################################################################
# 2024-05-11 P.M. 
# - the ODB connection should be established by the caller
# - the same is true for disconnecting from ODB
# - thus cache midas_client and use it...
###############################################################################
import  os, socket, midas
import  midas.client
import  psycopg2, json
from    psycopg2 import extensions
import  logging

logger = logging.getLogger('midas')

import TRACE;
TRACE_NAME = "runinfodb";

class RuninfoDB:
#------------------------------------------------------------------------------
    def __init__(self, config_file ="config/runinfodb.json", midas_client = None):

        self.client = midas_client;
        self.info   = json.loads(open(config_file).read());

        TRACE.TRACE(8, "db:%s host:%s port:%s user:%s passwd:%s schema:%s"%
                    (self.database(),self.host(),self.port(),self.user(),self.passwd(),self.schema()),TRACE_NAME)
        
        if (self.user() == None):
            TRACE.ERROR("Postgresql DB user is not defined",TRACE_NAME)
            raise Exception("RuninfoDB::__init__ : Postgresql DB user is not defined")

        self.conn     = self.open_connection();
        self.cur      = self.conn.cursor();
        TRACE.TRACE(8, "after open_connection",TRACE_NAME)

#------------------------------------------------------------------------------
    def __del__(self):
        if (self.conn): self.conn.close();

    def database(self):
        return self.info["database"];

    def host(self):
        return self.info["host"];

    def port(self):
        return self.info["port"];

    def user(self):
        return self.info["user"];

    def passwd(self):
        return self.info["passwd"];

    def schema(self):
        return self.info["schema"];
#------------------------------------------------------------------------------
# rc: integer return code
#------------------------------------------------------------------------------
    def open_connection(self):

        TRACE.TRACE(8, "connecting to Postgresql, port:%s passwd:%s"%(self.port(),self.passwd()),TRACE_NAME)

        conn = psycopg2.connect(database = self.database(),
                                host     = self.host(),
                                user     = self.user(),
                                password = self.passwd(),
                                port     = self.port())

        if conn.status == extensions.STATUS_READY:
          print("Connection is active and ready.")
        elif conn.status == extensions.STATUS_BEGIN:
          print("A transaction is currently open.")
        elif conn.status == extensions.STATUS_PREPARED:
          print("Connection is in prepared state.")
        elif conn.status == extensions.STATUS_UNKNOWN:
          print("Connection is in an unknown state.")

        if (conn.status == extensions.STATUS_READY): 
            TRACE.TRACE(8, "success in connecting to Postgresql",TRACE_NAME);
            return conn;
        else:
            TRACE.ERROR("Unable to connect to run_info database!",TRACE_NAME)
            conn.close();
            return None

#---v--------------------------------------------------------------------------
    def execute_query(self, query):
        self.cur.execute(query)
        self.conn.commit()
        TRACE.TRACE(8, self.cur.statusmessage,TRACE_NAME)
        return
              
#-------^----------------------------------------------------------------------
# if connection is bad, try one more time to re-open it
#---v--------------------------------------------------------------------------
    def check_connection(self):
        if (self.conn): return 0;

        self.conn = self.open_connection();
        return
              
#-------^----------------------------------------------------------------------
# Run Control transition cause: 0:start 1:end
#---v--------------------------------------------------------------------------
    def register_transition(run_number, transition_type, cause_type):

        query = (f"INSERT INTO {self.schema()}.run_transition"
                 f"(run_number,transition_type,cause_type,transition_time)"
                 f" VALUES ({run_number},{transition_type},{cause_type},CURRENT_TIMESTAMP);"
             )

        self.execute_query(query)
        return

#-------^---------------------------------------------------------------------
# integrate with ODB, assume we are connected to ODB
#-----------------------------------------------------------------------------
    def next_run_number(self, store_in_odb):

        TRACE.TRACE(8,"001 next_run_number",TRACE_NAME)
        
        run_number              = -1;
        run_type                = -1;
        partition_number        = -1; 
        condition_id            = 100;
        context_version         = -1;
        online_software_version = -1;
        
        rc = self.check_connection();
        if (rc < 0): return rc;
#-----------------------------------------------------------------------------
# retrieve from ODB parameters to be stored in Postgresql
#-------v---------------------------------------------------------------------
        partition_number = self.client.odb_get("/Mu2e/ARTDAQ_PARTITION_NUMBER");
#-----------------------------------------------------------------------------
# everything else comes from the active configuration
#-------v---------------------------------------------------------------------
        run_config   = self.client.odb_get("/Mu2e/ActiveRunConfiguration");
        run_type     = self.client.odb_get(f"/Mu2e/RunConfigurations/{run_config}/RunType");
        tt_name      = self.client.odb_get(f"/Mu2e/RunConfigurations/{run_config}/Trigger/Table");
        hostname     = os.environ["HOSTNAME"]
        context_name = "test"
#-------------------------------------------------------------------------------
# write run info into db
#-  at this point run number in the DB gets incremented
#-------v-----------------------------------------------------------------------
        run_config_version = "1"
        tt_version         = "1"
        
        query = (f"INSERT INTO {self.schema()}.run_configuration(run_type, condition_id, host_name,"
                 f"artdaq_partition, configuration_name, configuration_version, "
                 f"context_name, context_version, online_software_version, "
                 f"trigger_table_name, trigger_table_version, commit_time) "
                 f"VALUES ('{run_type}','{condition_id}','{hostname}','{partition_number}','{run_config}',"
                 f"'{run_config_version}','{context_name}','{context_version}','{online_software_version}','{tt_name}','{tt_version}',CURRENT_TIMESTAMP);"
                 )

        TRACE.TRACE(8,"query=%s"%query,TRACE_NAME)
        self.execute_query(query)

        query = f"select max(run_number) from {self.schema()}.run_configuration;"
        self.execute_query(query)
        print("run number = ",run_number)
        row        = self.cur.fetchone()
        print(row)
        run_number = int(row[0])
        print("run_number ",run_number)

        run_info_conditions = "unknown"
        query = (f"INSERT INTO {self.schema()}.run_condition(condition,commit_time)  VALUES ("
                 f" '{run_info_conditions}',CURRENT_TIMESTAMP);"
                 )
        self.execute_query(query)
        query = f"select max(condition_id) from {self.schema()}.run_condition;"
        self.execute_query(query)
        print("condition id = ",condition_id)
#------------------------------------------------------------------------------
# P.M. hopefully, these are going away
# insert a new row in the run_condition table if 
# it pk(runConfiguration,runConfigurationVersion, runContext,runContextVersion) doesnt exist yet
#-------v----------------------------------------------------------------------
        #run_context         = "no_context"
        #run_context_version = "0"
        #query = ("SELECT configuration_name, configuration_version, context_name, context_version "
              #f"FROM   {self.schema()}.run_condition WHERE "
        #         f"configuration_name  = '{run_config}' AND "
        #         f"configuration_version = '{run_config_version}'  AND "
        #         f"context_name      = '{run_context}'  AND "
        #         f"context_version   = '{run_context_version}';"
        #     )

        #self.execute_query(query);
#------------------------------------------------------------------------------
# get primary key in the run_condition table
# pk looks like "aaa16bbb04"
#-------v----------------------------------------------------------------------
        #self.execute_query(query)
        #rows = self.cur.fetchall();
        #nlines = len(rows);# PQntuples(res)
        #print(f"Number of rows: {len(rows)}")
        #if (nlines != 1):
        # pk = "";
              #nfields = len(self.cur.description);# PQnfields(res);
            #for i in range(0,nlines-1):
            #for rowi in rows:
            #    pk += rowi[0];
                #for j in range(0,nfields-1):
                #    pk +=  # PQgetvalue(res, i, j);

            #print(f"pk ={pk}");

            #run_info_conditions = "unknown";
            #pk_run = run_config + run_config_version + run_context + run_context_version;

            #if (pk != pk_run):
            #query = (f"INSERT INTO {self.schema()}.run_condition(condition,commit_time)  VALUES"
            #         f" '{run_info_conditions}',CURRENT_TIMESTAMP);"
            #        )

            #self.execute_query(query);
  
#-------^----------------------------------------------------------------------
# instead of printing, store the next run number directly in ODB
# MIDAS increments the next run number, so subtract one in advance
#-------v----------------------------------------------------------------------
        print("run number",run_number);
        if (store_in_odb): 
            rc = self.client.odb_set("/Runinfo/Run number",run_number-1);
        return run_number 
#------^-----------------------------------------------------------------------


def test1():
    # Initialize MIDAS client

    TRACE.Instance = "runinfodb".encode();

    logger.info("Initializing %s" % "get_next_run")
    expt     = os.getenv("MIDAS_EXPT_NAME");
    host     = socket.gethostname();
    client   = midas.client.MidasClient("get_next_run",host,expt, None)
    cfg_file = os.getenv("MU2E_DAQ_DIR")+'/config/runinfodb.json';
   
    # Initialize the RuninfoDB
    runinfo_db = RuninfoDB(midas_client=client,config_file=cfg_file);

    print("back after creating RuninfoDB")
    
    # Call next_run_number
    try:
        next_run = runinfo_db.next_run_number(store_in_odb=True)
        print(f"Next run number:{next_run}")
    except Exception as e:
        print(f"Error occurred: {e}")
              
#------------------------------------------------------------------------------
if __name__ == "__main__":

    test1()
