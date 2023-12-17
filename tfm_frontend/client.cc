#include <cstdlib>
#include <string>
#include <iostream>

#include <stdlib.h>
#include <stdio.h>

#include <xmlrpc-c/base.h>
#include <xmlrpc-c/client.h>

// #include "config.h"  /* information about this build environment */

#define NAME "XML-RPC C Test Client synch_client"
#define VERSION "1.0"

// void dumpValue(const char* prefix, struct _xmlrpc_value* valueP);

static void die_if_fault_occurred(xmlrpc_env * const envP) {
  if (envP->fault_occurred) {
    fprintf(stderr, "XML-RPC Fault: %s (%d)\n",envP->fault_string, envP->fault_code);
    exit(1);
  }
}

//-----------------------------------------------------------------------------
int main(int argc, char ** argv) {
                                        // the first and the second parameters are "daqint" and "configuring"
    xmlrpc_env    env;
    xmlrpc_value* resultP;

    xmlrpc_client_init(XMLRPC_CLIENT_NO_FLAGS, NAME, VERSION);

    xmlrpc_env_init(&env);

    resultP = xmlrpc_client_call(&env, 
                                 "http://localhost:18000/RPC2",
                                 "state_change",
                                 // "({s:i,s:i})",
                                 "(ss{s:i})", 
                                 "daqint",
                                 "configuring",
                                 "run_number", 1);
    die_if_fault_occurred(&env);
    size_t              length;
    const char*         value;
    xmlrpc_read_string_lp(&env, resultP, &length, &value);

    printf(" results: %s\n",value);
    //    dumpValue("result:",resultP);

    // xmlrpc_read_string(&env, resultP, &stateName);
    // die_if_fault_occurred(&env);
    // printf("%s\n", stateName);
    // free((char*)stateName);
   
    /* Dispose of our result value. */
    xmlrpc_DECREF(resultP);

    /* Clean up our error-handling environment. */
    xmlrpc_env_clean(&env);
    
    /* Shutdown our XML-RPC client library. */
    xmlrpc_client_cleanup();

    return 0;
}
