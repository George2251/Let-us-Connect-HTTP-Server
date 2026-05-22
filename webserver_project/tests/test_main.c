#define _POSIX_C_SOURCE 200809L
#include "unity.h"

#include <stdlib.h>

#include <string.h>


extern int run_server_entry(int argc,char*argv[]);

int mock_network_init_fail =0;

int mock_thread_pool_init_fail=0;

void setUp()
{
    mock_network_init_fail=0;
    mock_thread_pool_init_fail=0;
}

void tearDown()
{

}

