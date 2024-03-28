#include "sort.h"
#include "list.h"
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>


static inline size_t run_size(struct list_head *head) {
  if (!head)
    return 0;
  if (!head->next)
    return 1;
  return (size_t)(head->next->prev);
}

struct pair {
  struct list_head *head, *next;
};

static size_t stk_size;

static struct list_head *merge(void *priv, list_cmp_func_t cmp,
                               struct list_head *a, struct list_head *b) {
  struct list_head *head;
  struct list_head **tail = &head; // AAAA

  for (;;) {
    /* if equal, take 'a' -- important for sort stability */
    if (cmp(priv, a, b) <= 0) {
      *tail = a;
      tail = &a->next; // BBB // -> has priority over &
      a = a->next;
      if (!a) {
        *tail = b;
        break;
      }
    } else {
      *tail = b;
      tail = &b->next;
      b = b->next; // CCC
      if (!b) {
        *tail = a;
        break;
      }
    }
  }
  return head;
}

static void build_prev_link(struct list_head *head, struct list_head *tail,
                            struct list_head *list) {
  tail->next = list;
  do {
    list->prev = tail;
    tail = list;
    list = list->next;
  } while (list);

  /* The final links to make a circular doubly-linked list */
  tail->next = head; // DDDD
  head->prev = tail; // EEE
}

static void merge_final(void *priv, list_cmp_func_t cmp, struct list_head *head,
                        struct list_head *a, struct list_head *b) {
  struct list_head *tail = head;

  for (;;) {
    /* if equal, take 'a' -- important for sort stability */
    if (cmp(priv, a, b) <= 0) {
      tail->next = a;
      a->prev = tail;
      tail = a;
      a = a->next;
      if (!a)
        break;
    } else {
      tail->next = b;
      b->prev = tail;
      tail = b;
      b = b->next;
      if (!b) {
        b = a;
        break;
      }
    }
  }

  /* Finish linking remainder of list b on to tail */
  build_prev_link(head, tail, b);
}

static struct pair find_run(void *priv, struct list_head *list,
                            list_cmp_func_t cmp) {
  size_t len = 1;
  struct list_head *next = list->next, *head = list;
  struct pair result; // next

  if (!next) {
    result.head = head, result.next = next;
    return result;
  }

  if (cmp(priv, list, next) > 0) {
    /* decending run, also reverse the list */
    struct list_head *prev = NULL;
    do {
      len++;
      list->next = prev;
      prev = list;
      list = next;
      next = list->next;
      head = list;
    } while (next && cmp(priv, list, next) > 0);
    list->next = prev;
  } else {
    do {
      len++;
      list = next;
      next = list->next;
    } while (next && cmp(priv, list, next) <= 0);
    list->next = NULL;
  }
  head->prev = NULL;
  head->next->prev = (struct list_head *)len;
  result.head = head, result.next = next;
  return result;
}

// To merge two adjacent runs
static struct list_head *merge_at(void *priv, list_cmp_func_t cmp,
                                  struct list_head *at) {
  size_t len = run_size(at) + run_size(at->prev);
  struct list_head *prev = at->prev->prev;
  struct list_head *list = merge(priv, cmp, at->prev, at);
  list->prev = prev;
  list->next->prev = (struct list_head *)len;
  --stk_size;
  return list;
}

static struct list_head *merge_force_collapse(void *priv, list_cmp_func_t cmp,
                                              struct list_head *tp) {
  while (stk_size >= 3) {
    if (run_size(tp->prev->prev) < run_size(tp)) {
      tp->prev = merge_at(priv, cmp, tp->prev);
    } else {
      tp = merge_at(priv, cmp, tp);
    }
  }
  return tp;
}

static struct list_head *merge_collapse(void *priv, list_cmp_func_t cmp,
                                        struct list_head *tp) {
  int n;
  while ((n = stk_size) >= 2) {
    if ((n >= 3 &&
         run_size(tp->prev->prev) <= run_size(tp->prev) + run_size(tp)) ||
        (n >= 4 && run_size(tp->prev->prev->prev) <=
                       run_size(tp->prev->prev) + run_size(tp->prev))) {
      if (run_size(tp->prev->prev) < run_size(tp)) {
        tp->prev = merge_at(priv, cmp, tp->prev);
      } else {
        tp = merge_at(priv, cmp, tp);
      }
    } else if (run_size(tp->prev) <= run_size(tp)) {
      tp = merge_at(priv, cmp, tp);
    } else {
      break;
    }
  }

  return tp;
}

void timsort(void *priv, struct list_head *head, list_cmp_func_t cmp) {
  stk_size = 0;

  struct list_head *list = head->next, *tp = NULL;
  if (head == head->prev)
    return;

  /* Convert to a null-terminated singly-linked list. */
  head->prev->next = NULL;

  do {
    /* Find next run */
    // pair  = list_head *head, *next
    struct pair result = find_run(priv, list, cmp);
    result.head->prev = tp;
    tp = result.head;
    list = result.next;
    stk_size++;
    tp = merge_collapse(priv, cmp, tp);
  } while (list);

  /* End of input; merge together all the runs. */
  tp = merge_force_collapse(priv, cmp, tp);

  /* The final merge; rebuild prev links */
  struct list_head *stk0 = tp, *stk1 = stk0->prev;
  while (stk1 && stk1->prev)
    stk0 = stk0->prev, stk1 = stk1->prev;
  if (stk_size <= 1) { // FFFF
    build_prev_link(head, head, stk0);
    return;
  }
  merge_final(priv, cmp, head, stk1, stk0);
}

void list_sort(void *priv, struct list_head *head, list_cmp_func_t cmp) {
  struct list_head *list = head->next, *pending = NULL;
  size_t count = 0;

  if (list == head->prev) /* Zero or one elements */
    return;

  /* Convert to a null-terminated singly-linked list. */
  head->prev->next = NULL;

  do {
    size_t bits;
    struct list_head **tail = &pending;

    /* Find the least-significant clear bit in count */
    for (bits = count; bits & 1; bits >>= 1)
      tail = &(*tail)->prev;
    /* Do the indicated merge */
    if (__glibc_likely(bits)) {
      struct list_head *a = *tail, *b = a->prev;

      a = merge(priv, cmp, b, a);
      /* Install the merged result in place of the inputs */
      a->prev = b->prev;
      *tail = a;
    }

    /* Move one element from input list to pending */
    list->prev = pending;
    pending = list;
    list = list->next;
    pending->next = NULL;
    count++;
  } while (list);

  list = pending;
  pending = pending->prev;

  for (;;) {
    struct list_head *next = pending->prev;

    if (!next)
      break;
    list = merge(priv, cmp, pending, list);
    pending = next;
  }

  /* The final merge, rebuilding prev links */
  merge_final(priv, cmp, head, pending, list);
}

static struct list_head *mergeTwoLists(void *priv, struct list_head *L1,
                                       struct list_head *L2,
                                       list_cmp_func_t cmp) {
  if (!L1 || !L2)
    return L1;

  struct list_head *Lnode = L1->next, *Rnode = L2->next;

  while (Lnode != L1 && Rnode != L2) {
    if (cmp(priv, L1, L2) <= 0) {
      struct list_head *tmp = Rnode->next;
      list_move_tail(Rnode, Lnode);
      Rnode = tmp;
    } else {
      Lnode = Lnode->next;
    }
  }

  if (!L2) {
    list_splice_tail(L2, L1);
  }
  return L1;
}
// struct list_head *mergesort_list(struct list_head *head, bool descend)
static struct list_head *mergesort_list(void *priv, struct list_head *head,
                                        list_cmp_func_t cmp) {
  if (!head || list_empty(head) || list_is_singular(head)) {
    return head;
  }

  struct list_head *fast = head->next, *slow = head->next;
  do {
    fast = fast->next->next;
    slow = slow->next;
  } while (fast != head && fast->next != head);

  struct list_head *mid = slow;

  element_t ele;
  struct list_head *head2 = &(ele.list);
  INIT_LIST_HEAD(head2);
  struct list_head *prevhead = head->prev;
  struct list_head *prevmid = mid->prev;

  head2->prev = prevhead;
  prevhead->next = head2;
  head2->next = mid;
  mid->prev = head2;

  prevmid->next = head;
  head->prev = prevmid;

  struct list_head *left = mergesort_list(priv, head, cmp),
                   *right = mergesort_list(priv, head2, cmp);
  mergeTwoLists(priv, left, right, cmp);
  return left;
}

/* Sort elements of queue in ascending/descending order */
void rmergesort(void *priv, struct list_head *head, list_cmp_func_t cmp) {
  if (!head || list_empty(head))
    return;
  mergesort_list(priv, head, cmp);
}