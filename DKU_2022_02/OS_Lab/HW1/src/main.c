//
// @file : main.c
// @author : Isu Kim @ dev.gooday2die@gmail.com | Github @ https://github.com/gooday2die
// @brief : A file that implements main function and all its friends.
//

#include <stdio.h>
#include <stdbool.h>

#include "utils.h"
#include "sish.h"


/**
 * The entry point of this process.
 * @return 0 if successful, otherwise some error code.
 */
int main(int argc, char* argv[]) {
    bool devMode = false;

    if ((argc != 1) && !strcmp(argv[1], "iamdev")) { // Check if argument "iamdev" was set.
        printf("[+] Debug mode on!\n");
        devMode = true;
    }

    printBanner(); // Print our banner

    if(geteuid() == 0) {
        printf("Be careful! You are running SiSH as root!\n");
    }

    int len = 0;
    const char **paths = getPaths(&len);

    if (devMode) {
	for (int i = 0 ; i < len - 1 ; i++) printf("%s\n", paths[i]);
    }

    printf("[+] Found %d PATH environments.\n", len - 1);
    runSiSH(paths, len - 1);

    printBye(); // Print goodbye.
    return 0;
}
