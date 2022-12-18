//
// @file : sish.c
// @author : Isu Kim @ dev.gooday2die@gmail.com | Github @ https://github.com/gooday2die
// @brief : A file that implements all functions for SiSH.
//

#include "sish.h"
pid_t curPID = 0;

/**
 * TODO:
 * - add error messages and permissions and things (ex normal using accessing root's files)
 * - signal sending.
 */

/**
 * A function that handles signal.
 * With the signal that was fired, this will send signal into the child process.
 * @param sig The signal that was caught by signal
 */
void signalHandler(int sig) {
    printf("\n\n[+] GOT SIGNAL : %s\n", strsignal(sig));
    if (curPID != 0) kill(curPID, SIGINT); // send SIGINT to the child if it has one
}

/**
 * A function that runs SiSH.
 * @param paths All paths that PATH environment stores. This is the range to look for the binary file
 * @param pathCount The total count of PATH environments.
 * @return returns 0 if successfully exited, other codes if result was non-zero.
 */
int runSiSH(const char** paths, int pathCount) {
    printf("[+] Initializing SiSH...\n");

    // Find out hostname
    // Yes, getenv("HOSTNAME") works, but sometimes it turns out as NULL.
    // Thus, we are using gethostname function for getting host name.
    char hostName[1024]; // Max hostName length is 1024.
    int result = gethostname(hostName, 1024);

    struct sigaction act;
    act.sa_handler = signalHandler;  // set signal handler as signalHandler function
    signal(SIGINT, signalHandler); // catch SIGINT
    signal(SIGTSTP, signalHandler); // catch SIGTSTP

    printf("[+] Init Done!\n");

    while (1) {
        char command[MAX_COMMAND_LEN]; // Total buffer of user input is length of 100.

        // Get CWD for printing out in prompt options.
        char cwd[MAX_CWD_LEN];
        getcwd(cwd, sizeof(cwd));

        // Print prompt options.
        printf("%s@%s:~%s", getenv("USER"), hostName, cwd);
        if (!strcmp(getenv("USER"), "root")) printf("# ");
        else printf("$ ");

        // I hate using scanf. That is too buggy, so for getting user input, we will use fgets instead of scanf.
        // Check https://dev.to/berviantoleo/safe-way-to-handle-user-input-in-c-2e5l for handling input with fgets.
        if(fgets (command, MAX_COMMAND_LEN, stdin) != NULL) {
            // Remove new line from the user input since fgets includes a new line
            // Check https://stackoverflow.com/questions/2693776/removing-trailing-newline-character-from-fgets-input
            command[strcspn(command, "\n")] = 0;
            // Now parse command and execute the command.
            if (!strcmp("quit", command)) break; // If requested command was "quit", exit process.
	    else if (!strcmp("exit", command)) printf("[-] Use quit instead\n");
            else if (strlen(command) == 0) ; // If input was empty, do nothing.
            else { // Execute and check if there is any error.
                int result = processCommand(command, paths, pathCount);
                if (result) printError(result);
		curPID = 0;
            }
        }
    }
    return 0;
}

/**
 * A function that processes command.
 * This will parse command then execute command then execute command.
 * @param command The command string to process.
 * @param paths The PATH variables.
 * @param pathCount The total count of PATH variables.
 * @return Following integer values:
 *         0 if operation was successful,
 *         errno values when cd failed. (https://man7.org/linux/man-pages/man3/errno.3.html)
 *         wait return values.
 */
int processCommand(const char* command, const char** paths, int pathCount) {
    int parseCount = 0;
    const char** parsed = parseCommand(&parseCount, command);
    int response; // For storing results.

    pid_t* childPID;

    // Try executing direct binary file.
    response = processDirectBinary(parsed, childPID);
    if (response != -99) return response;

    // Try executing special commands.
    response = processSpecialCommands(parseCount, parsed);
    if (response != -99) return response;

    // If this was not a special command, just find binary and execute.
    response = executePathBinary(parsed, pathCount, paths, childPID);
    return response;
}

/**
 * A function that figures out which binary file to use.
 * This function works like "which" command in POSIX system.
 * For example, this function will find out that "ls" is actually /usr/bin/ls.
 * This will return the fastest index of binary that was found.
 * For example, if we had multiple binary files named "ls", this will return the fastest "ls" that was found.
 * Check Limitations at README.md.
 * @param command The command to look binary file for.
 * @param paths All paths that PATH environment stores. This is the range to look for the binary file
 * @param pathCount The total count of PATH environments.
 * @return A string that represents path to original binary file. NULL if binary was not found.
 */
const char* findWhichBin(const char* command, const char** paths, int pathCount) {
    char* tmpBin; // The string that represents current binary file.
    char* curPath; // The string that stores current path.

    if (paths[0] == NULL) {
        printf("[-] PATH environment is empty. Please check your PC settings.");
        exit(-1);
    }

    int count = 0;
    curPath = strdup(paths[count]);

    // Iterate over all paths and find out which one includes the command
    // I miss iterator from C++ :(
    for (int i = 0 ; i < pathCount ; i++) {
        tmpBin = strcat(curPath, "/"); // Generate full path.
        tmpBin = strcat(tmpBin, strdup(command)); // Generate full path.

        // Check if binary file exists
        // https://stackoverflow.com/questions/230062/whats-the-best-way-to-check-if-a-file-exists-in-c
        if (access(tmpBin, F_OK) == 0) {
            return tmpBin; // When the binary was found, return the path to binary.
        }

        curPath = strdup(paths[i]); // Update curPath
    }

    return NULL; // When the binary was not found.
}


/**
 * A function that checks if current command was direct execution of binary file.
 * This will check if file exists and is executable. Then execute with arguments.
 * For example, this function is for executing ./a.out, /foo/bar/a.out
 * @param parsed The array of string that stores parsed command.
 * @return -99 if this was not a file.
 *         0 if successfully executed.
 *         -1 if file was not executable.
 *         errno if execution of binary file failed.
 */
int processDirectBinary(const char** parsed, pid_t* childPID) {
    // Check if the file exists and is able to execute the binary file.
    if (access(parsed[0], F_OK) == 0) {
        //if (access(parsed[0], X_OK) != 0) return -1; // When file was not executable.
        printf("[+] Executing %s\n", parsed[0]);
        return execute(parsed[0], (char**) parsed, childPID); // Execute code.
    } else return -99;
}


/**
 * For SiSH, there are some special commands.
 * This function will find out if this was a special command or not.
 * The list of special commands are:
 * - cd
 * - clear
 * @param parseCount The total count of parsed elements.
 * @param parsed The array of string for parsed ones.
 * @return -99 if this was not a special command
 *         0 if successfully executed.
 *         errno if execution failed.
 */
int processSpecialCommands(int parseCount, const char** parsed) {
    if (!strcmp(parsed[0], "cd")) {
        // Since cd command is not a binary, this needs to be processed using chdir function.
        // This will work for pwd and all directory related features.
        // Usage of cd will be "cd /foo/bar"
        if (parseCount == 3) {
            int result = chdir(parsed[1]);
            if (!result) { // If result was successful
                //printf("[+] Changed current directory to : %s\n", parsed[1]);
                return 0;
            } else { // If failed, return errno for failing.
                printf("[-] cd failed : %s\n", strerror(errno));
                return errno;
            }
        } else printf("[-] Usage: cd /foo/bar\n");
    } else if (!strcmp(parsed[0], "clear")) {
        // Original clear will fail since TERM environment variable is not set.
        // So, this will print out new lines for CLEAR_NEW_LINES times.
        // Yes, this is stupid :b
        for (int i = 0 ; i < CLEAR_NEW_LINES ; i++) printf("\n");
        return 0;
    }
    return -99;
}


/**
 * A function that executes path binary.
 * For example, this is for "ls".
 * This will automatically find ls as "/usr/bin/ls" and execute it with arguments.
 * @param parsed The array of string for arguments.
 * @param pathCount The total count of PATH variables.
 * @param paths The array of string that represents PATH.
 * @return 0 if successfully executed.
 *         -1 if not found binary was not executable.
 *         -99 if command was not found.
 *         errno if execution failed.
 */
int executePathBinary(const char** parsed, int pathCount, const char** paths, pid_t* childPID) {
    const char* bin = findWhichBin(parsed[0], paths, pathCount);

    if (bin == NULL) return -99; // When there was no binary found.
    //if (access(bin, X_OK) != 0) return -1; // When file was not executable, return -1.
    return execute(bin, (char**) parsed, childPID);
}


/**
 * A function that executes binary file using fork and execve.
 * This function will:
 * 1. Fork process
 * 2. For child, execve with arguments provided.
 * 3. For parent, wait for child to die.
 * @param bin The path to binary file that needs to be executed.
 * @param args The array of strings for arguments, this is NULL terminated.
 * @return 0 if executed successfully
 *         errno on error if execve failed to execute something.
 */
int execute(const char* bin, char** args, pid_t* childPID) {
    pid_t pid;
    pid = fork(); // Fork right now.
    int result;

    struct sigaction old_sa;
    struct sigaction new_sa;

    if (pid == -1) { // If fork failed
        printf("[-] fork failed.\n");
    } else if (pid == 0) { // If this was child process, execve.
        // printf("[+] Child process was generated, PID : %d\n", getpid()); // For debug purpose
        // *childPID = getpid();
	// curPID = getpid();

        // execve will NEVER return a value if executed successfully.
        // Thus, if result is set other than 0, this means something went wrong.
        result = execve(bin, args, NULL);

        // If execve failed to execute, the following printf will be called.
        // This will let user know something went wrong.
        printf("[-] Execution failed : %s\n", strerror(errno));
        exit(0);
    } else { // If this was parent.
        wait(0); // Wait for the child to die.
        return errno;
    }
}

/**
 * A simple function that prints out error message if there was an error.
 * @param errCode The error code to print error.
 */
void printError(int errCode) {
    switch (errCode) {
        case 0: // This means the execution was successful.
            return;
        case -1: // This means the file was not executable.
            printf("[-] File is not executable.\n");
            break;
        case -99:
            printf("[-] Binary or command not found.\n");
            break;
        default: // For default cases, do nothing.
            break;
    }
}
