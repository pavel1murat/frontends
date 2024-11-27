import  midas,TRACE
from runinfodb import RuninfoDB
import os, psycopg2

# MIDAS client setup
class MidasClient:
    def __init__(self):
        # Add any initialization if needed
        pass

    def odb_get(self, key):
        # Fetch values from MIDAS ODB
        odb_data = {
            "/Mu2e/PostgresqlDB/Database": "run_info_dev",
            "/Mu2e/PostgresqlDB/Host": "mu2edaq14-ctrl",
            "/Mu2e/PostgresqlDB/Port": "5434",
            "/Mu2e/PostgresqlDB/User": "run_user",
            "/Mu2e/PostgresqlDB/Pwd": "daq14_run_user",
            "/Mu2e/PostgresqlDB/Schema": "test",
            "/Mu2e/ARTDAQ_PARTITION_NUMBER": 13,
            "/Mu2e/RunConfigurations/demo/RunType": 4,
            "/Mu2e/RunConfigurations/demo/TriggerTable": "demo_tt_v00",
        }
        return odb_data.get(key, None)

    def odb_set(self, key, value):
        print(f"ODB Set: {key} = {value}")
        return 0  # Simulate success


def test1():
    # Initialize MIDAS client
    client = MidasClient()

    # Initialize the RuninfoDB
    runinfo_db = RuninfoDB(client)

    # Call next_run_number
    try:
        next_run = runinfo_db.next_run_number(run_configuration="demo", store_in_odb=True)
        print(f"Next run number: {next_run}")
    except Exception as e:
        print(f"Error occurred: {e}")

#------------------------------------------------------------------------------
if __name__ == "__main__":

    test1()
