
#include "adifall.ext"

int arr_sort_cmp (void * a, void * b)
{
    void * pa = *(void **)a;
    void * pb = *(void **)b;
    int  vala = (int)(long)pa;
    int  valb = (int)(long)pb;

    return vala - valb;
}

int rbtree_cmp_key (void * a, void * b)
{
    int  vala = (int)(long)a;
    int  valb = (int)(long)b;

    return vala - valb;
}

ulong hashtab_hash_key (void * key)
{
    return (ulong)key;
}

int rbtree_callback (void * para, void * key, void * obj, int index)
{
    int   val = (int)(long)key;

    if (index > 0 && (index % 10)==0) printf("\n");
    printf(" (%d)%d", index, val);
    return 0;
}


int main (int argc, char ** argv)
{
    int         i, ind, num = 0;

    arr_t      * arlist2 = NULL;
    arr_t      * arlist = NULL;
    rbtree_t   * ptree = NULL;
    hashtab_t  * hashtab = NULL;
    skiplist_t * skiplist = NULL;

    int        * pdata = NULL;
    btime_t      time1, time2, diff;
    void       * res = NULL;
    int          ret, val = 0;

    float       ar_insertres[20];
    float       ar2_insertres[20];
    float       ar_findres[20];
    float       ar2_findres[20];
    float       ar_deleteres[20];
    int         ar_allocmem[20];
    float       rb_insertres[20];
    float       rb_findres[20];
    float       rb_deleteres[20];
    int         rb_allocmem[20];
    float       ht_insertres[20];
    float       ht_findres[20];
    float       ht_deleteres[20];
    int         ht_allocmem[20];
    float       skl_insertres[20];
    float       skl_findres[20];
    float       skl_deleteres[20];
    int         skl_allocmem[20];

    if (argc < 2) {
        printf("Usage: %s <num>\n", argv[0]);
        return 0;
    }
 
    srand(time(0));

    for (ind=1; ind<argc && argv[ind]; ind++) {
        num = atoi(argv[ind]);
        if (num <= 0) break;

        printf("\n\nData Sample: %d\n\n", num);

        /* Creates instance variable objects of some data structures. */
        arlist = arr_new(num+16);
        arlist2 = arr_new(num+16);

        ptree = rbtree_new(rbtree_cmp_key, 1);

        hashtab = ht_only_new(num*2, rbtree_cmp_key);
        ht_set_hash_func(hashtab, hashtab_hash_key);
    
        skiplist = skiplist_alloc(rbtree_cmp_key);

        /* Randomly generate data to be inserted into each data structure object.*/
        pdata = malloc(num * sizeof(int));
        for (i=0; i<num; i++) pdata[i] = rand();

        /* the performance indicators of inserting these data into dynamic array in an orderly manner. */
        btime(&time1);
        for (i=0; i<num; i++) {
            arr_insert_by(arlist, (void *)(long)pdata[i], rbtree_cmp_key);
            //arr_push(arlist, (void *)(long)pdata[i]);
            //arr_sort_by(arlist, arr_sort_cmp);
        }
        btime(&time2);
        diff = btime_diff(&time1, &time2);
        ar_insertres[ind-1] = (float)diff.s + (float)diff.ms/1000.;
        ar_allocmem[ind-1] = sizeof(*arlist) + arlist->num_alloc * sizeof(void *);
        printf("  DynArr data insert: %d/%d, TotleTime: %ld.%03ld sec\n",
                num, arr_num(arlist), diff.s, diff.ms);

        /* performance indicators of inserting data into dynamic array before sorting. */
        btime(&time1);
        for (i=0; i<num; i++) {
            arr_push(arlist2, (void *)(long)pdata[i]);
        }
        arr_sort_by(arlist2, arr_sort_cmp);
        btime(&time2);
        diff = btime_diff(&time1, &time2);
        ar2_insertres[ind-1] = (float)diff.s + (float)diff.ms/1000.;
        printf("  DynArr data insert: %d/%d, TotleTime: %ld.%03ld sec\n",
                num, arr_num(arlist2), diff.s, diff.ms);


        /* the performance indicators of inserting these data into the red-black tree */
        btime(&time1); val = 0;
        for (i=0; i<num; i++) {
            ret = rbtree_insert(ptree, (void *)(long)pdata[i], (void *)(long)pdata[i], NULL);
            if (ret == 0) {
                val++;
                printf("    RBTree inserting data, duplicate No.%d Val:%d\n", i, pdata[i]);
            }
        }
        btime(&time2);
        diff = btime_diff(&time1, &time2);
        rb_insertres[ind-1] = (float)diff.s + (float)diff.ms/1000.;
        rb_allocmem[ind-1] = sizeof(*ptree) + ptree->num * sizeof(rbtnode_t);
        printf("  RBTree data insert: %d/%d, Duplicate: %d, TotleTime: %ld.%03ld sec\n",
                num, rbtree_num(ptree), val, diff.s, diff.ms);


        /* the performance indicators of inserting these data into the hash table */
        btime(&time1); val = 0;
        for (i=0; i<num; i++) {
            ret = ht_set(hashtab, (void *)(long)pdata[i], (void *)(long)pdata[i]);
            if (ret == 0) {
                val++;
                printf("    Hasttab inserting data, duplicate No.%d Val:%d\n", i, pdata[i]);
            }
        }
        btime(&time2);
        diff = btime_diff(&time1, &time2);
        ht_insertres[ind-1] = (float)diff.s + (float)diff.ms/1000.;
        ht_allocmem[ind-1] = sizeof(*hashtab) + hashtab->len * sizeof(hashnode_t);
        printf(" HashTab data insert: %d/%d, Duplicate: %d, TotleTime: %ld.%03ld sec\n",
                num, ht_num(hashtab), val, diff.s, diff.ms);
 

        /* the performance indicators of inserting these data into the skiplist */
        btime(&time1); val = 0;
        for (i=0; i<num; i++) {
            ret = skiplist_insert(skiplist, (void *)(long)pdata[i], (void *)(long)pdata[i]);
            if (ret == 0) {
                val++;
                printf("    Skiplist inserting data, duplicate No.%d Val:%d\n", i, pdata[i]);
            }
        }
        btime(&time2);
        diff = btime_diff(&time1, &time2);
        skl_insertres[ind-1] = (float)diff.s + (float)diff.ms/1000.;
        skl_allocmem[ind-1] = skiplist_memsize(skiplist);
        printf("Skiplist data insert: %d/%d, Duplicate: %d, TotleTime: %ld.%03ld sec\n",
                num, skiplist_num(skiplist), val, diff.s, diff.ms);


        /* performance indicators of looking up all data in dynamic array */
        btime(&time1);
        for (i=0; i<num; i++) {
            res = arr_find_by(arlist, (void *)(long)pdata[i], rbtree_cmp_key);
            if (!res) printf("    DynArr not found %d\n", pdata[i]);
        }
        btime(&time2);
        diff = btime_diff(&time1, &time2);
        ar_findres[ind-1] = (float)diff.s + (float)diff.ms/1000.;
        printf("  DynArr data search: %d/%d, TotleTime: %ld.%03ld sec\n",
                num, arr_num(arlist), diff.s, diff.ms);

        btime(&time1);
        for (i=0; i<num; i++) {
            res = arr_find_by(arlist2, (void *)(long)pdata[i], rbtree_cmp_key);
            if (!res) printf("    ####DynArr not found %d\n", pdata[i]);
        }
        btime(&time2);
        diff = btime_diff(&time1, &time2);
        ar2_findres[ind-1] = (float)diff.s + (float)diff.ms/1000.;
        printf("##DynArr data search: %d/%d, TotleTime: %ld.%03ld sec\n",
                num, arr_num(arlist2), diff.s, diff.ms);
 

        /* performance indicators of looking up all data in red-black tree */
        btime(&time1);
        for (i=0; i<num; i++) {
            res = rbtree_get(ptree, (void *)(long)pdata[i]);
            if (!res) printf("    RBTree not found %d\n", pdata[i]);
        }
        btime(&time2);
        diff = btime_diff(&time1, &time2);
        rb_findres[ind-1] = (float)diff.s + (float)diff.ms/1000.;
        printf("  RBTree data search: %d/%d, TotleTime: %ld.%03ld sec\n",
                num, rbtree_num(ptree), diff.s, diff.ms);


        /* performance indicators of looking up all data in hash table */
        btime(&time1);
        for (i=0; i<num; i++) {
            res = ht_get(hashtab, (void *)(long)pdata[i]);
            if (!res) printf("    HashTab not found %d\n", pdata[i]);
        }
        btime(&time2);
        diff = btime_diff(&time1, &time2);
        ht_findres[ind-1] = (float)diff.s + (float)diff.ms/1000.;
        printf(" HashTab data search: %d/%d, TotleTime: %ld.%03ld sec\n",
                num, ht_num(hashtab), diff.s, diff.ms);


        /* performance indicators of looking up all data in skiplist */
        btime(&time1);
        for (i=0; i<num; i++) {
            res = skiplist_find(skiplist, (void *)(long)pdata[i]);
            if (!res) printf("    Skiplist not found %d\n", pdata[i]);
        }
        btime(&time2);
        diff = btime_diff(&time1, &time2);
        skl_findres[ind-1] = (float)diff.s + (float)diff.ms/1000.;
        printf("Skiplist data search: %d/%d, TotleTime: %ld.%03ld sec\n",
                num, skiplist_num(skiplist), diff.s, diff.ms);


        /* performance indicators of removing all data in dynamic array */
        btime(&time1);
        for (i=0; i<num/2; i+=2) {
            res = arr_delete_by(arlist, (void *)(long)pdata[i], rbtree_cmp_key);
            if (!res) printf("    DynArr del not found %d\n", pdata[i]);
            else {
                val = (int)(long)res;
                if (val != pdata[i]) printf("    DynArr del %d not match %d\n", val, pdata[i]);
            }
        }
        btime(&time2);
        diff = btime_diff(&time1, &time2);
        ar_deleteres[ind-1] = (float)diff.s + (float)diff.ms/1000.;
        printf("  DynArr data remove: %d/%d, TotleTime: %ld.%03ld sec\n",
                num, arr_num(arlist), diff.s, diff.ms);

        /* performance indicators of removing all data in red-black tree */
        btime(&time1);
        for (i=0; i<num/2; i+=2) {
            res = rbtree_delete(ptree, (void *)(long)pdata[i]);
            if (!res) printf("    RBTree del not found %d\n", pdata[i]);
            else {
                val = (int)(long)res;
                if (val != pdata[i]) printf("    RBTree del [No%d]%d not match %d\n", i, val, pdata[i]);
            }
        }
        btime(&time2);
        diff = btime_diff(&time1, &time2);
        rb_deleteres[ind-1] = (float)diff.s + (float)diff.ms/1000.;
        printf("  RBTree data remove: %d/%d, TotleTime: %ld.%03ld sec\n",
                num, rbtree_num(ptree), diff.s, diff.ms);


        /* performance indicators of removing all data in hash table */
        btime(&time1);
        for (i=0; i<num/2; i+=2) {
            res = ht_delete(hashtab, (void *)(long)pdata[i]);
            if (!res) printf("    HashTab del not found %d\n", pdata[i]);
            else {
                val = (int)(long)res;
                if (val != pdata[i]) printf("    HashTab del [No%d]%d not match %d\n", i, val, pdata[i]);
            }
        }
        btime(&time2);
        diff = btime_diff(&time1, &time2);
        ht_deleteres[ind-1] = (float)diff.s + (float)diff.ms/1000.;
        printf(" HashTab data remove: %d/%d, TotleTime: %ld.%03ld sec\n",
                num, ht_num(hashtab), diff.s, diff.ms);
 

        /* performance indicators of removing all data in skiplist */
        btime(&time1);
        for (i=0; i<num/2; i+=2) {
            res = skiplist_delete(skiplist, (void *)(long)pdata[i]);
            if (!res) printf("    Skiplist del not found %d\n", pdata[i]);
            else {
                val = (int)(long)res;
                if (val != pdata[i]) printf("    Skiplist del [No%d]%d not match %d\n", i, val, pdata[i]);
            }
        }
        btime(&time2);
        diff = btime_diff(&time1, &time2);
        skl_deleteres[ind-1] = (float)diff.s + (float)diff.ms/1000.;
        printf("Skiplist data remove: %d/%d, TotleTime: %ld.%03ld sec\n",
                num, skiplist_num(skiplist), diff.s, diff.ms);


        arr_free(arlist);
        arr_free(arlist2);
        rbtree_free(ptree);
        ht_free(hashtab);
        skiplist_free(skiplist);
        free(pdata);
    }

    /*printf("\n\n   NUM  |  AR-MEM  |  RB-MEM  |  HT-MEM  |  SK-MEM  |"
           " AR-ADD | AR-ADD2| RB-ADD | HT-ADD | SK-ADD |"
           " AR-SCH | AR-SCH2| RB-SCH | HT-SCH | SK-SCH |"
           " AR-DEL | RB-DEL | HT-DEL | SK-DEL |\n");
    for (ind=1; ind<argc; ind++) {
        printf("%8d|%10d|%10d|%10d|%10d|"
               "%8.3f|%8.3f|%8.3f|%8.3f|%8.3f|"
               "%8.3f|%8.3f|%8.3f|%8.3f|%8.3f|"
               "%8.3f|%8.3f|%8.3f|%8.3f|\n",
               atoi(argv[ind]),
               ar_allocmem[ind-1], rb_allocmem[ind-1], ht_allocmem[ind-1], skl_allocmem[ind-1],
               ar_insertres[ind-1], ar2_insertres[ind-1], rb_insertres[ind-1], ht_insertres[ind-1], skl_insertres[ind-1],
               ar_findres[ind-1], ar2_findres[ind-1], rb_findres[ind-1], ht_findres[ind-1], skl_findres[ind-1],
               ar_deleteres[ind-1], rb_deleteres[ind-1], ht_deleteres[ind-1], skl_deleteres[ind-1]);
    }
    printf("\n");*/

    printf("\n\nperformance indicators (memory consumed: bytes) of storing data:\n");
    printf(" DATA-NUM |  AR-MEM  |  RB-MEM  |  HT-MEM  |  SK-MEM  |\n");
    for (ind=1; ind<argc; ind++) {
        printf("%10d|%10d|%10d|%10d|%10d|\n",
               atoi(argv[ind]), ar_allocmem[ind-1], rb_allocmem[ind-1], ht_allocmem[ind-1], skl_allocmem[ind-1]);
    }
    printf("\n");

    printf("\nperformance indicators (time spent: seconds) of inserting data:\n");
    printf(" DATA-NUM | AR-ADD | AR-ADD2| RB-ADD | HT-ADD | SK-ADD |\n");
    for (ind=1; ind<argc; ind++) {
        printf("%10d|%8.3f|%8.3f|%8.3f|%8.3f|%8.3f|\n",
               atoi(argv[ind]),
               ar_insertres[ind-1], ar2_insertres[ind-1], rb_insertres[ind-1], ht_insertres[ind-1], skl_insertres[ind-1]);
    }
    printf("\n");

    printf("\nperformance indicators (time spent: seconds) of looking up data:\n");
    printf(" DATA-NUM | AR-SCH | AR-SCH2| RB-SCH | HT-SCH | SK-SCH |\n");
    for (ind=1; ind<argc; ind++) {
        printf("%10d|%8.3f|%8.3f|%8.3f|%8.3f|%8.3f|\n",
               atoi(argv[ind]),
               ar_findres[ind-1], ar2_findres[ind-1], rb_findres[ind-1], ht_findres[ind-1], skl_findres[ind-1]);
    }
    printf("\n");

    printf("\nperformance indicators (time spent: seconds) of removing data:\n");
    printf(" DATA-NUM | AR-DEL | RB-DEL | HT-DEL | SK-DEL |\n");
    for (ind=1; ind<argc; ind++) {
        printf("%10d|%8.3f|%8.3f|%8.3f|%8.3f|\n",
               atoi(argv[ind]),
               ar_deleteres[ind-1], rb_deleteres[ind-1], ht_deleteres[ind-1], skl_deleteres[ind-1]);
    }
    printf("\n");

    return 0;
}


