//-----------------------------------------------------------------------------
// Example creating a client that registers an rpc call
//-----------------------------------------------------------------------------
#include <stdio.h>
#include <stdlib.h>
#include "midas.h"
#include "mrpc.h"
#include "mjson.h"
//#include "mjsonrpc.h"


// this only works/exists in mhttpd
//// test of a JSON-RPC handler
//static MJsonNode* handle_test(const MJsonNode* params)
//{
//   if (!params) {
//      MJSO* doc = MJSO::I();
//      doc->D("test description");
//      doc->P("par",  MJSON_STRING, "test parameter");
//      doc->R("result", MJSON_INT, "returns a result");
//      doc->R("data", MJSON_INT, "returns a result");
//      return doc;
//   }
//
//   //MJsonNode* error = NULL;
//   MJsonNode* dresult = MJsonNode::MakeArray();
//   MJsonNode* sresult = MJsonNode::MakeArray();
//
//   dresult->AddToArray(MJsonNode::MakeNull());
//   sresult->AddToArray(MJsonNode::MakeInt(5));
//   dresult->AddToArray(MJsonNode::MakeJSON(""));
//   sresult->AddToArray(MJsonNode::MakeInt(1));
//   return mjsonrpc_make_result("data", dresult, "status", sresult);
//}


// Test of a old-style RPC function
INT test_function(INT index, void *prpc_param[])
{
    printf("RPC test function called\n");

    //const char* cmd  = CSTRING(0);
    //const char* args = CSTRING(1);
    char* return_buf = CSTRING(2);
    //int   return_max_length = CINT(3);
    sprintf(return_buf, "Hello from mepics");

    return SUCCESS;
}



int main(int argc, char *argv[])
{	
    INT status;
    char host_name[256];
    char exp_name[32];

    status = cm_get_environment(host_name, sizeof(host_name), exp_name, sizeof(exp_name));
    if (status != CM_SUCCESS) {
        printf("Cannot get environment settings\n");
        return 1;
    }

    status = cm_connect_experiment(host_name, exp_name, "mepics", NULL);
    if (status != CM_SUCCESS) {
        printf("Cannot connect to experiment\n");
        return 1;
    }

    status = cm_register_function(RPC_JRPC, test_function);
    if (status != CM_SUCCESS) {
        printf("Cannot register RPC function\n");
        return 1;
    }

    //mjsonrpc_add_handler("mepics_test", handle_test, false);

    cm_start_watchdog_thread();

    while (!cm_is_ctrlc_pressed()) {
        status = cm_yield(1000);
        if (status == RPC_SHUTDOWN)
            break;
    }

    cm_stop_watchdog_thread();
    cm_disconnect_experiment();
    return 0;
}
