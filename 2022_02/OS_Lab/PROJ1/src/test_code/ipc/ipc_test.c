#include <stdio.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <errno.h>



struct msgbuf {  // Struct to save values
	long mtype;
	int value;
};

pid_t child_pid; // For storing child pid.
int msgq; // For storing message queue's id.


/**
 * A function that is for closing everything.
 * This will remove message queue and kill child process.
 * @param signum: This will be SIGINT, but will be ignored.
 */
void quitHandler(int signum) {
	printf("[P] Killing child and removing message queue\n");
	kill(child_pid, SIGINT); // Kill child process.
	msgctl(msgq, IPC_RMID, NULL); // Clearn IPC message queue with msgctl.
	exit(0);
}

/**
 * A function that is for parent.
 * This will iterate and will put data into the IPC messsage queue.
 * Once it is done putting value into message queue, it will send SIGUSR1 into child process to let it know
 * that there is a data put into the messge queue.
 */
void parent() {
	printf("[P] Generated\n");
	int i = 0;
	signal(SIGINT, quitHandler); // Register quitHandler as SIGINT's callback function

	while(1) {
		printf("[p] Enter to send message\n");
		getchar();    

		// Generate new msgbuf.
		struct msgbuf msg;
		memset(&msg, 0, sizeof(msg));
		msg.value = i;
		msg.mtype = 1;

		int res = msgsnd(msgq, &msg, sizeof(msg), IPC_NOWAIT); // msgsend with IPC_NOWAIT
		if (res == -1) printf("[P] %s\n", strerror(errno));

		kill(child_pid, SIGUSR1); // Send signal into child process
		printf("[p] MSGQ : %d / Value : %d / Sent signal to %d\n", msgq, i, child_pid);
		i++;
	}
}


/** 
 * A function that is for handling SIGUSR1.
 * This will get IPC message queue data and display the value in the screen.
 * @param signum: This is always SIGUSR1, will be ignored.
 */
void signalHandler(int signum) {
	printf("[C] Got signal\n");
	
	struct msgbuf msg;
	memset(&msg, 0, sizeof(msg));
	
	int res = msgrcv(msgq, &msg, sizeof(msg), 1, 0);
	if (res == -1) printf("[C] %s\n", strerror(errno));
	printf("[C] Got %d bytes / value : %d\n", res, msg.value);
	return;
}

/**
 * A function that is for child process.
 * This will register SIGUSR1's callback function as signalHandler.
 */
void child() {
	printf("[C] My PID : %d / MSGQ : %d\n", getpid(), msgq);
	signal(SIGUSR1, (void*) signalHandler); // Register signal handler's callback.
	while(1) {
		printf("[C] Waiting for signal\n");
		pause(); // Wait for SIGUSR1
	}
}

int main(void) {
	int ret;
	int key = 0x12345;
	// Generate message queue
	msgq = msgget(key, 0666 | IPC_CREAT);
	printf("[+] msgq id : %d\n", msgq);

	pid_t result = fork();
	if (result == 0) { // I am child
		child();
	} else if (result < 0) { // I fucked up.
		printf("[+] For failed\n");
		exit(0);
	} else {
		// I am parent.
		child_pid = result;
		printf("[P] Child PID : %d\n", child_pid);
		parent();
	}
	return 0;
}
