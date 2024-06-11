/*
 * Copyright (c) 2003-2024 Ke Hengzhong <kehengzhong@hotmail.com>
 * All rights reserved. See MIT LICENSE for redistribution.
 *
 * #####################################################
 * #                       _oo0oo_                     #
 * #                      o8888888o                    #
 * #                      88" . "88                    #
 * #                      (| -_- |)                    #
 * #                      0\  =  /0                    #
 * #                    ___/`---'\___                  #
 * #                  .' \\|     |// '.                #
 * #                 / \\|||  :  |||// \               #
 * #                / _||||| -:- |||||- \              #
 * #               |   | \\\  -  /// |   |             #
 * #               | \_|  ''\---/''  |_/ |             #
 * #               \  .-\__  '-'  ___/-. /             #
 * #             ___'. .'  /--.--\  `. .'___           #
 * #          ."" '<  `.___\_<|>_/___.'  >' "" .       #
 * #         | | :  `- \`.;`\ _ /`;.`/ -`  : | |       #
 * #         \  \ `_.   \_ __\ /__ _/   .-` /  /       #
 * #     =====`-.____`.___ \_____/___.-`___.-'=====    #
 * #                       `=---='                     #
 * #     ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~   #
 * #               佛力加持      佛光普照              #
 * #  Buddha's power blessing, Buddha's light shining  #
 * #####################################################
 */ 

#ifndef _MEMORY_H_
#define _MEMORY_H_

#ifdef __cplusplus
extern "C" {
#endif

void kmem_print ();

void kmem_alloc_init (size_t size);
void kmem_alloc_free ();


#define kosmalloc(size) kosmalloc_dbg(size, __FILE__, __LINE__)
#define koszmalloc(size) koszmalloc_dbg(size, __FILE__, __LINE__)
#define kosrealloc(ptr, size) kosrealloc_dbg(ptr, size, __FILE__, __LINE__)
#define kosfree(ptr) kosfree_dbg(ptr, __FILE__, __LINE__)
    
void * kosmalloc_dbg  (size_t size, char * file, int line);
void * koszmalloc_dbg (size_t size, char * file, int line);
void * kosrealloc_dbg (void * ptr, size_t size, char * file, int line);
void   kosfree_dbg    (void * ptr, char * file, int line);


void * kalloc_dbg   (size_t size, char * file, int line);
void * kzalloc_dbg  (size_t size, char * file, int line);
void * krealloc_dbg (void * ptr, size_t size, char * file, int line);
void   kfree_dbg    (void * ptr, char * file, int line);

void   kfreeit (void * ptr);

#define kalloc(size)        kalloc_dbg((size), __FILE__, __LINE__)
#define kzalloc(size)       kzalloc_dbg((size), __FILE__, __LINE__)
#define krealloc(ptr, size) krealloc_dbg((ptr), (size), __FILE__, __LINE__)
#define kfree(ptr)          kfree_dbg((ptr), __FILE__, __LINE__)

#ifdef __cplusplus
}
#endif

#endif

