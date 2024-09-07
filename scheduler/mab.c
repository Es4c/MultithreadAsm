#include "mab.h"

int MAX_SIZE = 2048;
MabPtr last_block = NULL; //used for allocating

/**
 * @brief round value to next power of 2 if it's not power of 2.
 * modified from https://stackoverflow.com/questions/466204/rounding-up-to-next-power-of-2
 * 
 * @param x 
 * @return int 
 */
int roundUp(int x)
{   
    if(x>0){
        x--;
        x |= x >> 1;
        x |= x >> 2;
        x |= x >> 4;
        x |= x >> 8;
        x |= x >> 16;
        x++;
        return x<8 ? 8:x;
        
    }
    return 8;
}


/**
 * @brief Create a Mab object
 * 
 * @param size 
 * @param allocated 
 * @param offset 
 * @return MabPtr 
 */
MabPtr createMab(int size,int allocated, int offset){
    MabPtr new_mab;
    if(!(new_mab = (MabPtr)malloc(sizeof(Mab)))){
        fprintf(stderr,"ERROR: Could not create a new Mab block of size %d\n",size);
        return NULL;
    }
    new_mab->offset = offset;
    new_mab->size = size;
    new_mab->allocated = allocated;
    new_mab->parent = NULL;
    new_mab->left_child = NULL;
    new_mab->right_child = NULL;
    return new_mab;
}


int mergable(MabPtr m){
    return (!m->allocated && !(m->left_child || m->right_child));
}

/**
 * @brief similar to memAlloc, this functions uses dfs to traverse and free blocks
 * 
 * @param m normally a root node
 * @return MabPtr m
 */
MabPtr memMerge(MabPtr m){
     if(m->left_child || m->right_child){
        memMerge(m->left_child);
        memMerge(m->right_child);
     }
     if(m->left_child && m->right_child){
        if(mergable(m->left_child) && mergable(m->right_child)){
            free(m->left_child);
            free(m->right_child);
            m->left_child = NULL;
            m->right_child = NULL;
        }
    }
    return m;
} 
// merge buddy memory blocks

/**
 * @brief 
 * It will split the input by half as its left & right child
 * and its left child will be passed into this function again
 * until a satified block is found.
 * the block is created with allocated = 0 by default,
 * remember to change it to 1 when you need
 * 
 * @param m the block you want to split, normally you should get this by memAlloc()
 * @param size 
 * @return MabPtr the block prepared for allocation
 */
MabPtr memSplit(MabPtr m, int size){
    if(!m){
        printf("m == null\n ");
        return NULL;
    }

    int actual_size = roundUp(size);
    if(m->size/2<size && m->size<size){
        printf("insufficient space for %d with %d\n", size, m->size);
        return NULL;
    }else{
        int diff = m->size - actual_size;

        if(diff<0){
            printf("impossible\n");
            return NULL;
        }
        if(diff == 0 || diff<actual_size){
            m->allocated = 1;
            return m;
        }
        m->left_child = createMab(m->size/2,0,m->offset);
        m->right_child = createMab(m->size/2,0,m->offset+m->size/2);
        m->left_child->parent = m;
        m->right_child->parent = m;
        if(diff == actual_size){
            m->left_child->allocated = 1;
            return m->left_child;
        }else if(diff > actual_size){
            return memSplit(m->left_child,actual_size);
        }
    }
    
    return NULL;
    
   
} // split a memory block

/**
 * @brief 
 * dfs recursion search with pruning based on 
 * various if-statements.
 * It uses a global variable last_block to assist
 * its functionality
 * 
 * @param m the block you want to allocate from
 * @param size 
 * @return MabPtr the block for allocation, you should memSplit() it before use
 */
MabPtr dfsAlloc(MabPtr m, int size){
    if(last_block) return NULL;
    if(!m || m->size<size || m->allocated) return NULL;
    if(m->size == size && (m->left_child || m->right_child)) return NULL;
    if(m->size>=size && (!m->left_child && !m->right_child)){
        last_block = m;
        return NULL;
    }
    dfsAlloc(m->left_child,size);
    dfsAlloc(m->right_child,size);
    
    return last_block;
}

/**
 * @brief the assistant code or an adapter for 'real' memAlloc
 * it basically clears the value of last_block so that you don't
 * have to do it manually after each time you calling memAlloc
 * @param m the block you want to allocate from
 * @param size 
 * @return MabPtr the block for allocation, you should memSplit() it before use
 */
MabPtr memAlloc(MabPtr m, int size){
    dfsAlloc(m,size);
    MabPtr p = &(*last_block);
    last_block = NULL;
    return p;
} // allocate memory block


/**
 * @brief just set allocate to 0, you may want to do memMerge() afterward
 * 
 * @param m 
 * @return MabPtr 
 */
MabPtr memFree(MabPtr m){
   m->allocated = 0;
   return m;
}
