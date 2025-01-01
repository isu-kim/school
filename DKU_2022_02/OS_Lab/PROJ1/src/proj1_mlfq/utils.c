//
// @file : utils.c
// @author : Isu Kim @ isu@isu.kim | Github @ https://github.com/isu-kim
// @brief : A file that implements utility functions, such as queue.
//


#include "utils.h"


/**
 * A function that initializes all queues.
 * @return 0 when successful, -1 when something failed.
 */
int init_queues() {
    printf("[+] Initializing queues.\n");
    queues = (struct pcb_t**) malloc(sizeof(struct pcb_t*) * queue_count);
    memset(queues, 0, sizeof(struct pcb_t*) * queue_count);

    /**
    for (int i = 0 ; i < queue_count ; i++) {
        struct pcb_t* tmp = malloc(sizeof(struct pcb_t));
        if (!tmp) return -1;
        memset(tmp, 0, sizeof(struct pcb_t));
        tmp->next = NULL;
        tmp->total_exec_time = 0;
        tmp->total_io_time = 0;
        tmp->pid = -1;
        queues[i] = tmp;
    }


     **/
    printf("[+] Initialized queues.\n");
    return 0;
}


/**
 * A function that frees all allocated queues.
 * OS is my garbage collector.
 */
void destroy_queues() {
    for (int i = 0 ; i < queue_count ; i++) {
        struct pcb_t* tmp = queues[i];
        while (tmp) {
            struct pcb_t* to_destroy = tmp;
            tmp = tmp->next;
            free(to_destroy);
        }
    }
    printf("[+] Destroyed queues.\n");
}


/**
 * A function that pops from a queue.
 * @param queue_type The index of queue array to pop data from. Refer to utils.h for queue index.
 * @return Popped pcb_t* value. If queue was empty, returns NULL.
 *         The popped value MUST be freed since it is a data in the heap region.
 */
struct pcb_t* pop_queue(int queue_type) {
    if (!queues[queue_type] || queues[queue_type] == 0) return NULL; // If this was empty, return NULL.
    struct pcb_t* tmp = queues[queue_type]; // Do a typical list job.
    queues[queue_type] = queues[queue_type]->next;
    return tmp;
}


/**
 * A function that pushes a pcb into designated queue.
 * @param queue_type The index of queue array to push data into. Refer to utils.h for queue index.
 * @param value The pcb_t value to push into.
 */
void push_queue(int queue_type, struct pcb_t value) {
    // Allocate and memset with 0.
    struct pcb_t* new = malloc(sizeof(struct pcb_t));
    memset(new, 0, sizeof(struct pcb_t));

    // Set values into the new pcb and do typical list job.
    struct pcb_t* tail = queues[queue_type]; // Find tail and insert.

    // Copy values into the new one.
    new->next = NULL;
    new->pid = value.pid;
    new->r_tq = value.r_tq;
    new->state = value.state;
    new->io_left = value.io_left;
    new->terminated_at = value.terminated_at;
    new->total_io_time = value.total_io_time;
    new->total_exec_time = value.total_exec_time;
    new->registered_at = value.registered_at;
    new->queue_stat = value.queue_stat;
    new->wait_time = value.wait_time;

    if (tail) {  // If the queue was not empty, just push.
        while (tail->next) { // Find tail and set next of tail as current. This is O(n) operation.
            tail = tail->next;
        }
        tail->next = new;
    } else { // If this was the first entry into the queue, set this as the start.
        queues[queue_type] = new;
    }
}


/**
 * A function that pushes element to the head of the queue.
 * This function is intended to be used for IO boost.
 * @param queue_type The index of queue array to push data into. Refer to Utils.h for queue index.
 * @param value The pcb_t value to push into.
 */
void push_queue_head(int queue_type, struct pcb_t value) {
    // Allocate and memset with 0.
    struct pcb_t* new = malloc(sizeof(struct pcb_t));
    memset(new, 0, sizeof(struct pcb_t));

    // Copy values into the new one.
    struct pcb_t* head = queues[queue_type];
    new->next = head;
    new->pid = value.pid;
    new->r_tq = value.r_tq;
    new->state = value.state;
    new->io_left = value.io_left;
    new->terminated_at = value.terminated_at;
    new->total_io_time = value.total_io_time;
    new->total_exec_time = value.total_exec_time;
    new->registered_at = value.registered_at;
    new->queue_stat = value.queue_stat;
    new->wait_time = value.wait_time;
    queues[queue_type] = new;
}


/**
 * A function that represents a queue into string.
 * @param queue_type The queue index to convert into string.
 * @param result The char* to store result into.
 */
void queue_to_str(int queue_type, char* result) {
    struct pcb_t* tmp = queues[queue_type];

    // Generate an empty string and memset 0.
    char* tmp_str = malloc(sizeof(char) * MAX_STRING);
    memset(tmp_str, 0, sizeof(char) * MAX_STRING);
    tmp_str[0] = '[';

    // Iterate over all list elements and concat pid.
    while (tmp) {
        char pid_str[MAX_STRING];
        if (tmp->pid) sprintf(pid_str, "%d|", tmp->pid); // Remove 0th one.
        strcat(tmp_str, pid_str);
        tmp = tmp->next;
    }

    tmp_str[strlen(tmp_str) > 1 ? strlen(tmp_str) - 1 : 1] = ']'; // Sometimes there might be only "["
    strcpy(result, tmp_str); // Dump generated str.
}


/**
 * A function that represents wait queue into string.
 * This is used since wait queue will be presented with a io duration left as well.
 * @param result The char* to store result into.
 */
void wait_queue_to_str(char* result) {
    struct pcb_t* tmp = queues[QUEUE_WAIT];

    // Generate an empty string and memset 0.
    char* tmp_str = malloc(sizeof(char) * MAX_STRING);
    memset(tmp_str, 0, sizeof(char) * MAX_STRING);
    tmp_str[0] = '[';

    // Iterate over all list elements and concat pid.
    while (tmp) {
        char pid_str[MAX_STRING];
        if (tmp->pid) sprintf(pid_str, "%d : %d|", tmp->pid, tmp->io_left); // Remove 0th one.
        strcat(tmp_str, pid_str);
        tmp = tmp->next;
    }

    tmp_str[strlen(tmp_str) > 1 ? strlen(tmp_str) - 1 : 1] = ']'; // Sometimes there might be only "["
    strcpy(result, tmp_str); // Dump generated str.
}


/**
 * A function that retrieves total elements in the queue.
 * @param queue_index The designated queue to look for.
 * @return The count of queue size.
 */
int get_queue_size(int queue_index) {
    struct pcb_t* tmp = queues[queue_index];
    int cnt = 0;
    while (tmp) {
        cnt++;
        tmp = tmp->next;
    }

    return cnt;
}


/**
 * A function that loads workload from file.
 * The workload must be in following format:
 *
 * # of queues
 * CPU burst duration in time ticks, -1 for random.
 * IO burst duration in time ticks, -1 for random.
 * Interval between processes in time ticks, -1 for random.
 * Scheduling algorithms, 0 for RR 1 for FIFO
 * Time quantum values, -1 for FIFO.
 *
 * Example:
 * 4
 * 10
 * 20
 * 5
 * 0/0/0/1/
 * 5/2/3/-1/
 *
 *
 * This will make an workload of
 * - Total 4 queues.
 * - With each process having 10 CPU burst time tick / 20 IO burst time tick
 * - Each processes have interval of 5 time tick after each other.
 * - 1st Queue having RR as scheduler.
 * - 2nd Queue having RR as scheduler.
 * - 3rd Queue having RR as scheduler.
 * - 4th Queue having FIFO as scheduler.
 *
 * @param file_name
 * @return
int load_workload(char* file_name) {
    FILE* fp = fopen(file_name, "r");
    if (!fp) return -1;

    char* read;
    fscanf(fp, "%d", &workload.number_of_queues);
    fscanf(fp, "%d", &workload.cpu_burst_duration);
    fscanf(fp, "%d", &workload.io_burst_duration);
    fscanf(fp, "%d", &workload.process_interval);

    workload.scheduler_of_queues = malloc(sizeof(int) * workload.number_of_queues);
    for (int i = 0 ; i < workload.number_of_queues ; i++)
        fscanf("%d/", &workload.scheduler_of_queues[i]);

}
  */
