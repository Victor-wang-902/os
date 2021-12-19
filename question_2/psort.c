#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>

typedef struct args { // type define for pass an argument to thread functions. as im operating on the original array, it's important to keep track of the positions so that the threads don't mess each other up
    int *start; // start of the array to be sorted by the thread
    int *end; // end of the array to be sorted by the thread
    int *mid; //where the function starts to sort
} args;

void swap(int *a, int *b){ // utility function for swapping the values of two pointers
    int temp;
    temp = *a;
    *a = *b;
    *b = temp;
}

void *sort(void *input){ // sort function used directly by the threads. inputs are args type which contains the arguments we'd like to use.
    int *start = ((struct args *)input)->start; // it's important to keep the global positional arguments unchanged here
    int *end = ((struct args *)input)->end;
    int *ptr = ((struct args *)input)->mid; // pointer for iterating through the whole array
    int *cur_ptr; // pointers for doing swapping within the sorted region
    int *temp; 
    if (start == end) 
        return NULL;
    while (ptr != end){
        ptr++; // increment first: the last comparison can be made
        temp = ptr;
        cur_ptr = ptr;
        while (temp != start) {
            temp--; // decrement first: comparison with the first element can be made
            if (*cur_ptr < *temp){ // compare with the sorted array
                swap(cur_ptr, temp); // if smaller then swap
                cur_ptr = temp;
                continue;
            }
            break;
        }
    }
    return NULL;
}


int main(int argc, char *argv[])
{
    FILE *fd; // file or stdin stream
    char *buf = (char *)malloc(512 * sizeof(char)); // dynamic buffer that determines the size and accommodates the entirety of the file
    int size = 0; // current size of the buffer
    int capacity = 512; // max capacity of the buffer
    int n; // actual bytes read
    int count = 0;  // actual interger count
    char *ptr; // utility char pointer
    char temp_buf[512]; // temp buffer for putting an integer in
    char *temp; // utility pointer for temp buffer
    int *array; // array to put the integers in
    int *arr_ptr; // utility array pointer
    int *start; // indicates the start of the array
    int *mid1; // indicates the end of first part of the array
    int *mid2; // indicates the start of the second part of the array
    int *end; // indicates the end of the array
    pthread_t thread_id1; // tid for first thread
    pthread_t thread_id2; // tid for second thread
    args *arg; // define a args type pointer for passing argument
    if (argc < 1 || argc > 2){ // either 1 or 2 args
        printf("invalid argument(s)");
        return 0;
    }
    else if (argc == 1) { // read from pipe redirect
        fd = stdin;
    }
    else {
        fd = fopen(argv[1], "r"); // read from file
    }
    
    if (fd == NULL){ 
      printf("cannot open %s.\n", argv[3]);
      return 0;
    }
    // this part is for dynamic buffer allocation and file reading
    ptr = buf + size;
    while ((n = fread(ptr, 1, 512, fd)) >= 0){ // read 512 bytes a time
        size += n; // update the current size 
        if (size >= capacity - 1){ // check if current capacity is enough
            buf = realloc(buf, 2 * capacity); // if not reallocate twice the size
            ptr = buf + size; // put ptr to the end of the string for more read or EOF
            capacity *= 2; // update capacity
        }
        else{
            ptr += n; // if so just update the pointer
        }
        if (!n){ // if reached the end of file quit the loop
            break;
        }
    }
    *ptr = 0; // set EOF

/* this is the method i tried before, it worked on other compilers but not here. the reason was that fseek is not supported by the compiler here
    char len_buf[1024];
    int sum;
    char * buf;
    while ((n = fread(len_buf, 1, 1024, fd)) > 0) {
        sum += n;
    }
    sum += 1;
    buf = (char *)malloc(sum * sizeof(char));
    fseek(fd, 0,SEEK_SET);
    fread(buf, 1, sum, fd);
    if (buf[sum -1] != 0){
        printf("error");
        return 0;
    }
*/
    
    for (ptr = buf; *ptr != 0; ptr++){ // count how many intergers there are in the file
        if (*ptr == ' ') // space + 1
            count += 1;
    }
    count += 1;
    array = (int *)malloc(sizeof(int) * count); // allocate memory for array of the exact size of the number of the integers
    temp = temp_buf; //set up the temp buffer for storing individual integers
    *temp = 0;
    arr_ptr = array;
    for (ptr = buf; *ptr != 0; ptr ++){ 
        if (*ptr == ' '){ // if a space is read, convert the char in the temp buffer to an integer and write to the array
            *temp = 0; // set EOF
            *arr_ptr = atoi(temp_buf); // convert
            memset(temp_buf,0,sizeof(temp_buf)); //clearing up
            temp = temp_buf;
            arr_ptr++; // inc the array pointer
        }
        else{
            *temp = *ptr; // if not, read the current integer into the temp buffer
            if (temp == &temp_buf[511]) { // assume that the length of an integer is smaller than 512.
                printf("number too large");
                return 0;
            }
            temp++; // inc the temp pointer
        }
        *temp = 0; // In the last interation, we have one more integer in the buffer to write
        *arr_ptr = atoi(temp_buf); // convert and write the last integer
    }
    start = array; // set up the positional pointer
    mid1 = start + count / 2 - 1;
    mid2 = mid1 + 1;
    end = start + count - 1;
    //struct args *arg1 = (struct args *)malloc(sizeof(struct args)); // alternative method for passing the argument
    //arg1->start = start;
    //arg1->end = mid1;
    //arg1->mid = start;
    //struct args *arg2 = (struct args *)malloc(sizeof(struct args));    
    //arg2->start = mid2;
    //arg2->end = end;
    //arg2->mid = mid2;
    args arg1 = { // set up the arguments to pass
        .start = start,
        .end = mid1,
        .mid = start,
    };
    args arg2 = {
        .start = mid2,
        .end = end,
        .mid = mid2,
    };
    args arg3 = {
        .start = start,
        .end = end,
        .mid = mid1,
    };
    arg = &arg1; // set the pointer to the first set of arguments
    pthread_create(&thread_id1, NULL, sort, (void *)arg); // create a thread to run sort on the first half the array
    arg = &arg2;
    pthread_create(&thread_id2, NULL, sort, (void *)arg);
    pthread_join(thread_id1, NULL); // wait for the thread to finish
    pthread_join(thread_id2, NULL);
    arg = &arg3;
    sort(arg); // sort out the results from the two threads
    for (arr_ptr = array; arr_ptr != &array[count]; arr_ptr++) { // print array elements
        printf("%d", *arr_ptr);
        if (arr_ptr != &array[count - 1])
            printf(" ");
    }
    free(buf); // free the heap memory
    free(array);
    fclose(fd);
    return 0;
    
}