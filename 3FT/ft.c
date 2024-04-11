/*--------------------------------------------------------------------*/
/* ft.c                                                               */
/* Author: Isaac Gyamfi and Ndongo Njie                                       */
/*--------------------------------------------------------------------*/

#include <stddef.h>
#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "dynarray.h"
#include "path.h"
#include "nodeFT.h"
#include "ft.h"



/*
  A File Tree is a representation of a hierarchy of directories and Files,
  represented as an AO with 3 state variables:
*/

/* 1. a flag for being in an initialized state (TRUE) or not (FALSE) */
static boolean bIsInitialized;
/* 2. a pointer to the root node in the hierarchy */
static Node_T oNRoot;
/* 3. a counter of the number of nodes in the hierarchy */
static size_t ulCount;



int FT_insertDir(const char *pcPath){
    

}

boolean FT_containsFile(const char *pcPath){

}

int FT_rmFile(const char *pcPath){


}

void *FT_getFileContents(const char *pcPath){

}

void *FT_replaceFileContents(const char *pcPath, void *pvNewContents, size_t ulNewLength){

}

int FT_init(void){
    assert(CheckerFT_isValid(bIsInitialized, oNRoot, ulCount));

    if(bIsInitialized)
        return INITIALIZATION_ERROR;
    bIsInitialized = TRUE;
    oNRoot = NULL;
    ulCount = 0;

    assert(CheckerFT_isValid(bIsInitialized, oNRoot, ulCount));
    return SUCCESS;

}

int FT_destroy(void){

    if(!bIsInitialized)
        return INITIALIZATION_ERROR;

    if(oNRoot) {
        ulCount -= Node_free(oNRoot);
        oNRoot = NULL;
    }

    bIsInitialized = FALSE;

    return SUCCESS;

}


/* --------------------------------------------------------------------

  The following auxiliary functions are used for generating the
  string representation of the DT.
*/

/*
  Performs a pre-order traversal of the tree rooted at n,
  inserting each payload to DynArray_T d beginning at index i.
  Returns the next unused index in d after the insertion(s).
*/
static size_t FT_preOrderTraversal(Node_T n, DynArray_T d, size_t i) {
   size_t c;

   assert(d != NULL);

    if(n != NULL) {
        (void) DynArray_set(d, i, n);     
        i++;
        
        for(c = 0; c < Node_getNumFileChildren(n); c++) {
            int iStatus;
            Node_T oNChild = NULL;
            /* Getting File Nodes First*/
            iStatus = Node_getChild(n,c,TRUE,&oNChild);
            assert(iStatus == SUCCESS);
            (void) DynArray_set(d, i, oNChild);
            i++;
        }

        for (c = 0; c < Node_getNumDirChildren(n); c++){
            int iStatus;
            Node_T oNChild = NULL;
            /* Getting Directory Nodes Second*/
            iStatus = Node_getChild(n,c, FALSE,&oNChild);
            assert(iStatus == SUCCESS);
            i = DT_preOrderTraversal(oNChild, d, i);
        }
    }
    return i;
}

/*
  Alternate version of strlen that uses pulAcc as an in-out parameter
  to accumulate a string length, rather than returning the length of
  oNNode's path, and also always adds one addition byte to the sum.
*/
static void FT_strlenAccumulate(Node_T oNNode, size_t *pulAcc) {
   assert(pulAcc != NULL);

   if(oNNode != NULL)
      *pulAcc += (Path_getStrLength(Node_getPath(oNNode)) + 1);
}
/*
  Alternate version of strcat that inverts the typical argument
  order, appending oNNode's path onto pcAcc, and also always adds one
  newline at the end of the concatenated string.
*/
static void FT_strcatAccumulate(Node_T oNNode, char *pcAcc) {
   assert(pcAcc != NULL);

   if(oNNode != NULL) {
      strcat(pcAcc, Path_getPathname(Node_getPath(oNNode)));
      strcat(pcAcc, "\n");
   }
}
/*--------------------------------------------------------------------*/

char *FT_toString(void){
    DynArray_T nodes;
    size_t totalStrlen = 1;
    char *result = NULL;

    if(!bIsInitialized)
        return NULL;

    nodes = DynArray_new(ulCount);
    (void) DT_preOrderTraversal(oNRoot, nodes, 0);

    DynArray_map(nodes, (void (*)(void *, void*)) FT_strlenAccumulate,
                (void*) &totalStrlen);

    result = malloc(totalStrlen);
    if(result == NULL) {
        DynArray_free(nodes);
        return NULL;
    }
    *result = '\0';

    DynArray_map(nodes, (void (*)(void *, void*)) FT_strcatAccumulate,
                (void *) result);

    DynArray_free(nodes);

    return result;
}

