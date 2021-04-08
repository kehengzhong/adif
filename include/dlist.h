/*
 * Copyright (c) 2003-2021 Ke Hengzhong <kehengzhong@hotmail.com>
 * All rights reserved. See MIT LICENSE for redistribution.
 */

#ifndef _DLIST_H_
#define _DLIST_H_

#ifdef __cplusplus
extern "C" {
#endif


typedef struct list_st dlist_t;

/* Note: the node structure must be reserved 2-pointers space while being defined.
         the 2-pointers must be at the begining of the struction and casted as 
         prev and next of dlist_t.  */

/* get the next node of the given node */
void * lt_get_next (void * node);


/* get the prev node of the given node */
void * lt_get_prev (void * node);

/* list instance allocation routine. allocate space and initialize all 
 * internal variables. the entry parameter is the default comparing function */
dlist_t * lt_new ();


/* duplicate one instance from the old list. all members are assigned same 
 * value. More attention should be paid, the actual content that pointer 
 * pointed to are same. */
dlist_t * lt_dup (dlist_t * lt);


/* reset all member values to initial state. */
void lt_zero (dlist_t * lt);


/* release the resource of the list. */
void lt_free (dlist_t * lt);


/* release the resources of the list, and call the user-given function to 
 * release actual content that pointer point to. */
void lt_free_all (dlist_t * lt, int (*func)());


/* add the data to the header of the list.  */
int lt_prepend (dlist_t * lt, void * data);


/* add the data to the tail of the list */
int lt_append (dlist_t * lt, void * data);


/* combine the 2 list into one list. after inserted into the header of the 
 * first list, the second list will be released. */
int lt_head_combine (dlist_t * lt, dlist_t ** plist);


/* combine the 2 list into one list. after inserted into the tail of the 
 * first list, the second list will be released.  */
int lt_tail_combine (dlist_t * lt, dlist_t ** plist);

/* combine the 2 list into one list. after inserted into the tail of the 
 * first list, the second list will be empty.  */
int lt_tail_combine_nodel (dlist_t * lt, dlist_t * list);


/* insert one data member into the given location of the list. */
int lt_insert (dlist_t * lt, void * data, int loc);


/* insert one data member immediately before the data member. */
int lt_insert_before (dlist_t * lt, void * curData, void * data);

/* insert one data node according to comparing result, generate a sorted list */
int lt_sort_insert_by (dlist_t * lt, void * data, int (*cmp)(void *, void *));

/* insert one data member immediately after the data member. */
int lt_insert_after (dlist_t * lt, void * curData, void * data);


/* insert all the members of the given list into the given location of the 
 * list. the second list will be released after insertion.  */
int lt_insert_list (dlist_t * lt, dlist_t ** plist, int loc);



/* insert all the members of the given list immediately before the data 
 * member. the second list will be released after insertion.  */
int lt_insert_list_before (dlist_t * lt, dlist_t ** plist, void * data);



/* insert all the members of the given list immediately after the data
 * member. the second list will be released after insertion. */
int lt_insert_list_after (dlist_t * lt, dlist_t ** plist, void * data);



void * lt_rm_head (dlist_t * lt);
void * lt_rm_tail (dlist_t * lt);


/* delete the data member of the given location from the list */
void * lt_delete (dlist_t * lt, int loc);


/* delete the data member from the list. */
void * lt_delete_ptr (dlist_t * lt, void * node);


/* delete the given-number nodes from the specified location on.  */
void * lt_delete_from_loc (dlist_t * lt, int from, int num);


/* delete the given-number nodes from the specified node on.  */
void * lt_delete_from_node (dlist_t * lt, void * node, int num);


/* return the total number of data member of the list */
int lt_num (dlist_t * lt);


/* return the specified-location data member of the list */
void * lt_get (dlist_t * lt, int loc);


/* return the specified-location data member of the list */
void * lt_value (dlist_t * lt, int loc);


/* return the first data member that the list stores. */
void * lt_first (dlist_t * lt);


/* return the last data member that the list controls */
void * lt_last (dlist_t * lt);


/* return the index number of the given node in the list */
int lt_index (dlist_t * lt, void * node);


/* according to the index order of the list, create a linear arr_t instance
 * to share and co-manage the same data contents. */
arr_t * lt_convert_to_linear (dlist_t * lt);

/* according to existing order of the arr_t, create a linked dlist_t instance
 * to manage the same data contents in a different manner. */
dlist_t * lt_new_from_linear (arr_t * st);


/* a linear search method. according to the pattern, traverse the every member
 * of the list and invoke comparing function. if the comparing result is zero,
 * then return the corresponding data. the comparing function is implemented
 * and passed as para. its first para is the data member of the list, the 
 * second para is the pattern. */
void * lt_search (dlist_t * lt, void * pattern, int (*cmp)(void *, void *));


/* a linear search method. according to the pattern, traverse the every member 
 * of the list and invoke comparing function. if the comparing result is zero,
 * then append the corresponding data to an linear stack instance. the 
 * comparing function is implemented by user and passed as a para. its first 
 * para is the data member of the list, the second para is the pattern. */
arr_t * lt_search_all (dlist_t * lt, void * pattern, int (*cmp)(void *, void *));


#ifdef __cplusplus
}
#endif

#endif

