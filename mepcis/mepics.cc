//-----------------------------------------------------------------------------
// Example creating a client that registers an rpc call
//-----------------------------------------------------------------------------
#include <stdio.h>
#include <stdlib.h>
#include "midas.h"
#include "mrpc.h"
//#include "mjson.h"

//int handle_test(midas_json_rpc_conn_t *conn, const mjson_t *json, midas_json_rpc_result_t *result)
//{
//    // Example response data
//    mjson_write_string(result->buf, result->size, "Hello from MEPICS!");
//    return MJSON_RPC_SUCCESS;
//}
INT test_function(INT index, void *prpc_param[])
{
    printf("RPC test function called\n");

    const char* cmd  = CSTRING(0);
    const char* args = CSTRING(1);
    char* return_buf = CSTRING(2);
    int   return_max_length = CINT(3);
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

    // Register the JSON-RPC handler
    //status = mjsonrpc_add_handler("test", handle_test);
    //if (status != SUCCESS) {
    //    printf("Cannot register JSON-RPC handler\n");
    //    return 1;
    //}
    // Register traditional RPC function
    status = cm_register_function(RPC_JRPC, test_function);
    if (status != CM_SUCCESS) {
        printf("Cannot register RPC function\n");
        return 1;
    }

    while (!cm_is_ctrlc_pressed()) {
        cm_yield(1000);
    }

    cm_disconnect_experiment();
    return 0;
}
