#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <string.h>

int main(int argc, char *argv[]) {
    
    int seed = -1;
    int array_size = -1;
    
    if (argc == 3) {
        seed = atoi(argv[1]);
        array_size = atoi(argv[2]);
    } 
    else {
        printf("Usage:\n");
        printf("  %s <seed> <array_size>\n", argv[0]);
        printf("Example: %s 123 1000\n", argv[0]);
        return 1;
    }
    
    if (seed == -1 || array_size == -1) {
        printf("Error: invalid arguments\n");
        return 1;
    }

    int pipefd[2];
    if (pipe(pipefd) == -1) {
        perror("pipe failed");
        return 1;
    }

    pid_t pid = fork();
    
    if (pid == -1) {
        perror("fork failed");
        return 1;
    }
    else if (pid == 0) {
        close(pipefd[0]);
        
        dup2(pipefd[1], STDOUT_FILENO);
        close(pipefd[1]);
        
        char seed_str[20], size_str[20];
        snprintf(seed_str, sizeof(seed_str), "%d", seed);
        snprintf(size_str, sizeof(size_str), "%d", array_size);
        
        char *args[] = {
            "./sequential_min_max",
            seed_str,  
            size_str,   
            NULL
        };
        
        printf("Executing: %s %s %s\n", args[0], args[1], args[2]);
        execvp(args[0], args);
        perror("execvp failed");
        exit(1);
    }
    else {
        close(pipefd[1]); 
        
        printf("Output from sequential_min_max:\n");
        printf("===============================\n");
        
        char buffer[1024];
        ssize_t bytes_read;
        while ((bytes_read = read(pipefd[0], buffer, sizeof(buffer) - 1)) > 0) {
            buffer[bytes_read] = '\0';
            printf("%s", buffer);
        }
        
        close(pipefd[0]);
        
        int status;
        waitpid(pid, &status, 0);
        
        printf("===============================\n");
        if (WIFEXITED(status)) {
            printf("Child process exited with status: %d\n", WEXITSTATUS(status));
        }
        
        return 0;
    }
}
