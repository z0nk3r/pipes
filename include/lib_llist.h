/** @file lib_llist.h
 *
 * @brief LinkedList Library for stack/queue storage of data. Singly-Linked
 * Lists. Analogous to a Python list.
 *
 */

#ifndef LIB_LLIST_H
#define LIB_LLIST_H

#include <errno.h>
#include <pthread.h>
#include <string.h>
#include <strings.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>

#define RETVAL_FAILURE -1
#define RETVAL_SUCCESS 0

/**
 * @brief struct ll_node_t - struct for each item in the linkedlist
 * @param   struct ll_node_t *next;
 * @param   void             *data;
 */
typedef struct ll_node_t ll_node_t;

/**
 * @brief struct llist_t - struct for containing all linkedlist metadata
 * @param   uint64_t            count;
 * @param   ll_node_t           *head;
 * @param   ll_node_t           *tail;
 * @param   pthread_mutex_t     lock;
 */
typedef struct llist_t llist_t;

// =============================================================================
//                               FUNCTION POINTERS
// =============================================================================
/**
 * @brief Function pointer that takes a single data point and does FUNC on DATA.
 *
 * Used for ll_iter and free-ing on ll_destroy.
 */
typedef void (*lliter_f)(void *);

/**
 * @brief Function pointer that takes a data point and does custom compare FUNC on DATA.
 *
 * Used for ll_search.
 */
typedef int32_t (*llcmp_f)(void *, char *);

// =============================================================================
//                              LIBRARY FUNCTIONS
// =============================================================================
/**
 * @brief Initialize a linked llist
 *
 * @returns llist       (llist *)           PTR to llist, NULL if Failed.
 */
llist_t *ll_create(void);

/**
 * @brief Put DATA at end of llist (enqueue) (queue behavior)
 *
 * @param   llist       (llist_t *)         PTR to the linked llist
 * @param   data        (void *)            DATA to be added to the llist
 *
 * @returns 0 on Success, -1 if Failed.
 */
int32_t ll_enq(llist_t *llist, void *data);

/**
 * @brief Returns DATA from front of llist (dequeue)
 *
 * @param   llist       (llist_t *)         PTR to the linked llist
 *
 * @returns ll_val      (void *)            PTR to data found at HEAD, NULL if
 * Failed.
 */
void *ll_deq(llist_t *llist);

/**
 * @brief Adds DATA to front/top of the llist (stack behavior)
 *
 * @param   llist       (llist_t *)         PTR to the linked llist
 * @param   data        (void *)            DATA to be added to the llist
 *
 * @returns 0 on Success, -1 if Failed.
 */
int32_t ll_push(llist_t *llist, void *data);

/**
 * @brief Returns DATA from front of llist (dequeue)
 *
 * @param   llist       (llist_t *)         PTR to the linked llist
 *
 * @returns ll_val      (void *)            PTR to data at HEAD, NULL if Failed.
 */
void *ll_pop(llist_t *llist);

/**
 * @brief Search llist for first occurance of KEY. (Nondestructive)
 * 
 * @param   llist       (llist_t *)         PTR to the linked llist
 * @param   key         (char *)            PTR to the search key
 * @param   cmpnode     (llcmp_f)           Func PTR to cmp data with
 * 
 * @returns ll_val      (void *)            PTR to data found, NULL if Failed.
 */
void *ll_search(llist_t *llist, char *key, llcmp_f cmpnode);

/**
 * @brief delete up to n nodes, starting at front/top of llist
 *
 * @param   llist       (llist_t **)        PTR to the PTR of the linked llist
 * @param   num_delete  (int64_t)           Number of Nodes to delete
 * @param   freenode    (lliter_f)          Func PTR to free DATA with
 *
 * @returns del_nodes   (int64_t)           Number of nodes successfully
 * deleted; -1 if failed.
 */
int64_t ll_delete(llist_t *llist, int64_t num_delete, lliter_f freenode);

/**
 * @brief empty/dump all contents of the llist
 *
 * @param   llist       (llist_t *)         PTR of the linked llist
 * @param   freenode    (lliter_f)          Func PTR to free DATA with
 *
 * @returns del_nodes   (int64_t)           Number of nodes successfully
 * deleted; -1 if failed.
 */
int64_t ll_dump(llist_t *llist, lliter_f freenode);

/**
 * @brief destroy the llist
 *
 * @param   llist       (llist_t **)        PTR to the PTR of the linked llist
 * @param   freenode    (lliter_f)          Func PTR to free DATA with
 *
 * @returns N/A         (void)
 */
void ll_destroy(llist_t **llist, lliter_f freenode);

/**
 * @brief printf the link llist
 *
 * @param   llist       (llist_t *)         PTR to the linked llist
 * @param   printnode   (lliter_f)          Func PTR to print DATA with
 *
 * @returns N/A         (void)
 */
void ll_printf(llist_t *llist, lliter_f printnode);

/**
 * @brief print the length of a llist
 *
 * @param   llist       (llist_t *)        PTR to linked llist
 *
 * @returns ll_len      (int32_t)          LEN of llist, -1 if Failed.
 */
int32_t ll_len(llist_t *llist);

/**
 * @brief Return DATA at HEAD of llist (Nondestructive)
 *
 * @param   llist       (llist_t *)        PTR to linked llist
 *
 * @returns data        (void*)            PTR to data, NULL if Failed.
 */
void *ll_head(llist_t *llist);

/**
 * @brief Return DATA at TAIL of llist (Nondestructive)
 *
 * @param   llist       (llist_t *)        PTR to linked llist
 *
 * @returns data        (void*)            PTR to data, NULL if Failed.
 */
void *ll_tail(llist_t *llist);

/**
 * @brief for each item in llist, do iter()
 *
 * @param   llist       (llist_t *)         PTR to the linked llist
 * @param   iter        (lliter_f)            FUNC PTR to execute on found data
 *
 * @returns ll_res      (int32_t)           NUM of results iter'd over, -1 if
 * Failed.
 */
int32_t ll_iter(llist_t *llist, lliter_f iter);

/**
 * @brief create a linked llist iterable, starting at HEAD. Nondestructive read
 * from llist.
 *
 * @param   llist       (llist_t *)         PTR to the linked llist
 *
 * @returns node        (ll_node_t *)       PTR pointing to HEAD of llist, NULL
 * if Failed.
 */
ll_node_t *ll_iter_start(llist_t *llist);

/**
 * @brief continues to the next item in llist
 *
 * @param   node        (ll_node_t *)       PTR to the current node in the llist
 * from ll_iter_start()
 *
 * @returns node        (void*)             PTR to data of given node, NULL if
 * Failed.
 */
void *ll_iter_next(ll_node_t *node);

/**
 * @brief Print the length of the current llist iteration (curr to tail)
 *
 * @param   node        (ll_node_t *)       PTR to the current node in the llist
 * from ll_iter_start()
 *
 * @returns ll_sz       (int32_t)           LEN of current llist, 0 if Failed.
 */
int32_t ll_iter_size(ll_node_t *node);

/**
 * @brief Destroy the iterable created in ll_iter_start()
 *
 * @param   node        (ll_node_t **)      PTR to the PTR of the llist HEAD
 *
 * @returns N/A         (void)
 */
void ll_iter_destroy(ll_node_t **node);

#endif /* LIB_LLIST_H */

/*** end of file ***/
