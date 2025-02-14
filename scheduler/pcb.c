/* PCB management functions for RR dispatcher */

/* Include Files */
#include "pcb.h"

/*******************************************************
 * PcbPtr createnullPcb() - create inactive Pcb.
 *
 * returns:
 *    PcbPtr of newly initialised Pcb
 *    NULL if malloc failed
 ******************************************************/
PcbPtr createnullPcb()
{
    PcbPtr new_process_Ptr;
    if (!(new_process_Ptr = (PcbPtr)malloc(sizeof(Pcb))))
    {
        fprintf(stderr, "ERROR: Could not create new process control block\n");
        return NULL;
    }
    new_process_Ptr->pid = 0;
    new_process_Ptr->args[0] = "./process";
    new_process_Ptr->args[1] = NULL;
    new_process_Ptr->arrival_time = 0;
    new_process_Ptr->service_time = 0;
    new_process_Ptr->remaining_cpu_time = 0;
    new_process_Ptr->status = PCB_UNINITIALIZED;
    new_process_Ptr->next = NULL;
    new_process_Ptr->iter = 0;
    new_process_Ptr->mab = NULL;
    new_process_Ptr->mab_size = 0;
    new_process_Ptr->remaining_iter=-1;
    new_process_Ptr->priority = LEVEL0;
    return new_process_Ptr;
}

/*******************************************************
 * PcbPtr enqPcb (PcbPtr headofQ, PcbPtr process)
 *    - queue process (or join queues) at end of queue
 *
 * returns head of queue
 ******************************************************/
PcbPtr enqPcb(PcbPtr q, PcbPtr p)
{
    if (!q)
    {
        return p;
    }
    else
    {   
    //    printf("qp: %d pp:%d\n",q->priority,p->priority);
        PcbPtr tmp = q;
        for (; tmp->next != NULL; tmp = tmp->next)
        {
            ;
        }
        tmp->next = p;
        return q;
    }
}

/*******************************************************
 * PcbPtr pushPcb (PcbPtr headofQ, PcbPtr process)
 *    - push process back to the queue
 *    - process will become the head of headofQ
 *
 * returns head of queue
 ******************************************************/
PcbPtr pushPcb(PcbPtr q, PcbPtr p){
    if(!p){
        return q;
    }
    if (!q)
    {
        return p;
    }
    else{
        p->next = q;
        return p;
    }
}


/*******************************************************
 * PcbPtr deqPcb (PcbPtr * headofQ);
 *    - dequeue process - take Pcb from head of queue.
 *
 * returns:
 *    PcbPtr if dequeued,
 *    NULL if queue was empty
 *    & sets new head of Q pointer in adrs at 1st arg
 *******************************************************/
PcbPtr deqPcb(PcbPtr * hPtr)
{
    if (!hPtr || !(*hPtr))
    {
        return NULL;
    }
    else
    {
        PcbPtr tmp = *hPtr;
        *hPtr = (*hPtr)->next;
        tmp->next = NULL;
        return tmp;
    }
}

/*******************************************************
 * PcbPtr startPcb(PcbPtr process) - start (or restart)
 *    a process
 * returns:
 *    PcbPtr of process
 *    NULL if start (restart) failed
 ******************************************************/
PcbPtr startPcb (PcbPtr p)
{
    if (p->pid == 0)
    {
        switch (p->pid = fork())
        {
            case -1:
                fprintf(stderr, "FATAL: Could not fork process!\n");
                exit(EXIT_FAILURE);
            case 0:
                p->pid = getpid();
                p->status = PCB_RUNNING;
                printPcb(p);
                fflush(stdout);
                execv(p->args[0], p->args);
                fprintf(stderr, "ALERT: You should never see me!\n");
                exit(EXIT_FAILURE);
        }
    }
    else
    {
        printPcb(p);
        kill(p->pid, SIGCONT);
    }
    p->iter = p->iter+1;
    p->status = PCB_RUNNING;
    return p;
}

/*******************************************************
 * PcbPtr suspendPcb(PcbPtr process) - suspend
 *    a process
 * returns:
 *    PcbPtr of process
 *    NULL if suspension failed
 ******************************************************/
PcbPtr suspendPcb(PcbPtr p)
{
    if (!p){
        fprintf(stderr, "ERROR: Can not suspend a NULL process\n");
        return NULL;
    }else{
        kill(p->pid, SIGTSTP);
        waitpid(p->pid, NULL, WUNTRACED); // Ensure synchronization in dispatcher output
        p->status = PCB_SUSPENDED;
        return p;
    }
}

/*******************************************************
 * PcbPtr terminatePcb(PcbPtr process) - terminate
 *    a process
 * returns:
 *    PcbPtr of process
 *    NULL if termination failed
 ******************************************************/
PcbPtr terminatePcb(PcbPtr p)
{
    if (!p)
    {
        fprintf(stderr, "ERROR: Can not terminate a NULL process\n");
        return NULL;
    }
    else
    {
        kill(p->pid, SIGINT);
        puts("PLEASE REPLACE ME!"); // YOU NEED TO REPLACE THIS LINE WITH A WAITPID() CALL (ONE LINE ONLY)!
        p->status = PCB_TERMINATED;
        return p;
    }
}

/*******************************************************
 * PcbPtr printPcb(PcbPtr process)
 *  - print process attributes on stdout
 *  returns:
 *    PcbPtr of process
 ******************************************************/
PcbPtr printPcb(PcbPtr p)
{
    printPcbHdr();
    printf("%7d%7d%7d%11d%10d%5d%13d  ",
        (int) p->pid, p->arrival_time, p->service_time,
            p->remaining_cpu_time, p->priority,p->remaining_iter,p->iter);
    switch (p->status) {
        case PCB_UNINITIALIZED:
            printf("UNINITIALIZED");
            break;
        case PCB_INITIALIZED:
            printf("INITIALIZED");
            break;
        case PCB_READY:
            printf("READY");
            break;
        case PCB_RUNNING:
            printf("RUNNING");
            break;
        case PCB_SUSPENDED:
            printf("SUSPENDED");
            break;
        case PCB_TERMINATED:
            printf("PCB_TERMINATED");
            break;
        default:
            printf("UNKNOWN");
    }
    printf("\n");
    
    return p;     
}

/*******************************************************
 * void printPcbHdr() - print header for printPcb
 *  returns:
 *    void
 ******************************************************/
void printPcbHdr()
{  
    printf("    pid arrive service  remain_t  priority  remain_iter  iter        status\n");

}
