
#ifndef _CHECK_SUM_H_
#define _CHECK_SUM_H_

#include "btype.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
   Update a running crc with the bytes buf[0..len-1] and return the updated
   crc. If buf is NULL, this function returns the required initial value
   for the crc. Pre- and post-conditioning (one's complement) is performed
   within this function so it shouldn't be done by the application.
   Usage example:

     uint32 crc = calcrc32(0L, NULL, 0);

     while (read_buffer(buffer, length) != EOF) {
       crc = calcrc32(crc, buffer, length);
     }
     if (crc != original_crc) error();
*/
uint32 calcrc32 (uint32 crc, uint8 * buf, uint32 len);


/*
   Update a running Adler-32 checksum with the bytes buf[0..len-1] and
   return the updated checksum. If buf is NULL, this function returns
   the required initial value for the checksum.
   An Adler-32 checksum is almost as reliable as a CRC32 but can be computed
   much faster. Usage example:

     uint32 adler = caladler32(0L, NULL, 0);

     while (read_buffer(buffer, length) != EOF) {
       adler = caladler32(adler, buffer, length);
     }
     if (adler != original_adler) error();
*/
uint32 caladler32 (uint32 adler, uint8 * buf, uint32 len);


#ifdef __cplusplus
}
#endif


#endif
