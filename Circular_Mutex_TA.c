#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <stdbool.h>
#include <string.h>
#include <sys/wait.h>
#include <pthread.h>

#define STUDENT_FILE "database.txt"
#define TA_COUNT 5
#define REPEAT_COUNT 3

typedef struct {
    int id;               // TA ID
    int current_student;  // Current student index
    int cycles;           // Number of completed cycles
    bool finished;        // Whether the TA is done marking
} TA;

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
            sem->value--;  // Decrement semaphore value
            pthread_mutex_unlock(&sem->lock);
            return;
        }
        pthread_mutex_unlock(&sem->lock);
        sched_yield();  // Yield the CPU to allow other threads/processes to run
    }
}

// Semaphore signal (V operation)
void semaphore_signal(semaphore *sem) {
    pthread_mutex_lock(&sem->lock);  // Lock the mutex to safely increment the semaphore value
    sem->value++;  // Increment semaphore value
    pthread_mutex_unlock(&sem->lock);  // Unlock the mutex
}

// Helper function to create a student file
void create_student_file() {
    FILE *file = fopen(STUDENT_FILE, "w");
    if (!file) {
        perror("Failed to create student file");
        exit(1);
    }

    for (int i = 1; i <= 19; i++) {
        fprintf(file, "%04d\n", i);
    }
    fprintf(file, "9999\n");
    fclose(file);
}

// Sleep for a random duration and print activity
void sleep_random(int max_time, int ta_id, const char *activity) {  
    int delay = rand() % max_time + 1;
    printf("TA%d is %s (%d seconds).\n", ta_id, activity, delay);
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
void marking(int ta_id, const char *student){
    // Mark the student
    int mark = rand() % 11; // Random mark between 0 and 10
    save_to_file(ta_id, atoi(student), mark);
    char marking[50]; 
    sprintf(marking, "marking student %s", student);
    // Simulate marking
    sleep_random(8, ta_id, marking);
    printf("TA%d finished marking student %s.\n", ta_id, student);
}
// Shared data
volatile int current_index = 0; // Shared index for the student list
semaphore sem[TA_COUNT];           // Custom semaphores

int main() {

    // Create student file
    create_student_file();

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
            srand(time(NULL) ^ getpid()); // Seed random number generator
            
            TA *ta = &tas[i];

            while (!ta->finished) {

                // Lock semaphores
                semaphore_wait(&sem[ta->id - 1]);
                semaphore_wait(&sem[ta->id % TA_COUNT]); // (j+1) mod TA_COUNT
                // Simulate accessing the database
                printf("Semaphores %d and %d is locked. ",ta->id, (ta->id) % (TA_COUNT)+1);
                sleep_random(4, ta->id, "accessing the database");
                
                // Access the database
                FILE *db = fopen(STUDENT_FILE, "r");
                if (!db) {
                    perror("Failed to open student file");
                    exit(1);
                }

                fseek(db, current_index * 5, SEEK_SET); // 5 = 4 digits + newline
                char student[5];
                fscanf(db, "%s", student);
                fclose(db);

                if (strcmp(student, "9999") == 0) {
                    current_index = 0; // Reset index to start
                    ta->cycles++;
                    if (ta->cycles == REPEAT_COUNT) {
                        ta->finished = true;
                        marking(ta->id, student);
                        semaphore_signal(&sem[ta->id - 1]);
                        semaphore_signal(&sem[ta->id % TA_COUNT]);
                        
                        break; // Exit the marking loop
                    }
                } else {
                    current_index++; // Move to the next student
                } 
                    
                // Release semaphores
                semaphore_signal(&sem[ta->id % TA_COUNT]);
                semaphore_signal(&sem[ta->id - 1]);
                
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

    printf("All TAs have finished marking.\n");
    return 0;
}

