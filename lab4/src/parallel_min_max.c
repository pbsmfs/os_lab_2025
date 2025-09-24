#include <ctype.h>
#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>

#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>

#include <getopt.h>

#include "find_min_max.h"
#include "utils.h"

volatile sig_atomic_t timeout_reached = 0;

void timeout_handler(int sig) {
    timeout_reached = 1;
}

double get_elapsed_time(struct timeval *start, struct timeval *end) {
    double elapsed = (end->tv_sec - start->tv_sec) * 1000.0;
    elapsed += (end->tv_usec - start->tv_usec) / 1000.0;
    return elapsed;
}

int main(int argc, char **argv) {
  // Таймер 1: начало программы
  struct timeval program_start_time;
  gettimeofday(&program_start_time, NULL);

  int seed = -1;
  int array_size = -1;
  int pnum = -1;
  int timeout = 0;
  bool with_files = false;

  while (true) {
    int current_optind = optind ? optind : 1;

    static struct option options[] = {
        {"seed", required_argument, 0, 0},
        {"array_size", required_argument, 0, 0},
        {"pnum", required_argument, 0, 0},
        {"timeout", required_argument, 0, 0},
        {"by_files", no_argument, 0, 'f'},
        {0, 0, 0, 0}
    };

    int option_index = 0;
    int c = getopt_long(argc, argv, "f", options, &option_index);

    if (c == -1) break;

    switch (c) {
      case 0:
        switch (option_index) {
          case 0:
            seed = atoi(optarg);
            if (seed <= 0) {
                printf("seed must be a positive number\n");
                return 1;
            }
            break;
          case 1:
            array_size = atoi(optarg);
            if (array_size <= 0) {
                printf("array_size must be a positive number\n");
                return 1;
            }
            break;
          case 2:
            pnum = atoi(optarg);
            if (pnum <= 0) {
                printf("pnum must be a positive number\n");
                return 1;
            }
            break;
          case 3:
            timeout = atoi(optarg);
            if (timeout <= 0) {
                printf("timeout must be a positive number\n");
                return 1;
            }
            break;
          case 4:
            with_files = true;
            break;
          default:
            printf("Index %d is out of options\n", option_index);
        }
        break;
      case 'f':
        with_files = true;
        break;
      case '?':
        break;
      default:
        printf("getopt returned character code 0%o?\n", c);
    }
  }

  if (optind < argc) {
    printf("Has at least one no option argument\n");
    return 1;
  }

  if (seed == -1 || array_size == -1 || pnum == -1) {
    printf("Usage: %s --seed \"num\" --array_size \"num\" --pnum \"num\" [--timeout \"num\"] [--by_files]\n",
           argv[0]);
    return 1;
  }

  // Таймер 2: после парсинга аргументов, перед генерацией массива
  struct timeval before_array_generation;
  gettimeofday(&before_array_generation, NULL);
  double parsing_time = get_elapsed_time(&program_start_time, &before_array_generation);

  int *array = malloc(sizeof(int) * array_size);
  GenerateArray(array, array_size, seed);

  // Таймер 3: после генерации массива, перед созданием процессов
  struct timeval after_array_generation;
  gettimeofday(&after_array_generation, NULL);
  double array_generation_time = get_elapsed_time(&before_array_generation, &after_array_generation);
  
  // Выводим информацию о времени на подготовительных этапах
  printf("\n=== Preparation Timings ===\n");
  printf("Argument parsing time: %.3fms\n", parsing_time);
  printf("Array generation time: %.3fms\n", array_generation_time);
  printf("Total preparation time: %.3fms\n", parsing_time + array_generation_time);
  printf("Array size: %d, Processes: %d, Timeout: %ds\n", array_size, pnum, timeout);
  printf("===========================\n\n");

  // Создаем массивы для pipe или файлов
  int pipes[2 * pnum];
  char filenames[pnum][256];
  pid_t child_pids[pnum];
  
  if (!with_files) {
    for (int i = 0; i < pnum; i++) {
      if (pipe(pipes + 2*i) == -1) {
        printf("Pipe creation failed!\n");
        return 1;
      }
    }
  } else {
    for (int i = 0; i < pnum; i++) {
      snprintf(filenames[i], sizeof(filenames[i]), "min_max_%d.txt", i);
      unlink(filenames[i]);
    }
  }

  int active_child_processes = 0;

  // Таймер 4: начало параллельной обработки
  struct timeval parallel_start_time;
  gettimeofday(&parallel_start_time, NULL);

  // Устанавливаем обработчик сигнала таймаута
  if (timeout > 0) {
    signal(SIGALRM, timeout_handler);
    alarm(timeout);
  }

  int part_size = array_size / pnum;
  int remainder = array_size % pnum;

  // Запускаем дочерние процессы
  for (int i = 0; i < pnum; i++) {
    pid_t child_pid = fork();
    if (child_pid >= 0) {
      active_child_processes += 1;
      child_pids[i] = child_pid;
      
      if (child_pid == 0) {
        // Дочерний процесс
        if (timeout > 0) {
          signal(SIGALRM, SIG_IGN);
        }
        
        int start = i * part_size;
        int end = (i == pnum - 1) ? array_size : start + part_size;
        
        if (i == pnum - 1 && remainder > 0) {
          end = array_size;
        }

        struct MinMax local_min_max = GetMinMax(array, start, end);

        if (with_files) {
          FILE *file = fopen(filenames[i], "w");
          if (file == NULL) {
            printf("Failed to open file %s\n", filenames[i]);
            exit(1);
          }
          fprintf(file, "%d %d", local_min_max.min, local_min_max.max);
          fclose(file);
        } else {
          close(pipes[2*i]);
          int write_fd = pipes[2*i + 1];
          
          write(write_fd, &local_min_max.min, sizeof(int));
          write(write_fd, &local_min_max.max, sizeof(int));
          
          close(write_fd);
        }
        free(array);
        exit(0);
      }
    } else {
      printf("Fork failed!\n");
      return 1;
    }
  }

  // В родительском процессе закрываем ненужные концы pipe
  if (!with_files) {
    for (int i = 0; i < pnum; i++) {
      close(pipes[2*i + 1]);
    }
  }

  // Ожидаем завершения дочерних процессов с таймаутом
  int completed_processes = 0;
  while (active_child_processes > 0) {
    if (timeout > 0 && timeout_reached) {
      printf("Timeout reached! Sending SIGKILL to child processes...\n");
      for (int i = 0; i < pnum; i++) {
        if (child_pids[i] > 0) {
          kill(child_pids[i], SIGKILL);
        }
      }
      break;
    }
    
    int status;
    pid_t finished_pid = waitpid(-1, &status, WNOHANG);
    
    if (finished_pid > 0) {
      active_child_processes -= 1;
      completed_processes++;
      
      if (WIFEXITED(status)) {
        printf("Child process %d exited normally with status %d\n", 
               finished_pid, WEXITSTATUS(status));
	sleep(1000);
      } else if (WIFSIGNALED(status)) {
        printf("Child process %d terminated by signal %d\n", 
               finished_pid, WTERMSIG(status));
      }
    } else if (finished_pid == 0) {
      usleep(10000);
    } else {
      perror("waitpid failed");
      break;
    }
  }

  // Отменяем таймаут, если он еще не сработал
  if (timeout > 0) {
    alarm(0);
  }

  // Таймер 5: конец параллельной обработки
  struct timeval parallel_end_time;
  gettimeofday(&parallel_end_time, NULL);
  double parallel_time = get_elapsed_time(&parallel_start_time, &parallel_end_time);

  struct MinMax min_max;
  min_max.min = INT_MAX;
  min_max.max = INT_MIN;

  // Собираем результаты только от завершившихся процессов
  for (int i = 0; i < pnum; i++) {
    int min = INT_MAX;
    int max = INT_MIN;
    bool result_available = false;

    if (with_files) {
      FILE *file = fopen(filenames[i], "r");
      if (file != NULL) {
        if (fscanf(file, "%d %d", &min, &max) == 2) {
          result_available = true;
        }
        fclose(file);
        unlink(filenames[i]);
      }
    } else {
      fd_set readfds;
      struct timeval tv;
      
      FD_ZERO(&readfds);
      FD_SET(pipes[2*i], &readfds);
      tv.tv_sec = 0;
      tv.tv_usec = 0;
      
      if (select(pipes[2*i] + 1, &readfds, NULL, NULL, &tv) > 0) {
        if (FD_ISSET(pipes[2*i], &readfds)) {
          read(pipes[2*i], &min, sizeof(int));
          read(pipes[2*i], &max, sizeof(int));
          result_available = true;
        }
      }
      close(pipes[2*i]);
    }

    if (result_available) {
      if (min < min_max.min) min_max.min = min;
      if (max > min_max.max) min_max.max = max;
    }
  }

  // Таймер 6: конец программы
  struct timeval program_end_time;
  gettimeofday(&program_end_time, NULL);
  double results_processing_time = get_elapsed_time(&parallel_end_time, &program_end_time);
  double total_program_time = get_elapsed_time(&program_start_time, &program_end_time);

  free(array);

  // Выводим детальную информацию о времени выполнения
  printf("\n=== Detailed Timings ===\n");
  printf("1. Preparation stages: %.3fms\n", parsing_time + array_generation_time);
  printf("   - Argument parsing: %.3fms\n", parsing_time);
  printf("   - Array generation: %.3fms\n", array_generation_time);
  printf("2. Parallel processing: %.3fms\n", parallel_time);
  printf("3. Results processing: %.3fms\n", results_processing_time);
  printf("4. Total program time: %.3fms\n", total_program_time);
  printf("========================\n");

  printf("\n=== Results ===\n");
  printf("Min: %d\n", min_max.min);
  printf("Max: %d\n", min_max.max);
  printf("Completed processes: %d/%d\n", completed_processes, pnum);
  
  if (timeout_reached) {
    printf("Status: TIMEOUT (processes were killed after %d seconds)\n", timeout);
  } else {
    printf("Status: COMPLETED (all processes finished normally)\n");
  }
  
  // Эффективность параллельной обработки
  if (parallel_time > 0) {
    double efficiency = (array_generation_time + parallel_time) / total_program_time * 100;
    printf("Parallel efficiency: %.1f%%\n", efficiency);
  }
  
  fflush(NULL);
  return 0;
}
