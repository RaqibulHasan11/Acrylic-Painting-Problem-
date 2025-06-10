#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>
#include <sys/wait.h>
#include <sys/shm.h>
#include <string.h>

#define NUM_ARTISTS 4
#define SHM_KEY 0x1234
#define SHM_SIZE 1024

sem_t artist_sems[NUM_ARTISTS];

pthread_mutex_t progress_mutex;

char *progress_report;

void* artist_work(void* arg) {
    int id = *(int*)arg;
    sem_wait(&artist_sems[id]);

    pthread_mutex_lock(&progress_mutex);
    printf("Artist %d started painting their section.\n", id + 1);
    sprintf(progress_report + strlen(progress_report), "Artist %d started painting.\n", id + 1);
    pthread_mutex_unlock(&progress_mutex);

    sleep(2);

    pthread_mutex_lock(&progress_mutex);
    printf("Artist %d finished painting. Waiting for paint to dry...\n", id + 1);
    sprintf(progress_report + strlen(progress_report), "Artist %d finished painting.\n", id + 1);
    pthread_mutex_unlock(&progress_mutex);

    sleep(1);

    if (id + 1 < NUM_ARTISTS) {
        sem_post(&artist_sems[id + 1]);
    }

    pthread_exit(NULL);
}

void preparation_stage() {
    printf(" Preparation stage started...\n");
    sleep(2);
    printf(" Preparation complete.\n");
}

// Painting phase using threads
void painting_stage() {
    printf(" Painting stage started...\n");

    pthread_t artists[NUM_ARTISTS];
    int artist_ids[NUM_ARTISTS];

    pthread_mutex_init(&progress_mutex, NULL);

    for (int i = 0; i < NUM_ARTISTS; i++) {
        sem_init(&artist_sems[i], 0, 0);
    }


    sem_post(&artist_sems[0]);


    for (int i = 0; i < NUM_ARTISTS; i++) {
        artist_ids[i] = i;
        pthread_create(&artists[i], NULL, artist_work, &artist_ids[i]);
    }

    for (int i = 0; i < NUM_ARTISTS; i++) {
        pthread_join(artists[i], NULL);
    }


    for (int i = 0; i < NUM_ARTISTS; i++) {
        sem_destroy(&artist_sems[i]);
    }

    pthread_mutex_destroy(&progress_mutex);
    printf(" Painting stage completed.\n");
}

void finishing_stage() {
    printf(" Finishing touches stage started...\n");
    sleep(2);
    printf(" Finishing complete.\n");
}

int main() {
    int shm_id;
    pid_t pid;

    shm_id = shmget(SHM_KEY, SHM_SIZE, IPC_CREAT | 0666);


    progress_report = shmat(shm_id, NULL, 0);
    strcpy(progress_report, "Progress Report:\n");

    pid = fork();
    if (pid == 0) {
        preparation_stage();
        exit(0);
    }
    wait(NULL);

    pid = fork();
    if (pid == 0) {
        painting_stage();
        exit(0);
    }
    wait(NULL);
    pid = fork();
    if (pid == 0) {
        finishing_stage();
        exit(0);
    }
    wait(NULL);
    printf("\n%s\n", progress_report);

    shmdt(progress_report);
    shmctl(shm_id, IPC_RMID, NULL);

    printf("Collaborative acrylic painting project completed successfully!\n");
    return 0;
}
