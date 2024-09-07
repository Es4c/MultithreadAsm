#include "multi_level.h"

/**
 * return the header of the queue with the highest priority
 * it's assumed that the queues are placed in priority order.
*/
PcbPtr jobSelector(PcbPtr queues[],int size){
    for(int i=0;i<size;i++){
        if(queues[i] != NULL){
            return queues[i];
        }
    }
    printf("No more jobs in the queue.\n");
    return NULL;
}

int getQuantum(PcbPtr *proc,int o, int q){
    PcbPtr p = *proc;
    switch(p->priority){
        case LEVEL0:
            return o;
        case LEVEL1:
            return q;
        case LEVEL2:
            return 1;
        default:
            return -1;
    }
}

/**
 * return smaller param. If equal, return the 2nd param
 */
int min(int p, int q){
    return p < q ? p : q;
}

int main(int argc, char *argv[]){
    int timer = 0;
    int turnaround = 0;
    double avg_turnaround = 0.0, avg_wait = 0.0;
    int n = 0;
    int MAX_SIZE = 2048;
    int used_mem = 0;
     printf("--------------------\n");
    MabPtr root = createMab(MAX_SIZE,0,0);
    //1. initialize job queue, wait queue for mem alloc, and 3 levels 
    PcbPtr job_queue = NULL;
    PcbPtr wait_queue = NULL;
    PcbPtr queues[3] = {NULL};//[0] for level0, [1] for level1, and so on.
    int queue_size = sizeof(queues)/sizeof(PcbPtr); //avoid duplicated calculation
    PcbPtr current_process = NULL;
    PcbPtr candidate_process = NULL;
    PcbPtr process = NULL;
    int quantum;
   
    //2.Fill Job Dispatch queue from job dispatch list file;
    FILE * input_list_stream = NULL;
    if (argc <= 0)
    {
        fprintf(stderr, "FATAL: Bad arguments array\n");
        exit(EXIT_FAILURE);
    }
    else if (argc != 2)
    {
        fprintf(stderr, "Usage: %s <TESTFILE>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    if (!(input_list_stream = fopen(argv[1], "r")))
    {
        fprintf(stderr, "ERROR: Could not open \"%s\"\n", argv[1]);
        exit(EXIT_FAILURE);
    }

    int t0;
    printf("Please enter the time_quantum for level 0 fcfs queue: ");
    scanf("%d", &t0);

    int t1;
    printf("Please enter the time_quantum for level 1 round robin queue: ");
    scanf("%d", &t1);

    int k;
    printf("Please enter maximum number of iterations for a job to stay in the Level-1 queue: ");
    scanf("%d", &k);

    if(t0 < 1 || t1 < 1 || k < 1){
        printf("All of them should be >=1 !\n");
        exit(EXIT_SUCCESS);
    }


    // put processes into fcfs_queue
    while (!feof(input_list_stream)) {  
        process = createnullPcb();
        if (fscanf(input_list_stream,"%d, %d, %d",
             &(process->arrival_time), 
             &(process->service_time), &(process->mab_size)) != 3)  {
            free(process);
            continue;
        }
        if(process->mab_size>MAX_SIZE){
            free(process);
            continue;
        }
	    process->remaining_cpu_time = process->service_time;
        process->remaining_iter = 1;
        process->status = PCB_INITIALIZED;
        job_queue = enqPcb(job_queue, process);
        n++;
    }
    fclose(input_list_stream);

    while (job_queue || wait_queue || current_process || candidate_process){
        printf("timer: %d\n",timer);
        quantum = 1;
        //5.1 Unload any arrived pending processes from the Job Dispatch queue dequeue process from Job Dispatch queue and enqueue on LEVEL0 queue;
        while (job_queue && job_queue->arrival_time <= timer) {
            process = deqPcb(&job_queue);          // dequeue process
            process->status = PCB_READY;            // set pcb ready
            wait_queue = enqPcb(wait_queue, process);
        }
        
        while (wait_queue){
            printf("Allocating memory for %d with size of %d\n", wait_queue->pid,wait_queue->mab_size);
            MabPtr z = memAlloc(root, wait_queue->mab_size);
            if(z){
                wait_queue->mab = memSplit(z,wait_queue->mab_size);
                if(wait_queue->mab){
                    used_mem+=wait_queue->mab->size;
                    printf("Allocate success! We have %d memory left\n", MAX_SIZE-used_mem);
                    printf("Passing into level 0...\n");
                }
                
                queues[0] = enqPcb(queues[0], deqPcb(&wait_queue));
            }else{
                printf("Allocate failed! Continue to wait\n");
                printf("Not enough memory for new task\n");
                break;
            }
        }

        candidate_process = jobSelector(queues,queue_size);
        if(!current_process && candidate_process){
            current_process = deqPcb(&queues[candidate_process->priority]);
            startPcb(current_process);
        }

        
        if (current_process) {
            // ">" is because enum is from small number to large number
            //so higher priority should have smaller actual value
            if(candidate_process &&(current_process->priority > candidate_process->priority)){ //pre-empted by higher-priority process
                suspendPcb(current_process);
                queues[current_process->priority] = pushPcb(queues[current_process->priority], current_process);
                current_process = deqPcb(&queues[candidate_process->priority]);
                startPcb(current_process);
                
            }

            quantum = min(current_process->remaining_cpu_time, getQuantum(&current_process, t0, t1));
            
            current_process->remaining_cpu_time -= quantum;
            current_process->remaining_iter -=1;
            sleep(quantum);
            timer+=quantum;
        
            if (current_process->remaining_cpu_time <= 0) {
                turnaround = timer - current_process->arrival_time;
                avg_turnaround += turnaround;
                avg_wait += turnaround - current_process->service_time;
                printf ("pid: %d, arrived at %d,turnaround time = %d, waiting time = %d\n",current_process->pid, current_process->arrival_time, turnaround,turnaround - current_process->service_time);
                terminatePcb(current_process);
                printf("%d\n",current_process->mab->allocated);
                memFree(current_process->mab);
                used_mem -= current_process->mab->size;
                printf("Freed %d memory, we have %d memory left\n",current_process->mab->size, MAX_SIZE-used_mem);
                memMerge(root);
                free(current_process);
                current_process = NULL;
            }else{
                switch(current_process->priority){
                    case LEVEL0:
                        if(current_process->remaining_iter == 0){
                            suspendPcb(current_process);
                            current_process->priority = LEVEL1;
                            current_process->remaining_iter = k;
                            queues[1] = enqPcb(queues[1],current_process);
                            current_process = NULL;
                        }
                        break;
                    case LEVEL1:
                        if(current_process->remaining_iter == 0){
                            suspendPcb(current_process);
                            current_process->priority = LEVEL2;
                            current_process->remaining_iter = -1;
                            queues[2] = enqPcb(queues[2],current_process);
                        }else{
                            queues[1] = enqPcb(queues[1],current_process);
                        }
                        current_process = NULL;
                        break;
                    case LEVEL2:
                        break;
                    default:
                        printf("The priority is not undefined!");
                        break;
                }
            }

            
        }else{
            sleep(1);
            timer += 1;
        }
 
    }

    avg_turnaround = avg_turnaround / n;
    avg_wait = avg_wait / n;
    printf("average turnaround time = %f\n", avg_turnaround);
    printf("average wait time = %f\n", avg_wait);
    memFree(root);
    memMerge(root);
    free(root);
    //Terminate the Round Robin dispatcher
    exit(EXIT_SUCCESS);
    return 0;
}