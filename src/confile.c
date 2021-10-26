/*  
 * Copyright (c) 2003-2021 Ke Hengzhong <kehengzhong@hotmail.com>
 * All rights reserved. See MIT LICENSE for redistribution. 
 */

#include "btype.h"
#include "memory.h"
#include "hashtab.h"
#include "dynarr.h"
#include "fileop.h"
#include "strutil.h"

#ifdef UNIX
#include <sys/stat.h>
#include <fcntl.h>
#endif

#include "confile.h"
 
hashtab_t * ht_global_conf = NULL;

#define DEFAULT_SEC  "default_section_broadreach_2001_02_12"

#define SEC_HASH_LEN   100
#define ITEM_HASH_LEN  300

typedef struct section {
    char      * name;
    hashtab_t * ht;
} Section;

typedef struct cfgItem {
    char * key;
    char * value;
} CFGItem;

#define CFGTYPE_NULL_LINE   0
#define CFGTYPE_COMMENT     1
#define CFGTYPE_SECTION     2
#define CFGTYPE_ITEM        3
#define CFGTYPE_SECTION_CMT 4
#define CFGTYPE_ITEM_CMT    5

typedef struct cfg_line {
    int       line;
    char    * rawstr;
    uint8     cfgtype;     /*0-null line, 1-comment, 2-section, 3-cfg item, 
                             4-section & include comment, 
                             5-cfg item & include comment*/
    char    * key;
    char    * value;
    char    * comment;

    hashtab_t * htsect;
} CFGLine;


void * cfgline_alloc ()
{
    CFGLine * line = NULL;

    line = kzalloc(sizeof(*line));
    return line;
}

int cfgline_free (void * vline)
{
    CFGLine * line = NULL;

    if (!vline) return -1;
    line = (CFGLine *)vline;

    if (line->rawstr) {
        kfree(line->rawstr);
        line->rawstr = NULL;
    }

    if (line->key) {
        kfree(line->key);
        line->key = NULL;
    }

    if (line->value) {
        kfree(line->value);
        line->value = NULL;
    }

    if (line->comment) {
        kfree(line->comment);
        line->comment = NULL;
    }

    if (line->htsect) {
        ht_free(line->htsect);
        line->htsect = NULL;
    }

    return 0;
}

int cfgline_cmp_key (void * a, void * b)
{
    CFGLine * cfgline = NULL;
    char    * key =  NULL;

    if (!a || !b) return -1;

    cfgline = (CFGLine *)a;
    key     = (char *)b;

    return strcasecmp(cfgline->key, key);
}


typedef struct _ConfMgmt {
    char        confile[128];
    arr_t     * line_list;

    CFGLine     default_sect;

    hashtab_t * sect_table;
} ConfMgmt;


void  * conf_mgmt_init (char * file)
{
    ConfMgmt * conf = NULL;

    conf = (ConfMgmt *)kzalloc(sizeof(*conf));
    if (!conf) return NULL;

    conf->line_list = arr_new(16);

    conf->sect_table = ht_only_new(SEC_HASH_LEN, cfgline_cmp_key);

    if (file && file_exist(file)) {
        strncpy(conf->confile, file, sizeof(conf->confile)-1);
        conf_mgmt_read (conf, conf->confile);
    }

    return conf;
}


int conf_mgmt_cleanup (void * vconf)
{
    ConfMgmt * conf = NULL;
    int        i, num;
    CFGLine  * line = NULL;

    if (!vconf) return -1;
    conf = (ConfMgmt *)vconf;

    cfgline_free(&conf->default_sect);

    num = arr_num(conf->line_list);
    for (i=0; i<num; i++) {
        line = (CFGLine *)arr_value(conf->line_list, i);
        if (line) {
            cfgline_free(line);
            kfree(line);
        }
    }
    arr_free(conf->line_list);

    ht_free(conf->sect_table);

    kfree(conf);
    return 0;
}

static char * conf_cmt_pos (char * str)
{
    int i, len;
    char * p = NULL;

    if (!str) return NULL;
    p = str;
    if (*p == ';' || *p == '#') return p;

    len = strlen(str);
    for (i=1; i<len; i++) {
        if (p[i]==';' || p[i]=='#') {
            if (p[i-1]==' ' || p[i-1]=='\t')
                return &p[i];
        }
    }

    return NULL;
}


int conf_mgmt_read (void * vconf, char * file)
{
    ConfMgmt * conf = NULL;
    FILE     * fp = NULL;
    char       buf[512], *piter=NULL, *pbuf=NULL, *pcmt=NULL;
    CFGLine  * line = NULL;
    int        seqno = 0;
    CFGLine  * sect = NULL;

    if (!vconf) return -1;
    conf = (ConfMgmt *)vconf;

    if (!file || !file_exist(file)) return -2;

    if (file != conf->confile)
        strncpy(conf->confile, file, sizeof(conf->confile)-1);

    fp = fopen(conf->confile, "r");
    if (!fp) return -3;

    sect = &conf->default_sect;

    for (seqno=0; !feof(fp); seqno++) {
        memset(buf, 0, sizeof(buf));

        fgets(buf, sizeof(buf)-1, fp);
        if (strlen(buf) < 1 && feof(fp)) break;
        piter = str_trim(buf);

        line = cfgline_alloc();
        line->line = seqno;
        line->rawstr = str_dup(piter, strlen(piter));

        arr_push(conf->line_list, line);

        if (strlen(piter) == 0) {
            line->cfgtype = CFGTYPE_NULL_LINE;
            continue;
        }
        if (*piter == ';' || *piter == '#') {
            line->cfgtype = CFGTYPE_COMMENT;
            continue;
        }

        pcmt = conf_cmt_pos(piter); 
        if (pcmt) {
            line->comment = str_dup(pcmt, strlen(pcmt));
        }

        if (*piter == '[') {
            pbuf = strchr(piter, ']');
            if (!pbuf) goto itemhandle;
            if (pcmt && pbuf > pcmt) goto itemhandle;

            if (pcmt) 
                line->cfgtype = CFGTYPE_SECTION_CMT;
            else
                line->cfgtype = CFGTYPE_SECTION;

            line->key = str_dup(piter+1, pbuf-piter-1);
            line->htsect = ht_only_new(ITEM_HASH_LEN, cfgline_cmp_key);
            sect = line;
         
            ht_set (conf->sect_table, line->key, line);

            continue;
        }

itemhandle:
        if (pcmt)
            line->cfgtype = CFGTYPE_ITEM_CMT;
        else
            line->cfgtype = CFGTYPE_ITEM;

        pbuf = strchr(piter, '=');
        if (pcmt && pbuf > pcmt) { //abc  #this = is comment
            *pcmt = '\0';
            pbuf = str_trim(piter);
            line->key = str_dup(pbuf, strlen(pbuf));
            continue;
        }

        *pbuf = '\0';
        pbuf++;

        piter = str_trim(piter);
        line->key = str_dup(piter, strlen(piter));

        if (pcmt) *pcmt = '\0';

        piter = str_trim(pbuf);
        line->value = str_dup(piter, strlen(piter));

        ht_set (sect->htsect, line->key, line);
    }

    fclose(fp);

    return 0;
}

int conf_mgmt_save (void * vconf, char * file)
{
    ConfMgmt * conf = NULL;
    FILE     * fp = NULL;
    int        i, num;
    CFGLine  * line = NULL;

    if (!vconf) return -1;
    conf = (ConfMgmt *)vconf;

    if (!file) file = &conf->confile[0];

    fp = fopen(file, "w");
    if (!fp) return -3;

    num = arr_num(conf->line_list);
    for (i = 0; i < num; i++) {

        line = (CFGLine *)arr_value(conf->line_list, i);
        if (!line) continue;

        if (line->cfgtype == CFGTYPE_NULL_LINE) {
            fprintf(fp, "\n");

        } else if (line->cfgtype == CFGTYPE_COMMENT) {
            fprintf(fp, "%s\n", line->rawstr);

        } else if (line->cfgtype == CFGTYPE_ITEM_CMT) {
            fprintf(fp, "%s = %s  %s\n", line->key, line->value, line->comment);

        } else if (line->cfgtype == CFGTYPE_ITEM) {
            fprintf(fp, "%s = %s\n", line->key, line->value);

        } else if (line->cfgtype == CFGTYPE_SECTION_CMT) {
            fprintf(fp, "[%s]  %s\n", line->key, line->comment);

        } else if (line->cfgtype == CFGTYPE_SECTION) {
            fprintf(fp, "[%s]\n", line->key);

        } else {
            continue;
        }
    }

    fclose(fp);

    return 0;
}

char * conf_get_string (void * vconf, char * sect, char * key)
{
    ConfMgmt * conf = NULL;
    CFGLine  * sectnode = NULL;
    CFGLine  * line = NULL;

    if (!vconf || !key) return NULL;
    conf = (ConfMgmt *)vconf;

    if (!sect) sectnode = &conf->default_sect;
    else      sectnode = (CFGLine *)ht_get(conf->sect_table, sect);

    if (!sectnode) return NULL;

    line = ht_get(sectnode->htsect, key);
    if (!line) return NULL;

    return line->value;
}


int conf_get_int (void * vconf, char * sect, char * key)
{
    ConfMgmt * conf = NULL;
    CFGLine  * sectnode = NULL;
    CFGLine  * line = NULL;
    int        val = 0;

    if (!vconf || !key) return -1;
    conf = (ConfMgmt *)vconf;

    if (!sect) sectnode = &conf->default_sect;
    else      sectnode = (CFGLine *)ht_get(conf->sect_table, sect);

    if (!sectnode) return -1;

    line = ht_get(sectnode->htsect, key);
    if (!line) return -1;

    if (strlen(line->value) >= 2 && strncasecmp(line->value, "0x", 2) == 0)
        val = strtol(line->value+2, NULL, 16);
    else
        val = atoi(line->value);

    return val;
}


uint32 conf_get_ulong (void * vconf, char * sect, char * key)
{
    ConfMgmt * conf = NULL;
    CFGLine  * sectnode = NULL;
    CFGLine  * line = NULL;

    if (!vconf || !key) return 0;
    conf = (ConfMgmt *)vconf;

    if (!sect) sectnode = &conf->default_sect;
    else      sectnode = (CFGLine *)ht_get(conf->sect_table, sect);

    if (!sectnode) return 0;

    line = ht_get(sectnode->htsect, key);
    if (!line) return 0;

    if (strlen(line->value) >= 2 && strncasecmp(line->value, "0x", 2) == 0)
        return strtol(line->value + 2, NULL, 16);
    else
        return strtoul(line->value, (char **)NULL, 10);
}

long conf_get_hexlong (void * vconf, char * sect, char * key)
{
    ConfMgmt * conf = NULL;
    CFGLine  * sectnode = NULL;
    CFGLine  * line = NULL;

    if (!vconf || !key) return -1;
    conf = (ConfMgmt *)vconf;

    if (!sect) sectnode = &conf->default_sect;
    else      sectnode = (CFGLine *)ht_get(conf->sect_table, sect);

    if (!sectnode) return -1;

    line = ht_get(sectnode->htsect, key);
    if (!line) return -1;

    return strtol(line->value, (char **)NULL, 16);
}

double conf_get_double (void * vconf, char * sect, char * key)
{
    ConfMgmt * conf = NULL;
    CFGLine  * sectnode = NULL; 
    CFGLine  * line = NULL;
    
    if (!vconf || !key) return 0.;
    conf = (ConfMgmt *)vconf;

    if (!sect) sectnode = &conf->default_sect;
    else      sectnode = (CFGLine *)ht_get(conf->sect_table, sect);

    if (!sectnode) return 0.;

    line = ht_get(sectnode->htsect, key);
    if (!line) return 0.; 

    return strtod(line->value, (char **)NULL);
}

uint8 conf_get_bool (void * vconf, char * sect, char * key)
{
    ConfMgmt * conf = NULL;
    CFGLine  * sectnode = NULL;
    CFGLine  * line = NULL;
   
    if (!vconf || !key) return 0;
    conf = (ConfMgmt *)vconf;

    if (!sect) sectnode = &conf->default_sect;
    else      sectnode = (CFGLine *)ht_get(conf->sect_table, sect);

    if (!sectnode) return 0;

    line = ht_get(sectnode->htsect, key);
    if (!line) return 0;

    if (strcasecmp(line->value, "yes") == 0 ||
        strcasecmp(line->value, "true") == 0 ||
        strcasecmp(line->value, "1") == 0)
        return 1;

    return 0;
}

int conf_set_string (void * vconf, char * sect, char * key, char * value)
{
    ConfMgmt * conf = NULL;
    CFGLine  * sectnode = NULL;
    CFGLine  * line = NULL;

    if (!vconf || !key) return -1;
    conf = (ConfMgmt *)vconf;

    if (!sect) sectnode = &conf->default_sect;
    else      sectnode = (CFGLine *)ht_get(conf->sect_table, sect);

    if (!sectnode) return -2;

    line = ht_get(sectnode->htsect, key);
    if (!line) return -3;

    if (line->value) {
        kfree(line->value);
        line->value = NULL;
    }
    if (value)
        line->value = str_dup(value, strlen(value));

    return 0;
}

int conf_set_int (void * vconf, char * sect, char * key, int value)
{
    ConfMgmt * conf = NULL;
    CFGLine  * sectnode = NULL;
    CFGLine  * line = NULL;
    char       valstr[32];

    if (!vconf || !key) return -1;
    conf = (ConfMgmt *)vconf;

    if (!sect) sectnode = &conf->default_sect;
    else      sectnode = (CFGLine *)ht_get(conf->sect_table, sect);

    if (!sectnode) return -2;

    line = ht_get(sectnode->htsect, key);
    if (!line) return -3;

    if (line->value) { 
        kfree(line->value);
        line->value = NULL;
    }
    snprintf(valstr, sizeof(valstr)-1, "%d", value);

    line->value = str_dup(valstr, strlen(valstr));
    return 0;
}


int conf_set_ulong (void * vconf, char * sect, char * key, uint32 value)
{
    ConfMgmt * conf = NULL;
    CFGLine  * sectnode = NULL;
    CFGLine  * line = NULL;
    char       valstr[32];

    if (!vconf || !key) return -1;
    conf = (ConfMgmt *)vconf;

    if (!sect) sectnode = &conf->default_sect;
    else      sectnode = (CFGLine *)ht_get(conf->sect_table, sect);

    if (!sectnode) return -2;

    line = ht_get(sectnode->htsect, key);
    if (!line) return -3; 

    if (line->value) {
        kfree(line->value);
        line->value = NULL;
    }   
    snprintf(valstr, sizeof(valstr)-1, "%u", value);

    line->value = str_dup(valstr, strlen(valstr));
    return 0;
}


int conf_set_hexlong (void * vconf, char * sect, char * key, long value)
{
    ConfMgmt * conf = NULL;
    CFGLine  * sectnode = NULL;
    CFGLine  * line = NULL;
    char       valstr[32];

    if (!vconf || !key) return -1;
    conf = (ConfMgmt *)vconf;

    if (!sect) sectnode = &conf->default_sect;
    else      sectnode = (CFGLine *)ht_get(conf->sect_table, sect);

    if (!sectnode) return -2;

    line = ht_get(sectnode->htsect, key);
    if (!line) return -3; 

    if (line->value) {
        kfree(line->value);
        line->value = NULL;
    }
    snprintf(valstr, sizeof(valstr)-1, "0X%lX", value);

    line->value = str_dup(valstr, strlen(valstr));
    return 0;
}


int conf_set_double (void * vconf, char * sect, char * key, double value)
{
    ConfMgmt * conf = NULL;
    CFGLine  * sectnode = NULL;
    CFGLine  * line = NULL;
    char       valstr[32];

    if (!vconf || !key) return -1;
    conf = (ConfMgmt *)vconf;

    if (!sect) sectnode = &conf->default_sect;
    else      sectnode = (CFGLine *)ht_get(conf->sect_table, sect);

    if (!sectnode) return -2;

    line = ht_get(sectnode->htsect, key);
    if (!line) return -3; 

    if (line->value) {
        kfree(line->value);
        line->value = NULL;
    }   

    snprintf(valstr, sizeof(valstr)-1, "%f", value);

    line->value = str_dup(valstr, strlen(valstr));
    return 0;
}


int conf_set_bool(void * vconf, char * sect, char * key, uint8 value)
{
    ConfMgmt * conf = NULL;
    CFGLine  * sectnode = NULL;
    CFGLine  * line = NULL;
    char       valstr[32];

    if (!vconf || !key) return -1;
    conf = (ConfMgmt *)vconf;

    if (!sect) sectnode = &conf->default_sect;
    else      sectnode = (CFGLine *)ht_get(conf->sect_table, sect);

    if (!sectnode) return -2;

    line = ht_get(sectnode->htsect, key);
    if (!line) return -3; 

    if (line->value) {
        kfree(line->value);
        line->value = NULL;
    }   

    if (value)
        snprintf(valstr, sizeof(valstr)-1, "yes");
    else
        snprintf(valstr, sizeof(valstr)-1, "no");

    line->value = str_dup(valstr, strlen(valstr));

    return 0;
}

