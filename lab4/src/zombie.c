#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <string.h>

void demonstrate_zombie() {
    printf("\n=== Демонстрация зомби-процессов ===\n");
    
    pid_t pid = fork();
    
    if (pid == 0) {
        printf("Дочерний процесс: PID = %d, PPID = %d\n", getpid(), getppid());
        printf("Дочерний процесс завершается, но родитель не вызывает wait()...\n");
        exit(0);
    } else if (pid > 0) {
        printf("Родительский процесс: PID = %d, создал дочерний с PID = %d\n", getpid(), pid);
        printf("Родительский процесс продолжает работу 10 секунд...\n");
        printf("В это время дочерний процесс станет зомби\n");
        
        sleep(10);
        
        printf("Родительский процесс завершается...\n");
    } else {
        perror("fork failed");
        exit(1);
    }
}

int main() {
    demonstrate_zombie();
    return 0;
}
