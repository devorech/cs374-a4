#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <math.h> // must link with -lm
#include <string.h>

/*
A program with a pipeline of 3 threads that interact with each other as producers and consumers.
- Input thread is the first thread in the pipeline. It gets input from the user and puts it in a buffer it shares with the next thread in the pipeline.
- Square root thread is the second thread in the pipeline. It consumes items from the buffer it shares with the input thread. It computes the square root of this item. It puts the computed value in a buffer it shares with the next thread in the pipeline. Thus this thread implements both consumer and producer functionalities.
- Output thread is the third thread in the pipeline. It consumes items from the buffer it shares with the square root thread and prints the items.

*/

// Size of the buffers
#define SIZE 50000    // 50000 = 50 lines max, 1000 characters max each line

// Max number of lines that the user can input
#define NUM_LINES 50

// Buffer 1, shared resource between input thread and square-root thread
char buffer_1[SIZE];
// Number of items in the buffer
int count_1 = 0;
// Index where the input thread will put the next item
int write_idx_1 = 0;
// Index where the square-root thread will pick up the next item
int read_idx_1 = 0;
// Initialize the mutex for buffer 1
pthread_mutex_t mutex_1 = PTHREAD_MUTEX_INITIALIZER;
// Initialize the condition variable for buffer 1
pthread_cond_t full_1 = PTHREAD_COND_INITIALIZER;

/*
// Buffer 2, shared resource between square root thread and output thread
double buffer_2[SIZE];
// Number of items in the buffer
int count_2 = 0;
// Index where the square-root thread will put the next item
int prod_idx_2 = 0;
// Index where the output thread will pick up the next item
int con_idx_2 = 0;
// Initialize the mutex for buffer 2
pthread_mutex_t mutex_2 = PTHREAD_MUTEX_INITIALIZER;
// Initialize the condition variable for buffer 2
pthread_cond_t full_2 = PTHREAD_COND_INITIALIZER; */


// NOTES:
// read index and write index (only need a read index for buffer 1 and 2)
// checking whether there is 80 characters is done in the output thread
//  - check if difference between the read and write indeces is >= 80


/*
Get input from the user.
This function doesn't perform any error checking.
*/
char* get_user_input()
{
    char* input = (char*) calloc(1000, sizeof(char));
    printf("Enter your string here: ");
    fgets(input, 1000, stdin);
    return input;
}

/*
 Put an item in buff_1
*/
void put_buff_1(char* item){
    // Lock the mutex before putting the item in the buffer
    pthread_mutex_lock(&mutex_1);
    // Put the item in the buffer
    strncpy(&buffer_1[write_idx_1], item, strlen(item));
    // Increment the index where the next item will be put.
    write_idx_1 = write_idx_1 + strlen(item);
    count_1++;
    // Signal to the consumer that the buffer is no longer empty
    pthread_cond_signal(&full_1);
    // Unlock the mutex
    pthread_mutex_unlock(&mutex_1);
}

/*
 THREAD 1: 
 Function that the input thread will run.
 Get input from the user.
 Put the item in the buffer shared with the square_root thread.
*/
void *get_input(void *args)
{
    for (int i = 0; i < NUM_LINES; i++)
    {
        // Get the user input
        char* item = get_user_input();
        put_buff_1(item);
        printf("Buffer 1 Text: \n%s", buffer_1);
    }
    return NULL;
}

/*
Get the next item from buffer 1
*/
char* get_buff_1(){
    // Lock the mutex before checking if the buffer has data
    pthread_mutex_lock(&mutex_1);
    while (count_1 == 0) {
        // Buffer is empty. Wait for the producer to signal that the buffer has data
        pthread_cond_wait(&full_1, &mutex_1);
    }
    /*
    * FIX NEXT LINE: NEEDS TO READ FROM THE READ INDEX UNTIL THE END
    */
    //char* item = (char*) buffer_1[read_idx_1];
    size_t chars_to_read = write_idx_1 - read_idx_1;
    char* item = strncpy(item, &buffer_1[read_idx_1], chars_to_read);
    // Increment the index from which the item will be picked up
    read_idx_1 = read_idx_1 + strlen(item);
    count_1--;
    // Unlock the mutex
    pthread_mutex_unlock(&mutex_1);
    // Return the item
    return item;
}

/*
*
 Put an item in buff_2
*
void put_buff_2(double item){
  // Lock the mutex before putting the item in the buffer
  pthread_mutex_lock(&mutex_2);
  // Put the item in the buffer
  buffer_2[prod_idx_2] = item;
  // Increment the index where the next item will be put.
  prod_idx_2 = prod_idx_2 + 1;
  count_2++;
  // Signal to the consumer that the buffer is no longer empty
  pthread_cond_signal(&full_2);
  // Unlock the mutex
  pthread_mutex_unlock(&mutex_2);
}

*
 Function that the square root thread will run. 
 Consume an item from the buffer shared with the input thread.
 Compute the square root of the item.
 Produce an item in the buffer shared with the output thread.

*
void *compute_square_root(void *args)
{
    int item = 0;
    double square_root;
    for (int i = 0; i < NUM_ITEMS; i++)
    {
      item = get_buff_1();
      square_root = sqrt(item);
      put_buff_2(square_root);
    }
    return NULL;
}

*
Get the next item from buffer 2
*
double get_buff_2(){
  // Lock the mutex before checking if the buffer has data
  pthread_mutex_lock(&mutex_2);
  while (count_2 == 0)
    // Buffer is empty. Wait for the producer to signal that the buffer has data
    pthread_cond_wait(&full_2, &mutex_2);
  double item = buffer_2[con_idx_2];
  // Increment the index from which the item will be picked up
  con_idx_2 = con_idx_2 + 1;
  count_2--;
  // Unlock the mutex
  pthread_mutex_unlock(&mutex_2);
  // Return the item
  return item;
}


*
 Function that the output thread will run. 
 Consume an item from the buffer shared with the square root thread.
 Print the item.
*
void *write_output(void *args)
{
    double item;
    for (int i = 0; i < NUM_ITEMS; i++)
    {
      item = get_buff_2();
      printf("\nOutput: %.4f\n", item);
    }
    return NULL;
}
*/

int main()
{
    srand(time(0));
    pthread_t input_t; /*, square_root_t, output_t*/

    // Create the threads
    pthread_create(&input_t, NULL, get_input, NULL);
    //pthread_create(&square_root_t, NULL, compute_square_root, NULL);
    //pthread_create(&output_t, NULL, write_output, NULL);

    // Wait for the threads to terminate
    pthread_join(input_t, NULL);
    //pthread_join(square_root_t, NULL);
    //pthread_join(output_t, NULL);

    return EXIT_SUCCESS;
}