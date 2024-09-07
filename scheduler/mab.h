#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <unistd.h>


struct mab {
    int id; //migth be useful
    int offset; //starting address of the memory block
    int size; //size of the memory block
    int allocated; //the block allocated or not
    struct mab * parent; // for use in the Buddy binary tree
    struct mab * left_child; // for use in the binary tree
    struct mab * right_child; // for use in the binary tree
};
typedef struct mab Mab;
typedef Mab * MabPtr;


MabPtr createMab(int size,int allocated, int offset);
MabPtr memMerge(MabPtr m); // merge buddy memory blocks
MabPtr memSplit(MabPtr m, int size); // split a memory block
MabPtr dfsAlloc(MabPtr m, int size); // core memAlloc code
MabPtr memAlloc(MabPtr m, int size); // assistant code for memAlloc
MabPtr memFree(MabPtr m); // free memory block




