/*--------------------------------------------------------------------*/
/* ft.c                                                               */
/* Author: Isaac Gyamfi and Ndongo Njie                               */
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



/*
  Traverses the FT starting at the root as far as possible towards
  absolute path oPPath. If able to traverse, returns an int SUCCESS
  status and sets *poNFurthest to the furthest node reached (which may
  be only a prefix of oPPath, or even NULL if the root is NULL).
  Otherwise, sets *poNFurthest to NULL and returns with status:
  * CONFLICTING_PATH if the root's path is not a prefix of oPPath
  * MEMORY_ERROR if memory could not be allocated to complete request
*/
static int FT_traversePath(Path_T oPPath, Node_T *poNFurthest) {
    int iStatus;
    Path_T oPPrefix = NULL;
    Node_T oNCurr;
    Node_T oNChild = NULL;
    size_t ulDepth;
    size_t i;
    size_t ulChildID;

    assert(oPPath != NULL);
    assert(poNFurthest != NULL);

    /* root is NULL -> won't find anything */
    if(oNRoot == NULL) {
        *poNFurthest = NULL;
        return SUCCESS;
    }

    iStatus = Path_prefix(oPPath, 1, &oPPrefix);
    if(iStatus != SUCCESS) {
        *poNFurthest = NULL;
        return iStatus;
    }

    if(Path_comparePath(Node_getPath(oNRoot), oPPrefix)) {
        Path_free(oPPrefix);
        *poNFurthest = NULL;
        return CONFLICTING_PATH;
    }
    Path_free(oPPrefix);
    oPPrefix = NULL;

    oNCurr = oNRoot;
    ulDepth = Path_getDepth(oPPath);
    for(i = 2; i <= ulDepth; i++) {
        iStatus = Path_prefix(oPPath, i, &oPPrefix);
        if(iStatus != SUCCESS) {
            *poNFurthest = NULL;
            return iStatus;
        }
        if(Node_hasChild(oNCurr, oPPrefix, &ulChildID)) {
            /* go to that child and continue with next prefix */
            Path_free(oPPrefix);
            oPPrefix = NULL;
            iStatus = Node_getChild(oNCurr, ulChildID, FALSE, &oNChild);
            if(iStatus != SUCCESS) {
                *poNFurthest = NULL;
                return iStatus;
            }
            oNCurr = oNChild;
        }
        else {
            /* oNCurr doesn't have child with path oPPrefix:
            this is as far as we can go */
            break;
        }
    }

    Path_free(oPPrefix);
    *poNFurthest = oNCurr;
    return SUCCESS;
}

/*
  Traverses the FT to find a node with absolute path pcPath. Returns a
  int SUCCESS status and sets *poNResult to be the node, if found.
  Otherwise, sets *poNResult to NULL and returns with status:
  * INITIALIZATION_ERROR if the FT is not in an initialized state
  * BAD_PATH if pcPath does not represent a well-formatted path
  * CONFLICTING_PATH if the root's path is not a prefix of pcPath
  * NO_SUCH_PATH if no node with pcPath exists in the hierarchy
  * MEMORY_ERROR if memory could not be allocated to complete request
 */

static int FT_findNode(const char *pcPath, Node_T *poNResult) {
    Path_T oPPath = NULL;
    Node_T oNFound = NULL;
    int iStatus;

    assert(pcPath != NULL);
    assert(poNResult != NULL);

    if(!bIsInitialized) {
        *poNResult = NULL;
        return INITIALIZATION_ERROR;
    }

    iStatus = Path_new(pcPath, &oPPath);
    if(iStatus != SUCCESS) {
        *poNResult = NULL;
        return iStatus;
    }

    iStatus = FT_traversePath(oPPath, &oNFound);
    if(iStatus != SUCCESS)
    {
        Path_free(oPPath);
        *poNResult = NULL;
        return iStatus;
    }

    if(oNFound == NULL) {
        Path_free(oPPath);
        *poNResult = NULL;
        return NO_SUCH_PATH;
    }

    if(Path_comparePath(Node_getPath(oNFound), oPPath) != 0) {
        Path_free(oPPath);
        *poNResult = NULL;
        return NO_SUCH_PATH;
    }

    Path_free(oPPath);
    *poNResult = oNFound;
    return SUCCESS;
}
/*--------------------------------------------------------------------*/



int FT_insertDir(const char *pcPath){
    Node_T oNParent = NULL;
    Path_T oPPath = NULL;
    Node_T oNNewDir = NULL;
    int iStatus;

    assert(pcPath != NULL);

    if (!bIsInitialized || pcPath == NULL) {
        return INITIALIZATION_ERROR;
    }

    iStatus = FT_findNode(pcPath, &oNParent);
    if (iStatus == SUCCESS) {
        return ALREADY_IN_TREE;
    }

    iStatus = Path_new(pcPath, &oPPath);
    if (iStatus != SUCCESS) {
        return iStatus;
    }

   iStatus = Node_new(oPPath, oNParent, FALSE, NULL, 0, &oNNewDir);
    if (iStatus != SUCCESS) {
        Path_free(oPPath);
        return iStatus;
    }

    Path_free(oPPath);
    return SUCCESS;
}


boolean FT_containsDir(const char *pcPath){
    Node_T oNFound = NULL;
    int iStatus;

    assert(pcPath != NULL);

    if (!bIsInitialized || pcPath == NULL) {
        return FALSE;
    }

    iStatus = FT_findNode(pcPath, &oNFound);
    return (iStatus == SUCCESS && !Node_isFileNode(oNFound));

}

int FT_rmDir(const char *pcPath){
    Node_T oNTarget = NULL;
    int iStatus;

    assert(pcPath != NULL);

    if (!bIsInitialized || pcPath == NULL) {
        return INITIALIZATION_ERROR;
    }

    iStatus = FT_findNode(pcPath, &oNTarget);
    if (iStatus != SUCCESS || Node_isFileNode(oNTarget)) {
        return NO_SUCH_PATH;
    }
    
    return iStatus;

}

int FT_insertFile(const char *pcPath, void *pvContents, size_t ulLength){
    Node_T oNParent = NULL;
    Path_T oPPath = NULL;
    Node_T oNNewFile = NULL;
    int iStatus;

    assert(pcPath != NULL);

    if (!bIsInitialized || pcPath == NULL) {
        return INITIALIZATION_ERROR;
    }

    iStatus = FT_findNode(pcPath, &oNParent);
    if (iStatus == SUCCESS) {
        return ALREADY_IN_TREE;
    }

    iStatus = Path_new(pcPath, &oPPath);
    if (iStatus != SUCCESS) {
        return iStatus;
    }

    iStatus = Node_new(oPPath, oNParent, TRUE, pvContents, ulLength, &oNNewFile);
    if (iStatus != SUCCESS) {
        Path_free(oPPath);
        return iStatus;
    }

    Path_free(oPPath);
    return SUCCESS;

}

boolean FT_containsFile(const char *pcPath){
     /* If FT contains a file with path pcPath */
    int iStatus;
    Node_T oNFound = NULL;

    assert(pcPath != NULL);

    if (!bIsInitialized || pcPath == NULL) {
        return FALSE;
    }

    iStatus = FT_findNode(pcPath, &oNFound);
    return (iStatus == SUCCESS);

}

int FT_rmFile(const char *pcPath){
    Node_T oNTarget = NULL;
    int iStatus;

    assert(pcPath != NULL);

    if (!bIsInitialized || pcPath == NULL) {
        return INITIALIZATION_ERROR;
    }

    iStatus = FT_findNode(pcPath, &oNTarget);
    if (iStatus != SUCCESS || !Node_isFileNode(oNTarget)) {
        return NO_SUCH_PATH;
    }

    iStatus = Node_removeFile(oNTarget);
    return iStatus;



}

void *FT_getFileContents(const char *pcPath){
    
    Node_T oNFound = NULL;
    void *pvContent = NULL;

    assert(pcPath != NULL); 

    if (!bIsInitialized || pcPath == NULL) {
        return NULL;
    }

    if (FT_findNode(pcPath, &oNFound) != SUCCESS || !Node_isFileNode(oNFound)) {
        return NULL;
    }

    pvContent = FT_getFileContents(oNFound);
    return pvContent;

    /* Node_T oNFound = NULL;
    // assert(pcPath != NULL); 
    // void *pvContent;

    // if (!bIsInitialized || pcPath == NULL) {
    //     return NULL;
    // }

    // if (FT_findNode(pcPath, &oNFound) == NULL);
    // pvContent = oNFound->content;
    // return pvContent; */ 

}

void *FT_replaceFileContents(const char *pcPath, void *pvNewContents, size_t ulNewLength){
    Node_T oNTarget = NULL;
    void *oldContents = NULL;
    int iStatus;

    assert(pcPath != NULL);

    if (!bIsInitialized || pcPath == NULL) {
        return NULL;
    }

    iStatus = FT_findNode(pcPath, &oNTarget);
    if (iStatus != SUCCESS || !Node_isFileNode(oNTarget)) {
        return NULL;
    }

    oldContents = FT_replaceFileContents(oNTarget, pvNewContents, ulNewLength);
    return oldContents;

}

int FT_stat(const char *pcPath, boolean *pbIsFile, size_t *pulSize){
    Node_T oNFound = NULL;
    int iStatus;

    assert(pcPath != NULL);
    assert(pbIsFile != NULL);
    assert(pulSize != NULL);

    if (!bIsInitialized || pcPath == NULL) {
        return INITIALIZATION_ERROR;
    }

    iStatus = FT_findNode(pcPath, &oNFound);
    if (iStatus != SUCCESS) {
        return iStatus;
    }

    *pbIsFile = Node_isFileNode(oNFound);
    if (*pbIsFile) {
        *pulSize = Node_getFileSize(oNFound);
    }

    return SUCCESS;


}

int FT_init(void){

    if(bIsInitialized)
        return INITIALIZATION_ERROR;
    bIsInitialized = TRUE;
    oNRoot = NULL;
    ulCount = 0;

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
  string representation of the FT.
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
            i = FT_preOrderTraversal(oNChild, d, i);
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
    (void) FT_preOrderTraversal(oNRoot, nodes, 0);

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


