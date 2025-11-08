#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>

#define SHM_KEY 1234
#define SEM_KEY 5678

struct shared_data {
    int multiple;
    int counter;
};

void sem_lock(int semid) {
    struct sembuf op = {0, -1, 0};
    semop(semid, &op, 1);
}

void sem_unlock(int semid) {
    struct sembuf op = {0, 1, 0};
    semop(semid, &op, 1);
}

int main() {
    setvbuf(stdout, NULL, _IONBF, 0);

    // Create shared memory
    int shmid = shmget(SHM_KEY, sizeof(struct shared_data), IPC_PRIVATE);

    struct shared_data *shared = (struct shared_data *)shmat(shmid, NULL, 0);

    // Create semaphore
    int sem_id = semget(SEM_KEY, 1, IPC_PRIVATE);

    // Initialize semaphore to 1
    semctl(sem_id, 0, SETVAL, 1);

    // Initialize shared memory
    shared->multiple = 3;
    shared->counter = 0;

    pid_t pid = fork();

    if (pid == 0) {
        execl("./process_2/process_2", "process_2", NULL);
        perror("execl failed");
        exit(1);
    }

    // Parent process
    while (1) {
        for (int i = 0; i < 3; i++) {
            sem_lock(sem_id);

            shared->counter++;
            int c = shared->counter;
            int m = shared->multiple;

            printf("[P1] Cycle %d | Counter = %d", i, c);
            if (c % m == 0)
                printf(" - %d is a multiple of %d", c, m);
            printf("\n");

            sem_unlock(sem_id);

            if (c > 500) break;
            sleep(1);
        }
        if (shared->counter > 500) break;
    }

    // Wait for child
    wait(NULL);

    // Cleanup
    shmdt(shared);
    shmctl(shmid, IPC_RMID, NULL);
    semctl(sem_id, 0, IPC_RMID);

    printf("[P1] Cleaned up and exiting.\n");
    return 0;
}
