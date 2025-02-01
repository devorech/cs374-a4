#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <string.h>

/*
A program with a pipeline of 3 threads that interact with each other as producers and consumers.
- Input thread is the first thread in the pipeline. It gets input from the user and puts it in a buffer it shares with the next thread in the pipeline.
- Square root thread is the second thread in the pipeline. It consumes items from the buffer it shares with the input thread. It computes the square root of this item. It puts the computed value in a buffer it shares with the next thread in the pipeline. Thus this thread implements both consumer and producer functionalities.
- Output thread is the third thread in the pipeline. It consumes items from the buffer it shares with the square root thread and prints the items.

*/

// Size of the buffers
#define BUF_SIZE 50000    // 50000 = 50 lines max, 1000 characters max each line

// Max number of lines that the user can input
#define NUM_LINES 50

// Max size for each line
#define LINE_SIZE 1000

// Buffer 1, shared resource between input thread and square-root thread
char buffer_1[BUF_SIZE];
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


// Buffer 2, shared resource between line seperator thread and 
char buffer_2[BUF_SIZE];
// Number of items in the buffer
int count_2 = 0;
// Index where the square-root thread will put the next item
int write_idx_2 = 0;
// Index where the output thread will pick up the next item
int read_idx_2 = 0;
// Initialize the mutex for buffer 2
pthread_mutex_t mutex_2 = PTHREAD_MUTEX_INITIALIZER;
// Initialize the condition variable for buffer 2
pthread_cond_t full_2 = PTHREAD_COND_INITIALIZER;


// Buffer 3, shared resource between square root thread and output thread
char buffer_3[BUF_SIZE];
// Number of items in the buffer
int count_3 = 0;
// Index where the square-root thread will put the next item
int write_idx_3 = 0;
// Index where the output thread will pick up the next item
int read_idx_3 = 0;
// Initialize the mutex for buffer 2
pthread_mutex_t mutex_3 = PTHREAD_MUTEX_INITIALIZER;
// Initialize the condition variable for buffer 2
pthread_cond_t full_3 = PTHREAD_COND_INITIALIZER;


// Initialize the trackers to kill each threads (default = 0, will be equal to 1 when a STOP is read)
int stop_input = 0;
int stop_line_seperator = 0;
int stop_plus_sign = 0;


/*
Get input from the user.
This function doesn't perform any error checking.
*/
char* get_user_input()
{
    char* input = (char*) calloc(1000, sizeof(char));
    //printf("Enter your string here: ");
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
    strcpy(&buffer_1[write_idx_1], item);
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
    
    // Exit thread if the user enters/passes through STOP
    if (strcmp(item, "STOP\n") == 0)
    {
      //printf("Thread 1 ending!\n");
      stop_input = 1; // signal input thread has stopped
      pthread_exit(NULL);
    }
  }
  return NULL;
}

/*
Get the next item from buffer 1
*/
char* get_buff_1()
{
  // Lock the mutex before checking if the buffer has data
  pthread_mutex_lock(&mutex_1);
  while (count_1 == 0) {
      // Buffer is empty. Wait for the producer to signal that the buffer has data
      pthread_cond_wait(&full_1, &mutex_1);
  }
  size_t chars_to_read = (size_t)(write_idx_1 - read_idx_1);
  //printf("\nRead index: %d, Write index: %d, Chars to read: %d\n", read_idx_1, write_idx_1, chars_to_read);
  char* item = calloc(chars_to_read, sizeof(char));
  strncpy(item, &buffer_1[read_idx_1], chars_to_read);
  // Increment the index from which the item will be picked up
  read_idx_1 = write_idx_1;
  count_1--;
  // Unlock the mutex
  pthread_mutex_unlock(&mutex_1);
  // Return the item
  //printf("\nItem read: %sNumber of chars read:", item);
  return item;
}

/*
Replace all line seperators in the function.
In this case, replace \n with a space (" ")
*/
void replace_ls(char* item)
{
  // Replace /n (which falls at the end of the string) with a space (" ")
  // Count all occurences of \n in the item string
  char *temp = calloc(strlen(item), sizeof(char));
  
  // Keep finding and replacing all instances of the \n in the item while they exist
  while (strstr(item, "\n") != NULL)
  {
    // Back up the current line (so that we can add the part after the \n after)
    strcpy(temp, item);

    // Get the \n and everything after it of the first instance \n found
    char* curr_spot = strstr(item, "\n");

    // Get the spot/index that the first \n starts at
    int i = curr_spot - item;

    // End the item string (temporarily) at the \n
    item[i] = '\0';

    // Concatenate the blank space at the end (replaces the \n)
    strcat(item, " ");

    // Add back the rest of the string after the \n
    strcat(item, temp + i + 1); // + 1 signifies move forward once to start after the \n
  }

  // Free the temp string storage variable
  free(temp);
}

/*
 Put an item in buff_2
*/
void put_buff_2(char* item){
  // Lock the mutex before putting the item in the buffer
  pthread_mutex_lock(&mutex_2);
  // Put the item in the buffer
  strcpy(&buffer_2[write_idx_2], item);
  // Increment the index where the next item will be put.
  write_idx_2 = write_idx_2 + strlen(item);
  count_2++;
  // Signal to the consumer that the buffer is no longer empty
  pthread_cond_signal(&full_2);
  // Unlock the mutex
  pthread_mutex_unlock(&mutex_2);
} 

/*
  THREAD 2:
  Function that the line seperatror thread will run. 
  Consume an item from the buffer shared with the input thread.
  Replace the line seperator (\n) with a space (" ")
  Produce an item in the buffer shared with the output thread.
*/
void *seperate_line(void *args)
{
  for (int i = 0; i < NUM_LINES; i++)
  {
    char* item = get_buff_1(); // item is dynamically allocated data
    replace_ls(item);
    put_buff_2(item);
    //printf("Buffer 2: %s\n", buffer_2);
    //printf("STOPing is equal to %d\n", stop_threads);
    // Exit thread if the user enters/passes through STOP
    if (stop_input == 1)
    {
      //printf("Thread 2 ending!\n");
      stop_line_seperator = 1; // signal line seperator has stopped
      pthread_exit(NULL);
    }
  }
  return NULL;
}

/*
Get the next item from buffer 2
*/
char* get_buff_2(){
  // Lock the mutex before checking if the buffer has data
  pthread_mutex_lock(&mutex_2);
  while (count_2 == 0) {
      // Buffer is empty. Wait for the producer to signal that the buffer has data
      pthread_cond_wait(&full_2, &mutex_2);
  }
  size_t chars_to_read = (size_t)(write_idx_2 - read_idx_2);
  char* item2 = calloc(chars_to_read, sizeof(char));
  strncpy(item2, &buffer_2[read_idx_2], chars_to_read);
  // Increment the index from which the item will be picked up
  read_idx_2 = write_idx_2;
  count_2--;
  // Unlock the mutex
  pthread_mutex_unlock(&mutex_2);
  // Return the item
  //printf("\nItem read: %s\n", item);
  return item2;
}

/*
 Replaces all instances of ++ in the given string with a caret (^)
*/
void replace_plus_with_caret(char* item)
{
  // Count all occurences of ++ in the input string
  char* temp = calloc(strlen(item), sizeof(char));
  
  // Keep finding and replacing all instances of the ++ in the item while they exist
  while (strstr(item, "++") != NULL)
  {
      // Back up the current line (so that we can add the part after the ++ after)
      strcpy(temp, item);

      // Get the ++ and everything after it of the first instance ++ found
      char* curr_spot = strstr(item, "++");

      // Get the spot/index that the first ++ starts at
      int i = curr_spot - item;

      // End the string (temporarily) before the ++
      item[i] = '\0';

      // Concatenate the ^ with the current new string
      strcat(item, "^");

      // Add back the rest of the string after the ++
      strcat(item, temp + i + 2); // + 2 signifies move forward twice to start after the ++
  }
}

/*
 Put an item in buff_3
*/
void put_buff_3(char* item){
    // Lock the mutex before putting the item in the buffer
    pthread_mutex_lock(&mutex_3);
    // Put the item in the buffer
    strncpy(&buffer_3[write_idx_3], item, strlen(item));
    // Increment the index where the next item will be put.
    write_idx_3 = write_idx_3 + strlen(item);
    count_3++;
    // Signal to the consumer that the buffer is no longer empty
    pthread_cond_signal(&full_3);
    // Unlock the mutex
    pthread_mutex_unlock(&mutex_3);
}

/*
 THREAD 3:
 Function that the plus sign thread will run.
 Consume an item from the buffer shared with the input thread.
 Replace all instances of a "++" with a "^"
 Produce an item in the buffer shared with the output thread.
*/
void *replace_plusplus(void *args)
{
  for (int i = 0; i < NUM_LINES; i++)
  {
    char* item = get_buff_2();
    replace_plus_with_caret(item);
    put_buff_3(item);
    // Exit thread if the user enters/passes through STOP
    if (stop_line_seperator == 1)
    {
      //printf("Thread 3 ending!\n");
      stop_plus_sign = 1; // plus sign has stopped
      pthread_exit(NULL);
    }
  }

  return NULL;
}

/*
* Get an item from buff 3
*/
char* get_buff_3()
{
  // Lock the mutex before checking if the buffer has data
  pthread_mutex_lock(&mutex_3);
  while (count_3 == 0) {
      // Buffer is empty. Wait for the producer to signal that the buffer has data
      pthread_cond_wait(&full_3, &mutex_3);
  }
  size_t chars_to_read = (size_t)(write_idx_3 - read_idx_3);
  //printf("\nRead index: %d, Write index: %d, Chars to read: %d\n", read_idx_3, write_idx_3, chars_to_read);
  char* item3 = calloc(chars_to_read, sizeof(char));
  strncpy(item3, &buffer_3[read_idx_3], chars_to_read);
  // Increment the index from which the item will be picked up
  read_idx_3 = write_idx_3;
  count_3--;
  // Unlock the mutex
  pthread_mutex_unlock(&mutex_3);
  // Return the item
  //printf("\nItem read: %s", item3);
  return item3;
}


/*
 THREAD 4:
 Function that the output thread will run. 
 Consume an item from the buffer shared with the replace ++ thread.
 Print the item.
*/
void *write_output(void *args)
{
  char output[BUF_SIZE];
  int ri = 0; // index that was last read from
  int wi = 0; // index that is currently being wrote at in the buffer
  for (int i = 0; i < NUM_LINES; i++)
  {
    //printf("Before - Write index: %d\n", wi);
    char* item = get_buff_3();
    strcat(output, item);
    // Exit thread if the user enters/passes through STOP

    wi += strlen(item);
    //printf("After - Write index: %d\n", wi);

    // Check if there are more than 80 characters to read
    while ((wi - ri) > 79)
    {
      char* to_print = calloc(80, sizeof(char));
      strncpy(to_print, &output[ri], 80); 
      //printf("In Output Buffer: %s\n", output);
      printf("%s\n", to_print);
      ri += 80;
      //printf("New read index: %d\n", ri);
    }

    if (stop_plus_sign == 1)
    {
      //printf("Thread 4 ending!\n");
      pthread_exit(NULL);
    } 
  }
  return NULL;
}

int main()
{
    srand(time(0));
    pthread_t input_t, line_seperator_t, plus_sign_t, output_t;

    // Create the threads
    pthread_create(&input_t, NULL, get_input, NULL);
    pthread_create(&line_seperator_t, NULL, seperate_line, NULL);
    pthread_create(&plus_sign_t, NULL, replace_plusplus, NULL);
    pthread_create(&output_t, NULL, write_output, NULL);

    // Wait for the threads to terminate
    pthread_join(input_t, NULL);
    pthread_join(line_seperator_t, NULL);
    pthread_join(plus_sign_t, NULL);
    pthread_join(output_t, NULL);

    return EXIT_SUCCESS;
}