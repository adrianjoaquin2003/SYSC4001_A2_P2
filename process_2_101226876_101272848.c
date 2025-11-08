#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
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

    int shmid = shmget(SHM_KEY, sizeof(struct shared_data), IPC_PRIVATE);

    struct shared_data *shared = (struct shared_data *)shmat(shmid, NULL, 0);

    int semid = semget(SEM_KEY, 1, IPC_PRIVATE);

    // Wait until counter > 100
    while (1) {
        sem_lock(semid);
        int c = shared->counter;
        sem_unlock(semid);
        if (c > 100) break;
        printf("[P2] Waiting... counter=%d\n", c);
        sleep(1);
    }

    while (1) {
        sem_lock(semid);

        int c = shared->counter;
        int m = shared->multiple;

        if (c % m == 0)
            printf("[P2] Counter = %d is multiple of %d\n", c, m);

        sem_unlock(semid);

        if (c > 500) break;
        sleep(1);
    }

    shmdt(shared);
    printf("[P2] Exiting.\n");
    return 0;
}
