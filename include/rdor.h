#ifndef RDOR_H
#define RDOR_H

#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

/* generate one array(dicitonary) - methods 
 * if methods[0] = 1, means that the blocks[0] on failed disk will be recovered by diagnol-parity.
 * if methods[2] = 0, menas that the blocks[2] on failed disk will be recovered by row-parity.
 */
extern void rdor_decode_map(int p, int erased, int *methods);

/* decoe_map is an (p-1)*(p-1) array.
 * decode_map[1][0] means blocks[0] on disks[1].
 * the value of this map, represent its decode target, details as below:
 * if value is 0,  this block won't be read;
 * if value is 4,  read to recover blocks[4-1] by row-parity;
 * if value is 40, read to recover blcoks[4-1] by diagnol-parity;
 * if value is 43, read to recover blocks[4-1] by diagnol-parity and blocks[3-1] by row-parity.                  
 */
extern void rdor_decode_src_map(int p, int erased, int *methods, int **decode_map);

/*
 * do rdor_decode.
 * data_fnames: an array of name of filies storing encoded data of each disks
 */
extern int rdor_decode(int k, int readins, int buffersize, char** data_fnames, char** parity_fnames);


#ifdef __cplusplus
}
#endif

#endif