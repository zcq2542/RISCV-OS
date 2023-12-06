#include "fs.h"
#include "string.h"
#include "stdlib.h"

/* bit manipulation
 * see implementation in fs.c */
int bit_test(unsigned char *map, int i);

/* disk read/write
 * see implementation in fs.c */
int block_read(int block_no, void* dst);
int block_write(int block_no, void* src);

/* block allocate and free
 * see implementation in fs.c */
int alloc_block();
void free_block(int blk_id);

/* global fs_t instance */
fs_t fs;

// [lab6-ex1] initialize fs
// This function reads superblock and initialize the global fs variable "fs".
//  -- Read the definition of "fs_t" and "super_t" in "fs.h"
//  -- you should read superblock from disk by "block_read"
//  -- you should check the magic number; if not match, FATAL
//  -- you should update the "total_blk"
//  -- you should update the "avail_blks" by counting empty blocks in bitmap
//     (bit_test is a helper function to count)
void fs_init(m_uint32 super_blk_id) {
    // TODO: your code here
    //FATAL("fs_init is not implemented");
    //struct fs_super* dst = (struct fs_super*) malloc(sizeof(struct fs_super));
    super_t super;
    super_t* dst = &super;
    //printf("dst: %x \n", dst);
    
    block_read(super_blk_id, dst);
    //printf("bloack_read\n");
    
    if(dst->magic != 0x6640) FATAL("magic num is not 0x6640");
    fs.total_blks = dst->disk_size;
    //printf("fs.total_blks: %d\n", fs.total_blks);
    
    block_read(BITMAP_BLOCK_ID, fs.bitmap);
    int count = 0;
    for(int i = 0; i < fs.total_blks; ++i){
        if(bit_test(fs.bitmap, i) == 0) {
            //printf("i= %d, used: %d\t", i, 0);
            ++count;
        }
        else{
            //printf("i= %d, used: %d\t", i, 1);
        }
    }
    fs.avail_blks = count;
    //printf("fs.avail_blks: %d\n", fs.avail_blks);
    //free(dst);
}

// [lab6-ex2]
// This function reads inodes from disk.
// You should load a block from the inode arrary on disk
// which contains the inode "ino":
//   -- calculate which block contains inode "ino"
//      (note: "ino" starts from FS_ROOT_INODE)
//   -- load the block to fs.buffer_cache
//      (note: inode array starts at block INODEARR_BLOCK_START)
//   -- return the inode pointer (pointing in fs.buffer_cache)
inode_t *load_inode(int ino) {
    // TODO: your code here
    int INODES_PER_BLOCK = BLOCK_SIZE / sizeof(inode_t);
    //printf("INODES_PER_BLOCK: %d\n", INODES_PER_BLOCK);
    // Calculate the block number where the inode resides
    // Subtracting FS_ROOT_INODE since ino starts from FS_ROOT_INODE
    int block_num = (ino - FS_ROOT_INODE) / INODES_PER_BLOCK + INODEARR_BLOCK_START;
    //printf("ino block_num: %d\n", block_num);
    // Load the block into fs.buffer_cache
    int res = block_read(block_num, fs.buffer_cache);

    if (res == 1) {
        // Handle error if block couldn't be loaded
        FATAL("read_block error");
    }

    fs.buffer_blk_id = block_num; // keep the blk_no for flush

    // Calculate the offset of the inode within the block
    int inode_offset = ((ino - FS_ROOT_INODE) % INODES_PER_BLOCK) * sizeof(inode_t);
    //printf("inode_offset: %d\n", inode_offset);
    /*
    inode_t* inodp = (inode_t *)(fs.buffer_cache);
    
    for(int i = 0; i < INODES_PER_BLOCK; ++i){
        printf("i: %d\n", i);
        printf("size: %zu\n", inodp[i].size);
        printf("mode: %zu\n", inodp[i].mode);
    }
    */
    // Return the inode pointer
    return (inode_t *)(fs.buffer_cache + inode_offset);   
}

// [lab6-ex2]
// flush the contents of "fs.buffer_cache" (a list of inodes) back to disk
// notes:
//   -- you need to remember the block id of the cached inodes
//   -- later when you update files, remember to flush inodes when things change
void flush_inode() {
    // TODO: your code here
    int res = block_write(fs.buffer_blk_id, fs.buffer_cache);
    //printf("flush_inode res: %d\n", res);

}

int get_fin_data_blk(int block_num, inode_t* inodep){
    int final_blk_num = 0;
    if(block_num >= NUM_PTRS){ // in indirected blocks
        //printf("in indirected blocks\n");
        int indirec_blk[BLOCK_SIZE / sizeof(int)] = {};
        //int* indirec_blk = (int*)malloc(BLOCK_SIZE);
        int res = block_read(inodep->indirect_ptr, indirec_blk);
        if(res != 0) return -1;
        int indir_idx = block_num - NUM_PTRS;
        final_blk_num = indirec_blk[indir_idx];
        //free(indirec_blk);
    }
    else{
        final_blk_num = inodep->ptrs[block_num];
    }
    return final_blk_num;
}

int set_fin_data_blk(int block_num, inode_t* inodep){
    int final_blk_num = 0;
    if(block_num >= NUM_PTRS){ // in indirected blocks
        printf("set indirected blocks\n");
        if((inodep->indirect_ptr) == 0) {
            inodep->indirect_ptr = (m_uint32)alloc_block(); // alloc indir blk
            //bit_set(fs.bitmap, inodep->indirect_ptr); // bit set already in alloc
            //printf("inodep->indirect_ptr: %d\n", inodep->indirect_ptr);
        }
        int indirec_blk[BLOCK_SIZE / sizeof(int)] = {};
        //int* indirec_blk = (int*)malloc(BLOCK_SIZE);
        int res = block_read(inodep->indirect_ptr, indirec_blk);
        if(res != 0) FATAL("block_read(inodep->indirect_ptr, indirec_blk)");
        
        int indir_idx = block_num - NUM_PTRS;
        indirec_blk[indir_idx] = alloc_block(); // alloc data block
        block_write(inodep->indirect_ptr, indirec_blk); // flush
        //printf("indirec_blk[indir_idx]: %d\n", indirec_blk[indir_idx]);
        final_blk_num = indirec_blk[indir_idx];
        //free(indirec_blk);
    }
    else{ // direct (data)blk
        inodep->ptrs[block_num] = alloc_block();
        //printf("inodep->ptrs[block_num]:  %d\n", inodep->ptrs[block_num]);
        final_blk_num = inodep->ptrs[block_num];
    }
    return final_blk_num;
}

// [lab6-ex2]
// read "len" bytes of contents starting from "offset"
// in the file whose inode number is "ino".
// You should:
//   -- load the inode data structure
//   -- calculate which blocks (directed or indirected) contain the wanted data
//   -- copy the data to "buf"
//   -- return 0 if success; otherwise, return -1
// notes:
//   -- assume "buf" is the size of "len"
//   -- for simplicity, "len" will not exceed 1 block
int fs_read(int ino, int offset, int len, char *buf) {
    ASSERTX(len <= BLOCK_SIZE);
    // TODO: your code here
    // FATAL("fs_read is not implemented");
    //printf("ino: %d, offset: %d, len: %d, buf: %x\n", ino, offset, len, buf);
    inode_t* inodep = load_inode(ino);
    //printf("inodep: %x\n", inodep);
    //printf("inodep->size: %zu\n", inodep->size);
    /*
    if (offset + len > inodep->size) {
        return -1; // Read request exceeds file size
    }*/
    int block_num = offset / BLOCK_SIZE;
    int block_num2 = 0;
    int offset_in_block = offset % BLOCK_SIZE;
    int need2blk = 0;
    if(offset_in_block + len > BLOCK_SIZE){
        need2blk = 1;
        block_num2 = block_num+1;
    }
    if((block_num > BLOCK_SIZE-1 + NUM_PTRS) || (block_num2 > BLOCK_SIZE-1 + NUM_PTRS)) {
        //printf("file too large, out of boundary\n");    
        return -1;
    }/*
    int final_blk_num;
    if(block_num >= NUM_PTRS){ // in indirected blocks
        printf("in indirected blocks\n"); 
        int indirec_blk[BLOCK_SIZE / sizeof(int)] = {};
        int res = block_read(inodep->indirect_ptr, indirec_blk);
        if(res != 0) return -1;
        int indir_idx = block_num - NUM_PTRS;
        final_blk_num = indirec_blk[indir_idx];
    }
    else{
        final_blk_num = inodep->ptrs[block_num];
    }*/
    int final_blk_num = get_fin_data_blk(block_num, inodep);
    int final_blk_num2;
    if(need2blk) final_blk_num2 = get_fin_data_blk(block_num2, inodep);
    //printf("final_blk_num: %d\n", final_blk_num);
    //printf("offset in data blk: %d\n", offset_in_block);
    char src[BLOCK_SIZE];
    //char* src = (char*)malloc(BLOCK_SIZE);
    //char* test = (char*)malloc(2);
    //free(test);
    // printf("src: %x\n", src);
    int res = block_read(final_blk_num, src);
    if(res != 0) return -1;
    //printf("src: %s\n", src);
    if(need2blk == 0)
        memcpy(buf, src + offset_in_block, len);
    //printf("buf: %s\n", buf);
    else {
        memcpy(buf, src + offset_in_block, BLOCK_SIZE-offset_in_block);
        // printf("first part buf: %s\n", buf);
        //src[BLOCK_SIZE] = {};
        int res = block_read(final_blk_num2, src);
        if(res != 0) return -1;
        //printf("src: %s\n", src);
        memcpy(buf+ BLOCK_SIZE-offset_in_block, src, len - (BLOCK_SIZE-offset_in_block));
        //printf("src2: %s\n", src);
        //printf("buf: %s\n", buf);

    }
    //free(src);
    return 0;
}


// [lab6-ex3]
// return the size of the file "ino"
// (note: check "inode_t" in fs.h)
int fs_fsize(int ino) {
    // TODO: your code here
    // FATAL("fs_fsize is not implemented");
    inode_t* inodep = load_inode(ino);
    //printf("inodep: %x\n", inodep);
    //printf("inodep->size: %zu\n", inodep->size);

    return inodep->size;
}


// [lab6-ex3]
// write "len" bytes of data to the file ("ino") starting from "offset"
// notes:
//   -- use "alloc_block" to allocate a block if needed
//   -- remember to update file size
//   -- remember to flush the inode
//   -- remember to flush the modified blocks
//   -- return 0 if success; otherwise, return -1
int fs_write(int ino, int offset, int len, const char *buf) {
    // TODO: your code here
    //FATAL("fs_write is not implemented");
    
    inode_t* inodep = load_inode(ino);
    if(offset > inodep->size) {
        //printf("write offest: %d, file size: %d\n", offset, inodep->size);
        FATAL("fs_write offset > file size\n");
        //offset = inodep->size;
    }
    int append = 0; // whether size larger.
    int needalloc = 0;
    if(offset + len > inodep->size){
        m_uint32 newSize = offset + len;
        append = 1;
        if(newSize/BLOCK_SIZE > inodep->size/BLOCK_SIZE) needalloc = 1;
        inodep->size = offset + len; // need to flush inode later.

    }
    //printf("fz: %d\n", inodep->size);
    //blk_no in
    int block_num = offset / BLOCK_SIZE;
    int block_num2 = 0;
    int offset_in_block = offset % BLOCK_SIZE;
    int need2blk = 0;
    if(offset_in_block + len > BLOCK_SIZE){
        need2blk = 1;
        block_num2 = block_num+1; // assume len < BLOCK_SIZE
    }

    if((block_num > BLOCK_SIZE-1 + NUM_PTRS) || (block_num2 > BLOCK_SIZE-1 + NUM_PTRS)) {
        printf("offset too large, fs_write out of boundary\n");
        return -1;
    }
    
    int final_blk_num = 0;
    int final_blk_num2 = 0;
    
    if(append == 1) { // append == 1
        if((needalloc==1) && (need2blk==1)){ //part write old blk, part write to new blk.
            final_blk_num = get_fin_data_blk(block_num, inodep);
            final_blk_num2 = set_fin_data_blk(block_num2, inodep);
            //printf("final_blk_num2: %d\n", final_blk_num2);
        }
        else if((needalloc==1) && (need2blk==0)){ // write to a new blk
            final_blk_num = set_fin_data_blk(block_num, inodep);
        }
        else { // needalloc == 0
            // get physical blk_no
            final_blk_num = get_fin_data_blk(block_num, inodep);
            //final_blk_num2 = -1;
            //if(need2blk) final_blk_num2 = get_fin_data_blk(block_num2, inodep);
        }
           
    }
    else { // append == 0
        // get physical blk_no
        final_blk_num = get_fin_data_blk(block_num, inodep);
        //final_blk_num2 = -1;
        if(need2blk) final_blk_num2 = get_fin_data_blk(block_num2, inodep);
    }
    char src[BLOCK_SIZE] = {};
    //char* src = (char*) malloc(BLOCK_SIZE);
    //char fin[BLOCK_SIZE] = {}
    int res = block_read(final_blk_num, src);
    if(res != 0) return -1;
    //printf("buf: %s\n", buf);
    //printf("src: %s\n", src);
    //printf("offset_in_block: %d\n", offset_in_block);
    //printf("len: %d\n", len);
    //printf("fz: %d\n", inodep->size);
    if(need2blk == 0){
        memcpy(src+offset_in_block, buf, len);
        block_write(final_blk_num, src);
        //printf("src after: %s\n", src);
    }
    else {
        memcpy(src + offset_in_block, buf, BLOCK_SIZE-offset_in_block);
        block_write(final_blk_num, src);
        //printf("src: %s\n", src);
        
        char src2[BLOCK_SIZE] = {};
        //char* src2 = (char*)malloc(BLOCK_SIZE);   
        int res = block_read(final_blk_num2, src2);
        if(res != 0) return -1;
        //printf("src2: %s\n", src);
        memcpy(src2, buf+ BLOCK_SIZE-offset_in_block, len - (BLOCK_SIZE-offset_in_block));
        //printf("src2: %s\n", src);
        //printf("buf: %s\n", buf);
        block_write(final_blk_num2, src2);
        //free(src2);
    }
    //free(src);
    //printf("fz: %d\n", inodep->size);
    flush_inode();

    return 0;
    
}

