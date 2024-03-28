#include "list.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>


static void swap(int *a, int *b) {
  int temp = *a;
  *a = *b;
  *b = temp;
}

// Generates random data within a specified range
static void generate_random_data(int *arr, int size) {
  for (int i = 0; i < size; i++) {
    arr[i] = rand();
  }
}
static int asc(const void *a, const void *b) { return (*(int *)a - *(int *)b); }

static int des(const void *a, const void *b) { return (*(int *)b - *(int *)a); }

// Creates runs (partially sorted sub-arrays) of a given size
static void create_runs(int *arr, int size, int run_size) {
  int num_runs = size / run_size;
  for (int i = 0; i < num_runs; i++)
    qsort(arr + (i * run_size), run_size, sizeof(int),
          (int (*)(const void *, const void *))asc);
}

// Swaps elements within a run with a certain probability
static void shuffle_within_runs(int *arr, int size, int run_size,
                                double shuffle_prob) {
  for (int i = 0; i < size; i += run_size) {
    for (int j = 0; j < run_size; j++) {
      double ran = rand();
      int swap_index = rand() % run_size;
      if (ran / (double)RAND_MAX < shuffle_prob && i + j < size &&
          i + swap_index < size)
        swap(&arr[i + j], &arr[i + swap_index]);
    }
  }
}
static void shuffle_times(int *arr, int size, int times) {
  while (times) {
    int rand1 = rand() % size;
    int rand2 = rand() % size;
    if (rand1 != rand2) {
      times--;
      swap(&arr[rand1], &arr[rand2]);
    }
  }
}

void create_sample(struct list_head *head, element_t *space, int samples,
                   uint8_t type) {
  srand(time(NULL));
  int run_size = 64;
  double shuffle_prob = 0.2;

  int arr[samples];
  if (type == 0)
    generate_random_data(arr, samples);

  else if (type == 1) {
    generate_random_data(arr, samples);
    qsort(arr, samples, sizeof(int), (int (*)(const void *, const void *))asc);
  } else if (type == 2) {
    generate_random_data(arr, samples);
    qsort(arr, samples, sizeof(int), (int (*)(const void *, const void *))des);
  } else if (type == 3) {
    generate_random_data(arr, samples);
    qsort(arr, samples, sizeof(int), (int (*)(const void *, const void *))asc);
    shuffle_times(arr, samples, 3);
  } else if (type == 4) {
    generate_random_data(arr, samples);
    qsort(arr, samples, sizeof(int), (int (*)(const void *, const void *))asc);
    shuffle_times(arr, samples, 10);
  } else if (type == 5) {
    generate_random_data(arr, samples);
    create_runs(arr, samples, run_size);
  } else if (type == 6) {
    generate_random_data(arr, samples);
    create_runs(arr, samples, run_size);
    shuffle_within_runs(arr, samples, run_size, shuffle_prob);
  }  else if (type == 7) {
    int r = rand();
    for (int i = 0; i < samples; i++)
      arr[i] = r;
  }

  for (int i = 0; i < samples; i++) {
    element_t *elem = space + i;
    elem->val = arr[i];
    elem->seq = i;
    list_add_tail(&elem->list, head);
  }
}