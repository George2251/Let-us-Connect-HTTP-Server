#define _POSIX_C_SOURCE 200809L
#include "Dictionary.h"
#include "BinarySearchTree.h"
#include "LinkedList.h"
#include "Node.h"
#include <string.h>
#include <stdlib.h>

void setUp()
{

}

void tearDown()
{

}


void test_dictionary_should_prevent_null_and_empty_string_crashes()
{
    struct Dictionary dict= dictionary_constructor(compare_string_keys);
    
    char*key_empty="";
    char*val_empty="empty_val";

}