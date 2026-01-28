import  midas, TRACE
import  midas.client
import  os, socket, psycopg2
import  frontends.utils.runinfodb as fur;
import  logging

logger = logging.getLogger('midas')


# MIDAS client setup

#    def odb_get(self, key):
#        # Fetch values from MIDAS ODB
#        odb_data = {
#            "/Mu2e/PostgresqlDB/Database"  : "run_info_dev",
#            "/Mu2e/PostgresqlDB/Host"      : "mu2edaq14-ctrl",
#            "/Mu2e/PostgresqlDB/Port"      : "5434",
#            "/Mu2e/PostgresqlDB/User"      : "run_user",
#            "/Mu2e/PostgresqlDB/Pwd"       : "ask_antonio",
#            "/Mu2e/PostgresqlDB/Schema"    : "test",
#            "/Mu2e/RunConfigurations/demo/RunType": 4,
#            "/Mu2e/RunConfigurations/demo/TriggerTable": "demo_tt_v00",
#        }
#        return odb_data.get(key, None)
#
#    def odb_set(self, key, value):
#        print(f"ODB Set: {key} = {value}")
#        return 0  # Simulate success


def test1():
    # Initialize MIDAS client

    logger.info("Initializing %s" % "get_next_run")
    hostname = socket.gethostname().split('.')[0];
    client = midas.client.MidasClient("get_next_run", hostname, os.getenv('MIDAS_EXPT_NAME'), None)
   
    # Initialize the RuninfoDB

    db = fur.RuninfoDB(midas_client=client,config="config/runinfodb.json",)

    # Call next_run_number
    try:
        run_number = db.next_run_number(run_configuration="demo", store_in_odb=False)
        print(f"Next run number: {run_number}")

        db.register_transition(run_number,runinfo.START,0);
        db.register_transition(run_number,runinfo.START,1);
        
    except Exception as e:
        print(f"Error occurred: {e}")

#------------------------------------------------------------------------------
if __name__ == "__main__":

    test1()
