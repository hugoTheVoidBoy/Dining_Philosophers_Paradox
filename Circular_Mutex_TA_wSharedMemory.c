#include <stdio.h> 
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <stdbool.h>
#include <string.h>
#include <sys/wait.h>
#include <pthread.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>

#define STUDENT_COUNT 20
#define SEM_KEY 1234
#define SHM_KEY 5678
#define TA_COUNT 5
#define REPEAT_COUNT 3

typedef struct {
    int id;               // TA ID
    int current_student;  // Current student index
    int cycles;           // Number of completed cycles
    bool finished;        // Whether the TA is done marking
} TA;

// Shared data structure
typedef struct {
    int student_list[STUDENT_COUNT];
    int current_index;
} SharedData;

typedef struct {
    volatile int value;   // Semaphore value
    pthread_mutex_t lock;
} semaphore;

// Semaphore initialization
void semaphore_init(semaphore *sem, int initial_value) {
    sem->value = initial_value;
    pthread_mutex_init(&sem->lock, NULL);
}

// Semaphore wait (P operation)
void semaphore_wait(semaphore *sem) {
    while (1) {
        pthread_mutex_lock(&sem->lock);
        if (sem->value > 0) {
            sem->value--;
            pthread_mutex_unlock(&sem->lock);
            return; // Exit once we acquire the semaphore
        }
        pthread_mutex_unlock(&sem->lock);
        sched_yield(); // Allows other threads/processes to run
    }
}

// Semaphore signal (V operation)
void semaphore_signal(semaphore *sem) {
    pthread_mutex_lock(&sem->lock);
    sem->value++;
    pthread_mutex_unlock(&sem->lock);
}

// Helper function to create a student file
SharedData *create_student_file() {
    
    int shm_id = shmget(IPC_PRIVATE, sizeof(SharedData), IPC_CREAT | 0666);
    if (shm_id < 0) {
        perror("Failed to create shared memory");
        exit(1);
    }

    // Attach shared memory
    SharedData *shared_data = (SharedData *)shmat(shm_id, NULL, 0);
    if (shared_data == (void *)-1) {
        perror("Failed to attach shared memory");
        exit(1);
    }

    // Initialize shared memory
    for (int i = 0; i < STUDENT_COUNT - 1; i++) {
        shared_data->student_list[i] = i + 1; // Student IDs
    }
    shared_data->student_list[STUDENT_COUNT - 1] = 9999; // End marker
    shared_data->current_index = 0;
    
    return shared_data;
}

// Sleep for a random duration and print activity
void sleep_random(int max_time, int ta_id, const char *activity) {
    int delay = rand() % max_time + 1;
    printf("TA%d is %s (delay: %d seconds).\n", ta_id, activity, delay);
    sleep(delay);
}


// Save marked student data to a file
void save_to_file(int ta_id, int student, int mark) {
    char filename[16];
    sprintf(filename, "TA%d.txt", ta_id);
    
    FILE *file = fopen(filename, "a");
    if (!file) {
        perror("Failed to open TA file");
        exit(1);
    }

    fprintf(file, "Student: %04d, Mark: %d\n", student, mark);
    fclose(file);
}
void marking(int ta_id, int student){
    // Mark the student
    int mark = rand() % 11; // Random mark between 0 and 10
    save_to_file(ta_id, student, mark);

    char marking[50]; 
    sprintf(marking, "marking student %d", student);
    // Simulate marking
    sleep_random(8, ta_id, marking);
}
// Shared data
semaphore sem[TA_COUNT];           // Custom semaphores

int main() {


    // Create student shared data
    SharedData *shared_data = create_student_file();

    // Initialize custom semaphores
    for (int i = 0; i < TA_COUNT; i++) {
        semaphore_init(&sem[i], 1);  // Initialize each TA's semaphore
    }      
    

    // Initialize TA array
    TA tas[TA_COUNT];
    for (int i = 0; i < TA_COUNT; i++) {
        tas[i].id = i + 1;
        tas[i].current_student = 0;
        tas[i].cycles = 0;
        tas[i].finished = false;
    }

    // Fork processes for TAs
    for (int i = 0; i < TA_COUNT; i++) {

        if (fork() == 0) {
            // Child process (TA)
            TA *ta = &tas[i];
            srand(time(NULL) ^ getpid()); // Seed random number generator
            
            while (!ta->finished) {

                // Lock semaphores
                semaphore_wait(&sem[ta->id - 1]);
                semaphore_wait(&sem[ta->id % TA_COUNT]); // (j+1) mod TA_COUNT
                // Simulate accessing the database
                printf("Semaphores %d and %d is locked. ",ta->id, (ta->id) % (TA_COUNT)+1);
                sleep_random(4, ta->id, "accessing the database");

                // Access the shared memory
                int student = shared_data->student_list[ta->current_student];
                
                if (student == 9999) {
                    ta->current_student = 0; // Reset index to start
                    ta->cycles++;
                    if (ta->cycles == REPEAT_COUNT) {
                        ta->finished = true;
                        marking(ta->id, student);
                        semaphore_signal(&sem[ta->id - 1]);
                        semaphore_signal(&sem[ta->id % TA_COUNT]); // (j+1) mod TA_COUNT
                        break; // Exit the marking loop
                    }
                } else {
                    ta->current_student++; // Move to the next student    
                }

                // Release semaphores

                semaphore_signal(&sem[ta->id - 1]);
                semaphore_signal(&sem[ta->id % TA_COUNT]); // (j+1) mod TA_COUNT
                printf("Release Semaphores %d and %d. ",ta->id, (ta->id) % (TA_COUNT)+1);
                marking(ta->id, student);
            }

            printf("TA%d finished marking.\n", ta->id);
            exit(0); // Exit child process
        }
    }

    // Wait for all child processes to finish
    for (int i = 0; i < TA_COUNT; i++) {
        wait(NULL);
    }

    // Detach and remove shared memory
    shmdt(shared_data);


    printf("All TAs have finished marking.\n");
    return 0;
}

