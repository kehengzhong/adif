
#include <sys/mman.h>
#include "adifall.ext"

//#define PRNMAT 1

int main (int argc, char ** argv)
{
    pat_bitvec_t * bitvec = NULL;
    pat_kmpvec_t * kmpvec = NULL;
    pat_bmvec_t  * bmvec = NULL;
    pat_sunvec_t * sunvec = NULL;
    pat_sunvec_t * rsunvec = NULL;
    long           bitms = 0;
    long           kmpms = 0;
    long           bmms = 0;
    long           strms = 0;
    long           str2ms = 0;
    long           rkms = 0;
    long           sunms = 0;
    long           rsunms = 0;
    btime_t        t0, t1;
    void         * pbyte = NULL;
    struct stat    st;
    int            fd;
    char         * pstr = NULL;
    char         * pbgn = NULL;
    char         * pend = NULL;
#ifdef PRNMAT
    char           cont[128];
#endif
    int            i;

    if (argc < 3) {
        printf("Usage: %s <pattern> <file1> <file2>...\n", argv[0]);
        return 0;
    }

    bitvec = pat_bitvec_alloc(argv[1], strlen(argv[1]), 0);
    if (!bitvec) {
        printf("pattern %s build bit vector failed\n", argv[1]);
        return -10;
    }

    kmpvec = pat_kmpvec_alloc(argv[1], strlen(argv[1]), 0, 0);
    if (!kmpvec) {
        printf("pattern %s build kmp vector failed\n", argv[1]);
        return -10;
    }

    bmvec = pat_bmvec_alloc(argv[1], strlen(argv[1]), 0);
    if (!bmvec) {
        printf("pattern %s build bm vector failed\n", argv[1]);
        return -10;
    }

    sunvec = pat_sunvec_alloc(argv[1], strlen(argv[1]), 0, 0);
    if (!sunvec) {
        printf("pattern %s build sunday vector failed\n", argv[1]);
        return -10;
    }

    rsunvec = pat_sunvec_alloc(argv[1], strlen(argv[1]), 0, 1);
    if (!rsunvec) {
        printf("pattern %s build reverse sunday vector failed\n", argv[1]);
        return -10;
    }

    for (i = 2; i < argc; i++) {
        fd = open(argv[i], O_RDONLY);
        if (fd < 0) {
            printf("file %s open failed\n", argv[i]);
            break;
        }

        fstat(fd, &st);

        pbyte = mmap(NULL, st.st_size, PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, 0);
        if (!pbyte) {
            printf("mmap %s failed\n", argv[i]);
            perror("mmap fialed:");
            close(fd);
            continue;
        }

	pbgn = pbyte;
        pend = pbyte + st.st_size;

        btime(&t0);
        for (pstr = pbyte; pstr != NULL; ) {
            pstr = str_str(pstr, pend - pstr, argv[1], strlen(argv[1]));
            if (pstr) {
#ifdef PRNMAT
                printf("       str_str: '%s' found in %ld of file %s\n",
                       argv[1], pstr - (char *)pbyte, argv[i]);
 
                strncpy(cont, pstr, 48); cont[48] = '\0';
                //printf("content: %s...\n", cont);
#endif 
                pstr += strlen(argv[1]);
            }
        }
        btime(&t1);
        strms += btime_diff_ms(&t0, &t1);
 
        btime(&t0);
        for (pstr = pbyte; pstr != NULL; ) {
            pstr = str_rk_find(pstr, pend - pstr, argv[1], strlen(argv[1]));
            if (pstr) {
#ifdef PRNMAT
                printf("   str_rk_find: '%s' found in %ld of file %s\n",
                       argv[1], pstr - (char *)pbyte, argv[i]);
 
                strncpy(cont, pstr, 48); cont[48] = '\0';
                //printf("content: %s...\n", cont);
#endif
                pstr += strlen(argv[1]);
            }
        }
        btime(&t1);
        rkms += btime_diff_ms(&t0, &t1);

        btime(&t0);
        for (pstr = pbyte; pstr != NULL; ) {
            pstr = str_str2(pstr, pend - pstr, argv[1], strlen(argv[1]));
            if (pstr) {
#ifdef PRNMAT
                printf("      str_str2: '%s' found in %ld of file %s\n",
                       argv[1], pstr - (char *)pbyte, argv[i]);
 
                strncpy(cont, pstr, 48); cont[48] = '\0';
                //printf("content: %s...\n", cont);
#endif 
                pstr += strlen(argv[1]);
            }
        }
        btime(&t1);
        str2ms += btime_diff_ms(&t0, &t1);


        btime(&t0);
        for (pstr = pbyte; pstr != NULL; ) {
            pstr = kmp_find_bytes(pstr, pend - pstr, argv[1], strlen(argv[1]), kmpvec);
            if (pstr) {
#ifdef PRNMAT
                printf("           kmp: '%s' found in %ld of file %s\n",
                       argv[1], pstr - (char *)pbyte, argv[i]);
 
                strncpy(cont, pstr, 48); cont[48] = '\0';
                //printf("content: %s...\n", cont);
#endif
 
                pstr += strlen(argv[1]);
            }
        }
        btime(&t1);
        kmpms += btime_diff_ms(&t0, &t1);

        btime(&t0);
        for (pstr = pbyte; pstr != NULL; ) {
            pstr = sun_find_bytes(pstr, pend - pstr, argv[1], strlen(argv[1]), sunvec);
            if (pstr) {
#ifdef PRNMAT
                printf("        sunday: '%s' found in %ld of file %s\n",
                       argv[1], pstr - (char *)pbyte, argv[i]);
 
                strncpy(cont, pstr, 48); cont[48] = '\0';
                //printf("content: %s...\n", cont);
#endif 
                pstr += strlen(argv[1]);
            }
        }
        btime(&t1);
        sunms += btime_diff_ms(&t0, &t1);

        btime(&t0);
        for (pstr = pend; pstr != NULL; ) {
            pstr = sun_rfind_bytes(pbgn, pstr- pbgn, argv[1], strlen(argv[1]), rsunvec);
            if (pstr) {
#ifdef PRNMAT
                printf("reverse sunday: '%s' found in %ld of file %s\n",
                       argv[1], pstr - (char *)pbyte, argv[i]);
 
                strncpy(cont, pstr, 48); cont[48] = '\0';
                //printf("content: %s...\n", cont);
#endif 
            }
        }
        btime(&t1);
        rsunms += btime_diff_ms(&t0, &t1);

        btime(&t0);
        for (pstr = pbyte; pstr != NULL; ) {
            pstr = bm_find_bytes(pstr, pend - pstr, argv[1], strlen(argv[1]), bmvec);
            if (pstr) {
#ifdef PRNMAT
                printf("            bm: '%s' found in %ld of file %s\n",
                       argv[1], pstr - (char *)pbyte, argv[i]);
 
                strncpy(cont, pstr, 48); cont[48] = '\0';
                //printf("content: %s...\n", cont);
#endif 
                pstr += strlen(argv[1]);
            }
        }
        btime(&t1);
        bmms += btime_diff_ms(&t0, &t1);

        /* BIT parallel algorithm by BitString Co. */
        btime(&t0);
        for (pstr = pbyte; pstr != NULL; ) {
            pstr = shift_and_find(pstr, pend - pstr, argv[1], strlen(argv[1]), bitvec);
            if (pstr) {
#ifdef PRNMAT
                printf("           bit: '%s' found in %ld of file %s\n",
                        argv[1], pstr - (char *)pbyte, argv[i]);

                strncpy(cont, pstr, 48); cont[48] = '\0';
                //printf("content: %s...\n", cont);
#endif
                pstr += strlen(argv[1]);
            }
        }
        btime(&t1);
        bitms += btime_diff_ms(&t0, &t1);

        munmap(pbyte, st.st_size);

        close(fd);
    }

    printf("pattern algorithm comparing:\n"
           "     shift-and time: %ldms\n"
           "           kmp time: %ldms\n"
           "    boyer-more time: %ldms\n"
           "       str_str time: %ldms\n"
           "   str_rk_find time: %ldms\n"
           "      str_str2 time: %ldms\n"
           "        sunday time: %ldms\n"
           "reverse sunday time: %ldms\n",
           bitms, kmpms, bmms, strms, rkms, str2ms, sunms, rsunms);

    pat_bmvec_free(bmvec);
    pat_kmpvec_free(kmpvec);
    pat_bitvec_free(bitvec);
    pat_sunvec_free(sunvec);
    pat_sunvec_free(rsunvec);

    return 0;
}

