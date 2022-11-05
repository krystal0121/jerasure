#ifndef RDP_H
#define RDP_H

#ifdef __cplusplus
extern "C" {
#endif


/**
 * Do row-parity encode, write encoded data to coding_ptrs[0]
*/
void rdp_row_xor_encode(int p, char **data_ptrs, char **coding_ptr, int blocksize);

/**
 * Do diagnal-parity encode, write encoded data to coding_ptrs[01
*/
void rdp_diagnal_xor_encode(int p, char **data_ptrs, char **coding_ptr, int blocksize);

/**
 * blocksize: size of each block on disk, equal buffersize/k/k;
*/
extern void rdp_encode(int k, char **data_ptrs, char **coding_ptrs, int blocksize);

/**
 * k: encode data will write on k disks(files). in RDP, p=k+1
 * blocksize: size of each block on failed disk, euqal buffersize/k/k;
 * erasures: erasures[2]=-1, means disk[0] and disk[1] is failed. 
*/
extern int rdp_decode(int k, char **data_ptrs, char **coding_ptrs, int blocksize, int *erasures);

#ifdef __cplusplus
}
#endif

#endif