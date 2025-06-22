#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <unistd.h>
#include <sys/wait.h>

// sort
#define NUM_ITEMS 10
int mid = NUM_ITEMS / 2;

void mergeSort(int numbers[], int temp[], int array_size);
void m_sort(int numbers[], int temp[], int left, int right);
void merge(int numbers[], int temp[], int left, int mid, int right);

int numbers[NUM_ITEMS];
int temp[NUM_ITEMS];
int parent_numbers[NUM_ITEMS/2+1];
int child_numbers[NUM_ITEMS/2+1];
// pipe
int fd[2];
int pid, status;

int main()
{
  int i;
  //seed random number generator
  srand(getpid());

  //fill array with random integers
  for (i = 0; i < NUM_ITEMS; i++){
    numbers[i] = rand();
  }
  // print unsorted array
  printf("Unsorted array:\n");
  for (i = 0; i < NUM_ITEMS; i++)
    printf("%i\n", numbers[i]);

  if (pipe(fd) == -1) {
    perror("pipe failed.");
    exit(1);
  }

  if ((pid = fork()) == -1) {
    perror("fork failed.");
    exit(1);
  }

  if (pid == 0) { // child process
    // sort
    int child_num_items = NUM_ITEMS - mid;
    int child_temp[child_num_items];
    for (i = 0; i < child_num_items; i++) {
      child_numbers[i] = numbers[mid + i];
    }

    mergeSort(child_numbers, child_temp, child_num_items);

    // send sorted array to parent
    close(fd[0]);
    if (write(fd[1], child_numbers, child_num_items * sizeof(int)) == -1) {
      perror("pipe write.");
      exit(1);
    }
    close(fd[1]);
    exit(0);

  } else { // parent process
    // sort
    int parent_num_items = mid;
    int parent_temp[parent_num_items];
    for (i = 0; i < parent_num_items; i++) {
      parent_numbers[i] = numbers[i];
    }

    mergeSort(parent_numbers, parent_temp, parent_num_items);

    // receive sorted array from child
    close(fd[1]);
    if (read(fd[0], child_numbers, (NUM_ITEMS - mid) * sizeof(int)) == -1) {
      perror("pipe read.");
      exit(1);
    }
    wait(&status);
    close(fd[0]);
  }
  // merge two sorted arrays
  int p = 0, c = 0;
  for (i = 0; p < mid && c < NUM_ITEMS - mid; i++) {
    if (parent_numbers[p] <= child_numbers[c]) {
      numbers[i] = parent_numbers[p];
      p++;
    } else {
      numbers[i] = child_numbers[c];
      c++;
    }
  }
  while (p < mid) {
    numbers[i] = parent_numbers[p];
    p++;
    i++;
  }
  while (c < NUM_ITEMS - mid) {
    numbers[i] = child_numbers[c];
    c++;
    i++;
  }
  // print sorted array
  printf("Sorted array:\n");
  for (i = 0; i < NUM_ITEMS; i++)
    printf("%i\n", numbers[i]);

  return 0;
}


void mergeSort(int numbers[], int temp[], int array_size)
{
  m_sort(numbers, temp, 0, array_size - 1);
}



void m_sort(int numbers[], int temp[], int left, int right)
{
  int mid;

  if (right > left)
  {
    mid = (right + left) / 2;
    m_sort(numbers, temp, left, mid);
    m_sort(numbers, temp, mid+1, right);

    merge(numbers, temp, left, mid+1, right);
  }
}


void merge(int numbers[], int temp[], int left, int mid, int right)
{
  int i, left_end, num_elements, tmp_pos;

  left_end = mid - 1;
  tmp_pos = left;
  num_elements = right - left + 1;

  while ((left <= left_end) && (mid <= right))
  {
    if (numbers[left] <= numbers[mid])
    {
      temp[tmp_pos] = numbers[left];
      tmp_pos = tmp_pos + 1;
      left = left +1;
    }
    else
    {
      temp[tmp_pos] = numbers[mid];
      tmp_pos = tmp_pos + 1;
      mid = mid + 1;
    }
  }

  while (left <= left_end)
  {
    temp[tmp_pos] = numbers[left];
    left = left + 1;
    tmp_pos = tmp_pos + 1;
  }
  while (mid <= right)
  {
    temp[tmp_pos] = numbers[mid];
    mid = mid + 1;
    tmp_pos = tmp_pos + 1;
  }

  for (i=0; i <= num_elements; i++)
  {
    numbers[right] = temp[right];
    right = right - 1;
  }
}
