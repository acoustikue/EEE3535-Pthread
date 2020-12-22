#include <pthread.h>
#include "mandelbrot.h"

// Mandelbrot convergence map
unsigned mandelbrot[resolution*resolution];

// Lock and cond variables for thread_exit() and thread_join()
pthread_mutex_t lock;
pthread_cond_t  cond;

// Author: SukJoon Oh, 2018142216
// acoustikue@yonsei.ac.kr
int thread_alive;

// This must be called at the end of thread function
void thread_exit(void) {
    pthread_mutex_lock(&lock);
    thread_alive--;
    pthread_mutex_unlock(&lock);

    pthread_cond_signal(&cond); // Send signal
}

// This is called by the main thread.
void thread_join(void) {
    pthread_mutex_lock(&lock);
    while (thread_alive != 0) { pthread_cond_wait(&cond, &lock); }
    pthread_mutex_unlock(&lock);
}


// 
// Structure for tossing the parameters to each thread.
class Worker {
protected:
    unsigned idx; // starting index
    unsigned tag;

public:
    pthread_t thread;

    Worker(unsigned argidx = 0, unsigned argtag = 0) : idx(argidx), tag(argtag) {}
    
    // Setter & Getter, Interface
    void setIdx(int argidx) { this->idx = argidx < 0 ? 0 : argidx; }
    void setTag(int argtag) { this->tag = argtag < 0 ? 0 : argtag; }

    unsigned getIdx() { return idx; }
    unsigned getTag() { return tag; }
};

// Thread function to calculate a part of Mandelbrot set.
void* thread_mandelbrot(void* arg) {

    Worker* worker = (Worker*)arg;

    // Initialize objects.
    // The initial values are meaningless in this case. 
    Complex Z(0, 0);
    Complex C(0, 0);

    for (register int idx = worker->getTag(); 
        idx < int(worker->getIdx()); idx += num_threads) {

        Z = C = Complex(
            minW + 3.2 * int(idx % resolution) / resolution,
            minH + 3.1 * int(idx / resolution) / resolution
            ); // Initialize them with its appropriate range

        for (register int itor = 0; 
            itor < max_iterations && Z.magnitude2() < escape; itor++) {
            Z = Z * Z + C; // Definition
            mandelbrot[idx]++; // Increase the value. 
                // It has initial zeros.
        }
    }
    
    thread_exit();  // Wake up a thread waiting on the condition variable.
    return 0;
}

// Calculate the Mandelbrot convergence map.
void calc_mandelbrot(void) {

    // Initialize locks and condition variables
    pthread_mutex_init(&lock, NULL);
    pthread_cond_init(&cond, NULL);

    // Thread information
    Worker* worker = new Worker[num_threads];
    thread_alive = 0; // Initialize with zero.

    for (unsigned i = 0; i < num_threads; i++) {
        worker[i].setTag(i);
        worker[i].setIdx(resolution * resolution);
        pthread_create(&worker[i].thread, NULL, &thread_mandelbrot, (void*)&worker[i] );

        pthread_mutex_lock(&lock);
        thread_alive++; // Increase since a thread is created.
        pthread_mutex_unlock(&lock);
    }

    
    thread_join();  // Check the condition variable.

    pthread_mutex_destroy(&lock);
    pthread_cond_destroy(&cond);
    delete[] worker;
}

