#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <gf_complete.h>
#include <math.h>
#include "assert.h"
#include "galois.h"
#include "rdp.h"

enum Decode_Method {Row, Diagnol};

void rdor_decode_methods(int p, int erased, int *methods) {
    int *diagset;
    int i, v, flag;            // temporary var.

    diagset = (int *)malloc(sizeof(int)*p);

    /* initial */
    for (i = 0; i < p-1; i++){
        methods[i] = Row;
        diagset[i+1] = 0;
    }
    
    flag = 0;
    for (i = 1; i <= (p-1)/2; i++) {
        v = (i*i)%p;
        diagset[v] = 1;     // belong to Sp
        if (erased == v) flag = 1;   
    }
    
    /* A = Sp' - (erased+1) */
    if (flag == 1) {
        for (i = 1; i < p; i++) {
            if (diagset[i] != 1) {  // Sp'
                v = (i - erased -1) % p;
                v = (v < 0)?(v+p):v;
                methods[v] = Diagnol;
            }
        }
    }
    /* A = Sp - (erased+1) */
    else {
        for (i = 1; i < p+1; i++) {
            if (diagset[i] == 1) {  // Sp'
                v = (i - erased -1) % p;
                v = (v < 0)?(v+p):v;
                methods[v] = Diagnol;
            }
        }
    }
    
    free(diagset);
    return;
}

void rdor_decode_src_map(int p, int erased, int *methods, int **decode_map) {
    int i, r; // i is the index of disks, r is the index of blocks.
    int encode_target, decode_target;

    if (erased == p) {
        /* diagnol-parity disk is failed, do encode*/
        for (i = 0; i < p; i++) {
            for (r = 0; r < p-1; r++) {
                *(decode_map[i]+r) = ((r+i)%p+1)*10;
            }
        }
        return;
    }

    /* decode_map is an (p-1)*(p+1) array*/
    for (i = 0; i < p+1; i++){
        for (r = 0; r < p-1; r++) {
            if (i == erased) {
                *(decode_map[i]+r) = 0;
                continue;
            };
            /* initialization */
            *(decode_map[i]+r) = 0;

            if (*(methods + r) == Row && i < p) {
                /* read to recover by row-parity */
                *(decode_map[i]+r) += (r+1);
            }

            /* didn't used to do diagol-parity encode.*/
            if ((i+r) == (p-1))continue;

            encode_target = (i+r)%p;
            decode_target = encode_target-erased;
            if (decode_target < 0) {
                decode_target += p;
            }
            if (*(methods + decode_target) == Diagnol) {
                /* read to recover by diagnol-parity */

                *(decode_map[i]+r) += (decode_target+1)*10;
            }   
        }
    }
}

int rdor_decode(int k, int readins, int buffersize, char** data_fnames, char** parity_fnames) {
    int p, erased, blocksize;
    int *methods;       // decode method of each blocks on faild disk
    int **decode_map;   // the failed blocks corresponding to each surviving block
    char *block;         // surviving block
    char *data;         // decoded data
    char *fname;        // decode file name
    int targets, 
        rtarget_idx,
        dtarget_idx; 
    int  i, r, n;       // loop variables
    FILE *fp;

    p = k+1;
    blocksize = buffersize/k/k;

    /* check files, find failed disk */
    erased = -1;
    for(i=0; i < k; i++) {
        fp = fopen(data_fnames[i], "rb");
        if (fp == NULL) {
            if (erased != -1) {
                erased = -2;
                break;
            }
            erased = i;
        }
    }
    for(i=0; i < 2; i++) {
        fp = fopen(parity_fnames[i], "rb");
        if (fp == NULL) {
            if (erased != -1) {
                erased = -2;
                break;
            }
            erased = i+k;
        }
    }
    if (erased == -1) {
        fprintf(stderr, "There is no failed disk need to recover.\n");
        return -1;
    }
    if (erased == -2) {
        fprintf(stderr, "RDOR only support the recovery of single disk failure.\n");
        return -1;
    }
    if (erased == p) {
        fprintf(stderr, "Failed disk is the diagnol-parity disk, please switch to RDP.\n");
        return -1;
    }

    /* allocate memory */
    methods = (int *)malloc(sizeof(int)*k);
    decode_map = (int **)malloc((p+1) *sizeof(int*));
    for (int i=0; i < p+1; i++) {
        decode_map[i] = (int *)malloc(sizeof(int)*k);
    }
    block = (char *)malloc(sizeof(char)*blocksize);
    data = (char *)malloc(sizeof(char)*blocksize*k);

    rdor_decode_methods(p, erased, methods);
    rdor_decode_src_map(p, erased, methods, decode_map);

    if (erased < k) {
        fname = (char *)malloc(sizeof(char)*strlen(data_fnames[erased])+10);
        sprintf(fname, "%s.%s", data_fnames[erased], "decode");
    }
    else {
        fname = (char *)malloc(sizeof(char)*strlen(parity_fnames[erased-k])+10);
        sprintf(fname, "%s.%s", parity_fnames[erased-k], "decode");
    }
    
    /* start recovery */
    n = 1;
    while (n <= readins) {
        memset(data, 0, blocksize*k);
        for (i=0; i < p+1; i++) {
            if (i == erased) continue;
            if (i < p-1) {
                fp = fopen(data_fnames[i], "rb");
            }
            else {
                fp = fopen(parity_fnames[i-k], "rb");
            }
            for (r=0; r < k; r++) {
                targets = *(decode_map[i] + r);
                if (targets == 0) continue;

                fseek(fp, blocksize*(k*(n-1)+r), SEEK_SET);
                assert(blocksize == fread(block, sizeof(char), blocksize, fp));

                /* index of corresponding target block decoded by row-parity */
                rtarget_idx = targets%10-1;

                /* index of corresponding target block decoded by diagnol-parity */
                dtarget_idx = floor(targets/10)-1;

                /* row-parity-first */
                if (rtarget_idx >= 0) { 
                    galois_region_xor(block, data + rtarget_idx*blocksize, blocksize);
                }
                /* diagnol-partiy */
                if (dtarget_idx >= 0) {
                    galois_region_xor(block, data+dtarget_idx*blocksize, blocksize);
                }
            }
            fclose(fp);
        }
        /* write decode file*/
        if (n == 1){
            fp = fopen(fname, "wb");
        }
        else {
            fp = fopen(fname, "ab");
        }
        fwrite(data, sizeof(char), blocksize*k, fp);
        fclose(fp);
        n++;
    }

    free(methods);
    free(decode_map);
    free(block);
    free(data);

    return 0;

}