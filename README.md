# Dining_Philosophers_Paradox
Concurrent database-access TA grading simulator - based on the Dining Philosophers paradox. 
 
**Overview**
This program simulates a concurrent system where multiple Teaching Assistants (TAs) mark student assignments. It consists of shared memory, semaphores, and process synchronization in C. Each TA acquires semaphores of his/her own and the one one right, marks a list of students, releases the semaphores and records their scores in separate files, and ensures safe access to shared resources.

**Features**
-_Shared Memory_: Stores a list of student IDs accessible to all TAs.
-_Custom Semaphores_: Implements synchronization to prevent race conditions.
-_Concurrent Execution_: Multiple TA processes operate independently.
-_Simulated Realism_: Random sleep delays to mimic marking and database access times.
-_Cycle Control_: TAs repeat the marking process for a predefined number of cycles.

**File Descriptions**
-_Circular_Mutex_TA.c_: Main program source file containing logic for initialization, process creation, and marking from an external list of students.
-_Circular_Mutex_TA_wSharedMemory.c_: Main program source file containing logic for initialization, process creation, and marking, using a hardcoded list of students.
-_TA Output Files_: Each TA writes their marked student data to TA<ID>.txt (e.g., TA1.txt).


**System Libraries**
-pthread.h: For semaphore implementation.
-sys/ipc.h, sys/shm.h, sys/sem.h: For shared memory and semaphores.
-unistd.h: For process control and sleep functions.
-stdio.h, stdlib.h, time.h, stdbool.h, string.h: Standard utilities.


How to Compile
Use gcc to compile the program:
```bash
gcc Circular_Mutex_TA.c -o ta_marking_sim -lpthread
./ta_marking_sim

gcc Circular_Mutex_TA_wSharedMemory.c -o ta_marking_sim_sm -lpthread
./ta_marking_sim_sm

```

**The program will**
-Initialize shared memory and semaphores.
-Fork TA_COUNT concurrent child processes.
-Simulate marking assignments and save the results.
-Output files (TA<ID>.txt) will be created in the current directory, containing marked student data.


**Expected Output**
-Console: Displays each TA's activity, such as accessing the database, marking students, and releasing semaphores.
-Files: TA<ID>.txt files will contain student IDs and their assigned marks.
