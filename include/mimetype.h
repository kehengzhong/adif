/*
 * Copyright (c) 2003-2021 Ke Hengzhong <kehengzhong@hotmail.com>
 * All rights reserved. See MIT LICENSE for redistribution.
 */

#ifndef _MIMETYPE_H_
#define _MIMETYPE_H_ 
    
#ifdef __cplusplus
extern "C" {
#endif 


typedef struct mime_item {
    uint32    mimeid;
    uint32    appid;
    char      extname[16];
    char      mime[96];
} MimeItem;

void * mime_type_init ();
int    mime_type_clean (void * pmime);

void * mime_type_alloc (int tabsize);
void   mime_type_free  (void * vmgmt);

int mime_type_add (void * vmgmt, char * mime, char * ext, uint32 mimeid, uint32 appid);

int mime_type_get_by_extname (void * vmtype, char * ext, char ** pmime, uint32 * mimeid, uint32 * appid);
int mime_type_get_by_mimeid  (void * vmtype, uint32 mimeid, char ** pmime, char ** pext, uint32 * appid);
int mime_type_get_by_mime    (void * vmtyp, char * mime, char ** pext, uint32 * mimeid, uint32 * appid);

void * mime_type_get (void * vmgmt, char * mime, uint32 mimeid, char * ext);


#ifdef __cplusplus
}
#endif 
 
#endif

