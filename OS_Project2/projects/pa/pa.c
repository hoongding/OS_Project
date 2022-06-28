#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "threads/init.h"
#include "threads/malloc.h"
#include "threads/palloc.h"
#include "threads/thread.h"
#include "projects/pa/pa.h"

void run_patest(char **argv)
{   
    palloc_get_status(0);
    void * palloc1 = palloc_get_multiple(0,63);
    palloc_get_status(0);

    void * palloc2 = palloc_get_multiple(0,63);
    palloc_get_status(0);

    palloc_free_multiple(palloc1, 63);
    palloc_get_status(0);

    void * palloc3 = palloc_get_multiple(0,8);
    palloc_get_status(0);

    void * palloc4 = palloc_get_multiple(0,14);
    palloc_get_status(0);

    palloc_free_multiple(palloc3, 8);
    palloc_get_status(0);

    palloc_free_multiple(palloc2, 63);
    palloc_get_status(0);

    palloc_free_multiple(palloc4, 14);
    palloc_get_status(0);

    void * palloc5 = palloc_get_multiple(0,3);
    palloc_get_status(0);

    void * palloc6 = palloc_get_multiple(0,30);
    palloc_get_status(0);
    
    void * palloc7 = palloc_get_multiple(0,62);
    palloc_get_status(0);

    void * palloc8 = palloc_get_multiple(0, 120);
    palloc_get_status(0);
    while(1){
        timer_msleep(1000);
    }
}