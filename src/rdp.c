#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <gf_complete.h>
#include "galois.h"
#include "rdp.h"


void rdp_row_xor_encode(int p, char **data_ptrs, char **coding_ptrs, int blocksize) {
    int i;

    /* put the data of first 'node' into coing region 0 */
    memcpy(coding_ptrs[0], data_ptrs[0], blocksize*(p-1));

    /* put the XOR results into coding region 0 */
    for (i = 1; i < p-1; i++) {
        galois_region_xor(data_ptrs[i], coding_ptrs[0], blocksize*(p-1));
    }
}

void rdp_diagnal_xor_encode(int p, char **data_ptrs, char **coding_ptrs, int blocksize) {
    int i, j;
    char *src, *dest;
      
    memcpy(coding_ptrs[1], data_ptrs[0], blocksize*(p-1));

    for (i = 1; i < p; i++) {
        for(j = 0; j < p-1; j++){
            if(i+j == p-1) continue;

            dest = coding_ptrs[1] + (((i + j) % p) * blocksize);
            if(i == (p-1)){
                src = coding_ptrs[0] + (j*blocksize);
            }
            else{
                src = data_ptrs[i] + (j*blocksize);
            }
            galois_region_xor(src, dest, blocksize);
        }
   }
}

void rdp_encode(int k, char **data_ptrs, char **coding_ptrs, int blocksize){
    int p = k+1;

    rdp_row_xor_encode(p, data_ptrs, coding_ptrs, blocksize);
    rdp_diagnal_xor_encode(p, data_ptrs, coding_ptrs, blocksize);
}

int rdp_decode(int k, char **data_ptrs, char **coding_ptrs, int blocksize, int *erasures){
    int i, p, numerased, erased;
    
    p = k+1;
    numerased = 0;
    while (erasures[numerased] != -1) {
        numerased++;
        if ((numerased > 1 && erasures[numerased-1] < p) ) {
            fprintf(stderr, "RDP can work for below situations:\n");
            fprintf(stderr, "   1. one data disk failed \n");
            fprintf(stderr, "   2. row-parity disk failed \n");
            fprintf(stderr, "   3. diagnol-parity disk failed \n");
            fprintf(stderr, "   4. one data disk and diagnol-parity disk failed \n");
            return -1;
        }
    }

    if (numerased == 0){
        fprintf(stderr, "There is no disk need to recover.\n");
        return -1;
    }

    /* start rebuilding */
    /* Check whether the failed disk is a data disk or a parity disk*/
    i = 0;
    while (i < numerased) {
        erased = erasures[i];
        if (erased < p-1) {
            /* rebuild data disk */
            memcpy(data_ptrs[erased], coding_ptrs[0], blocksize*k);
            for (int i = 0; i < p-1; i++) {
                if (i == erased) continue;
                galois_region_xor(data_ptrs[i], data_ptrs[erased], blocksize*k);
            }
        }
        else if (erased == p-1) {
            /* rebuild row-xor parity disk */
            rdp_row_xor_encode(p, data_ptrs, coding_ptrs, blocksize);
        }
        else {
            /* rebuild diagnol-xor parity disk */
            rdp_diagnal_xor_encode(p, data_ptrs, coding_ptrs, blocksize);
        }
        i++;
    }
    return 0;
}
