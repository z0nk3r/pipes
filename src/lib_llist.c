/** @file lib_llist.c
 *
 * @brief LinkedList Library for stack/queue storage of data. Singly-Linked
 * Lists. Analogous to a Python list.
 *
 */

#include "lib_llist.h"

struct ll_node_t
{
    struct ll_node_t *next;
    void             *data;
};

struct llist_t
{
    int32_t         count;
    ll_node_t      *head;
    ll_node_t      *tail;
    pthread_mutex_t lock;
};

llist_t *
ll_create (void)
{
    llist_t *llist = NULL;

    llist = calloc(1, sizeof(*llist));
    if (NULL == llist)
    {
        perror("llist create");
        errno = 0;
        goto LL_CREATE_RET;
    }

    if (0 != pthread_mutex_init(&llist->lock, NULL))
    {
        perror("ll_create mutex init");
        errno = 0;
        free(llist);
        llist = NULL;
    }

LL_CREATE_RET:
    return llist;
}

int32_t
ll_enq (llist_t *llist, void *data)
{
    int32_t ret_val = RETVAL_FAILURE;
    if ((NULL == llist) || (NULL == data))
    {
        goto LL_ENQ_RET;
    }

    ll_node_t *node = calloc(1, sizeof(*node));
    if (NULL == node)
    {
        perror("enqueue allocation");
        errno = 0;
        goto LL_ENQ_RET;
    }

    node->data = data;

    pthread_mutex_lock(&llist->lock);

    if (NULL == llist->head)
    {
        llist->head = node;
    }
    else
    {
        llist->tail->next = node;
    }

    llist->tail = node;
    ++llist->count;

    pthread_mutex_unlock(&llist->lock);
    ret_val = RETVAL_SUCCESS;

LL_ENQ_RET:
    return ret_val;
}

void *
ll_deq (llist_t *llist)
{
    void *data = NULL;
    if (NULL == llist)
    {
        goto LL_DEQ_RET;
    }

    pthread_mutex_lock(&llist->lock);

    if (NULL == llist->head)
    {
        // llist->tail = llist->head;
        goto LL_DEQ_RET;
    }
    else
    {
        data            = llist->head->data;
        ll_node_t *temp = llist->head;
        llist->head     = llist->head->next;
        free(temp);
        --llist->count;
    }

    pthread_mutex_unlock(&llist->lock);

LL_DEQ_RET:
    return data;
}

int32_t
ll_push (llist_t *llist, void *data)
{
    int32_t ret_val = RETVAL_FAILURE;
    if ((NULL == llist) || (NULL == data))
    {
        goto LL_ENQ_RET;
    }

    ll_node_t *node = calloc(1, sizeof(*node));

    if (NULL == node)
    {
        perror("push allocation");
        errno = 0;
        goto LL_ENQ_RET;
    }
    node->data = data;

    pthread_mutex_lock(&llist->lock);

    node->next  = llist->head;
    llist->head = node;
    if (NULL == llist->head->next)
    {
        llist->tail = llist->head;
    }
    ++llist->count;

    pthread_mutex_unlock(&llist->lock);

    ret_val = RETVAL_SUCCESS;

LL_ENQ_RET:
    return ret_val;
}

void *
ll_pop (llist_t *llist)
{
    return ll_deq(llist);
}

void *
ll_search (llist_t *llist, char *key, llcmp_f cmpnode)
{
    void *ret_ptr = NULL;
    if ((NULL == llist) || (NULL == key) || (NULL == cmpnode))
    {
        goto LL_SEARCH_RET;
    }

    pthread_mutex_lock(&llist->lock);

    ll_node_t *node = llist->head;
    while (node)
    {
        if (0 == cmpnode(node->data, key))
        {
            ret_ptr = node->data;
            break;
        }
        node = node->next;
    }

    pthread_mutex_unlock(&llist->lock);

LL_SEARCH_RET:
    return ret_ptr;
}

int64_t 
ll_delete (llist_t *llist, int64_t num_delete, lliter_f freenode)
{
    int32_t ret_val = RETVAL_FAILURE;

    if ((NULL == llist) || (0 >= num_delete) || (NULL == freenode))
    {
        goto LL_DEL_RET;
    }
    ++ret_val;

    pthread_mutex_lock(&llist->lock);
    if (NULL == llist->head)
    {
        llist->tail = llist->head;
        goto LL_DEL_PRERET;
    }
    void *temp_data = NULL;

    for (int idx = 0; idx < num_delete; idx++)
    {
        temp_data       = llist->head->data;
        ll_node_t *temp = llist->head;
        llist->head     = llist->head->next;
        free(temp);
        --llist->count;

        if (NULL != temp_data)
        {
            freenode(temp_data);
            ++ret_val;
        }
    }

LL_DEL_PRERET:
    pthread_mutex_unlock(&llist->lock);

LL_DEL_RET:
    return ret_val;
}

int64_t
ll_dump (llist_t *llist, lliter_f freenode)
{
    int32_t ret_val = RETVAL_FAILURE;

    if ((NULL == llist) || (NULL == freenode))
    {
        goto LL_DUMP_RET;
    }
    ++ret_val;

    pthread_mutex_lock(&llist->lock);

    ll_node_t *head = llist->head;
    ll_node_t *temp = NULL;

    while (head)
    {
        temp = head;
        head = head->next;
        if (NULL != temp->data)
        freenode(temp->data);
        ++ret_val;
        free(temp);
        temp = NULL;
        --llist->count;
    }

    llist->head = NULL;
    llist->tail = NULL;

    pthread_mutex_unlock(&llist->lock);

LL_DUMP_RET:
    return ret_val;
}

void
ll_destroy (llist_t **llist, lliter_f freenode)
{
    if ((NULL == llist) || (NULL == (*llist)) || (NULL == freenode))
    {
        return;
    }

    pthread_mutex_lock(&(*llist)->lock);

    ll_node_t *head = (*llist)->head;
    ll_node_t *temp = NULL;

    while (head)
    {
        temp = head;
        head = head->next;
        freenode(temp->data);
        free(temp);
    }

    pthread_mutex_unlock(&(*llist)->lock);

    pthread_mutex_destroy(&(*llist)->lock);
    (*llist)->head = NULL;
    free(*llist);
    *llist = NULL;
}

void
ll_printf (llist_t *llist, lliter_f printnode)
{
    if ((NULL == llist) || (NULL == printnode))
    {
        return;
    }

    int32_t counter = 1;

    pthread_mutex_lock(&llist->lock);
    ll_node_t *node = llist->head;

    if (NULL == node)
    {
        printf(" HEAD: | EMPTY |\n");
    }

    while (node)
    {
        if (1 == counter)
        {
            printf(" HEAD: ");
            printnode(node->data);
            counter++;
        }
        else if (counter == llist->count)
        {
            printf(" TAIL: ");
            printnode(node->data);
            counter++;
        }
        else
        {
            printf("%5d: ", counter++);
            printnode(node->data);
        }

        node = node->next;
        (void)fflush(stdout);
    }

    pthread_mutex_unlock(&llist->lock);
    puts("");
}

int32_t
ll_len (llist_t *llist)
{
    int32_t len_retval = RETVAL_FAILURE;
    if (NULL == llist)
    {
        goto LL_LEN_RET;
    }

    len_retval = llist->count;

LL_LEN_RET:
    return len_retval;
}

void *
ll_head (llist_t *llist)
{
    void *ret_ptr = NULL;
    if ((NULL == llist) || (NULL == llist->head))
    {
        goto LL_HEAD_RET;
    }

    ret_ptr = llist->head->data;

LL_HEAD_RET:
    return ret_ptr;
}

void *
ll_tail (llist_t *llist)
{
    void *ret_ptr = NULL;
    if ((NULL == llist) || (NULL == llist->tail))
    {
        goto LL_TAIL_RET;
    }

    ret_ptr = llist->tail->data;

LL_TAIL_RET:
    return ret_ptr;
}

int32_t
ll_iter (llist_t *llist, lliter_f iter)
{
    int32_t ret_val = RETVAL_FAILURE;
    if ((NULL == llist) || (NULL == iter))
    {
        goto ITER_RETURN;
    }

    if (NULL == llist->head)
    {
        goto ITER_RETURN;
    }

    ++ret_val;

    pthread_mutex_lock(&llist->lock);
    ll_node_t *curr = llist->head;

    while (curr)
    {
        if (curr->data)
        {
            iter(curr->data);
            ++ret_val;
        }
        curr = curr->next;
    }
    pthread_mutex_unlock(&llist->lock);

ITER_RETURN:
    return ret_val;
}

ll_node_t *
ll_iter_start (llist_t *llist)
{
    ll_node_t *node = NULL;

    if ((NULL == llist) || (NULL == llist->head))
    {
        (void)fprintf(stderr, "ll_iter_start invalid params\n");
        goto LL_ITER_START_RET;
    }

    node = calloc(1, sizeof(*node));
    if (node)
    {
        node->next = llist->head;
    }

LL_ITER_START_RET:
    return node;
}

void *
ll_iter_next (ll_node_t *node)
{
    void *data = NULL;

    if ((NULL == node) || (NULL == node->next))
    {
        goto LL_ITER_NEXT_RET;
    }

    data       = node->next->data;
    node->next = node->next->next;

LL_ITER_NEXT_RET:
    return data;
}

int32_t
ll_iter_size (ll_node_t *node)
{
    int32_t iter_sz = RETVAL_FAILURE;
    if (NULL == node)
    {
        goto LL_ITER_SZ_RET;
    }

    ++iter_sz;
    ll_node_t *temp = node->next;
    while (temp)
    {
        ++iter_sz;
        temp = temp->next;
    }

LL_ITER_SZ_RET:
    return iter_sz;
}

void
ll_iter_destroy (ll_node_t **node)
{
    if ((NULL == node) || (NULL == (*node)))
    {
        return;
    }
    free(*node);
    *node = NULL;
}

/*** end of file ***/
