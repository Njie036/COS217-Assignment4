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
        return SUCCESS; /*just changed this*/
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
        if(Node_hasDirChild(oNCurr, oPPrefix, &ulChildID)) {
            /* go to that child and continue with next prefix */
            Path_free(oPPrefix);
            oPPrefix = NULL;
            iStatus = Node_getChild(oNCurr, ulChildID, FALSE, &oNChild);
            if (iStatus == SUCCESS) {
                 oNCurr = oNChild;
            }
            /*Not found as a directory, check if it is a file*/
            else{
                *poNFurthest = NULL;
                return iStatus;
            }  
        }

        else if(Node_hasFileChild(oNCurr, oPPrefix, &ulChildID)) {
            /* go to that child and continue with next prefix */
            Path_free(oPPrefix);
            oPPrefix = NULL;
            iStatus = Node_getChild(oNCurr, ulChildID, TRUE, &oNChild);
            if (iStatus == SUCCESS) {
                 oNCurr = oNChild;
            }
            /*If it was not success */
            else{
                *poNFurthest = NULL;
                return iStatus;
            }  
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
        Path_free(oPPath);
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
    int iStatus;
   Path_T oPPath = NULL;
   Node_T oNFirstNew = NULL;
   Node_T oNCurr = NULL;
   size_t ulDepth, ulIndex;
   size_t ulNewNodes = 0;

   assert(pcPath != NULL);

   /* validate pcPath and generate a Path_T for it */
   if(!bIsInitialized)
      return INITIALIZATION_ERROR;

   iStatus = Path_new(pcPath, &oPPath);
   if(iStatus != SUCCESS)
      return iStatus;

   /* find the closest ancestor of oPPath already in the tree */
   iStatus= FT_traversePath(oPPath, &oNCurr);
   if(iStatus != SUCCESS)
   {
      Path_free(oPPath);
      return iStatus;
   }

   /* no ancestor node found, so if root is not NULL,
      pcPath isn't underneath root. */
   if(oNCurr == NULL && oNRoot != NULL) {
      Path_free(oPPath);
      return CONFLICTING_PATH;
   }

   /* Make sure ancestor is not a file */
   if (oNCurr != NULL && Node_isFileNode(oNCurr)) {
        Path_free(oPPath);
        return NOT_A_DIRECTORY;
   }

   ulDepth = Path_getDepth(oPPath);
   if(oNCurr == NULL) /* new root! */
      ulIndex = 1;
   else {
      ulIndex = Path_getDepth(Node_getPath(oNCurr))+1;

      /* oNCurr is the node we're trying to insert */
      if(ulIndex == ulDepth+1 && !Path_comparePath(oPPath,
                                       Node_getPath(oNCurr))) {
         Path_free(oPPath);
         return ALREADY_IN_TREE;
      }
   }

   /* starting at oNCurr, build rest of the path one level at a time */
   while(ulIndex <= ulDepth) {
      Path_T oPPrefix = NULL;
      Node_T oNNewNode = NULL;

      /* generate a Path_T for this level */
      iStatus = Path_prefix(oPPath, ulIndex, &oPPrefix);
      if(iStatus != SUCCESS) {
         Path_free(oPPath);
         if(oNFirstNew != NULL)
            (void) Node_free(oNFirstNew);
         return iStatus;
      }

      /* insert the new node for this level */
      iStatus = Node_new(oPPrefix, oNCurr, FALSE, NULL, 0, &oNNewNode);
      if(iStatus != SUCCESS) {
         Path_free(oPPath);
         Path_free(oPPrefix);
         if(oNFirstNew != NULL)
            (void) Node_free(oNFirstNew);
         return iStatus;
      }

      /* set up for next level */
      Path_free(oPPrefix);
      oNCurr = oNNewNode;
      ulNewNodes++;
      if(oNFirstNew == NULL)
         oNFirstNew = oNCurr;
      ulIndex++;
   }
   Path_free(oPPath);
   /* update DT state variables to reflect insertion */
   if(oNRoot == NULL)
      oNRoot = oNFirstNew;
   ulCount += ulNewNodes;
   return SUCCESS;
}

/*--------------------------------------------------------------------*/

boolean FT_containsDir(const char *pcPath){
    Node_T oNFound = NULL;
    int iStatus;

    assert(pcPath != NULL);

    if (!bIsInitialized || pcPath == NULL) {
        return FALSE;
    }

    iStatus = FT_findNode(pcPath, &oNFound);
    if (iStatus != SUCCESS){
        return FALSE;
    }
    if (Node_isFileNode(oNFound) == FALSE) {
        return TRUE;
    }
    else return FALSE;

}

/*--------------------------------------------------------------------*/

int FT_rmDir(const char *pcPath){
    int iStatus;
    Node_T oNFound = NULL;

   assert(pcPath != NULL);

   iStatus = FT_findNode(pcPath, &oNFound);

   if(!bIsInitialized){
    return INITIALIZATION_ERROR;
   }
    
   if(iStatus != SUCCESS) {
        return iStatus;
    }
   
    if (Node_isFileNode(oNFound)) {
        return NOT_A_DIRECTORY;
    }

   ulCount -= Node_free(oNFound);
   if(ulCount == 0)
      oNRoot = NULL;

   return SUCCESS;
}

/*--------------------------------------------------------------------*/

int FT_insertFile(const char *pcPath, void *pvContents, size_t ulLength){
    int iStatus;
   Path_T oPPath = NULL;
   Node_T oNFirstNew = NULL;
   Node_T oNCurr = NULL;
   size_t ulDepth, ulIndex;
   size_t ulNewNodes = 0;

   assert(pcPath != NULL);

   /* validate pcPath and generate a Path_T for it */
   if(!bIsInitialized)
      return INITIALIZATION_ERROR;

   iStatus = Path_new(pcPath, &oPPath);
   if(iStatus != SUCCESS)
      return iStatus;

   /* find the closest ancestor of oPPath already in the tree */
   iStatus= FT_traversePath(oPPath, &oNCurr);
   if(iStatus != SUCCESS)
   {
      Path_free(oPPath);
      return iStatus;
   }

   /* no ancestor node found, so if root is not NULL,
      pcPath isn't underneath root. */
   if(oNCurr == NULL && oNRoot != NULL) {
      Path_free(oPPath);
      return CONFLICTING_PATH;
   }

   /* Make sure ancestor is not a file */
   if (oNCurr != NULL && Node_isFileNode(oNCurr)) {
        Path_free(oPPath);
        return NOT_A_DIRECTORY;
   }

   ulDepth = Path_getDepth(oPPath);
   if(oNCurr == NULL) /* new root! */
      ulIndex = 1;
   else {
      ulIndex = Path_getDepth(Node_getPath(oNCurr))+1;

      /* oNCurr is the node we're trying to insert */
      if(ulIndex == ulDepth+1 && !Path_comparePath(oPPath,
                                       Node_getPath(oNCurr))) {
         Path_free(oPPath);
         return ALREADY_IN_TREE;
      }
   }

   /* starting at oNCurr, build rest of the path one level at a time */
   while(ulIndex <= ulDepth) {
      Path_T oPPrefix = NULL;
      Node_T oNNewNode = NULL;

      /* generate a Path_T for this level */
      iStatus = Path_prefix(oPPath, ulIndex, &oPPrefix);
      if(iStatus != SUCCESS) {
         Path_free(oPPath);
         if(oNFirstNew != NULL)
            (void) Node_free(oNFirstNew);
         return iStatus;
      }

      /* insert the new node for this level */
      if(ulIndex < ulDepth) {
        iStatus = Node_new(oPPrefix, oNCurr, FALSE, NULL, 0, &oNNewNode);
      }
      else{
        iStatus = Node_new(oPPrefix, oNCurr, TRUE, pvContents, ulLength, &oNNewNode);
      }
      if(iStatus != SUCCESS) {
         Path_free(oPPath);
         Path_free(oPPrefix);
         if(oNFirstNew != NULL)
            (void) Node_free(oNFirstNew);
         return iStatus;
      }

      /* set up for next level */
      Path_free(oPPrefix);
      oNCurr = oNNewNode;
      ulNewNodes++;
      if(oNFirstNew == NULL)
         oNFirstNew = oNCurr;
      ulIndex++;
   }
   Path_free(oPPath);
   /* update DT state variables to reflect insertion */
   if(oNRoot == NULL)
      oNRoot = oNFirstNew;
   ulCount += ulNewNodes;
   return SUCCESS;
}

/*--------------------------------------------------------------------*/

boolean FT_containsFile(const char *pcPath){
     /* If FT contains a file with path pcPath */
    int iStatus;
    Node_T oNFound = NULL;

    assert(pcPath != NULL);

    if (!bIsInitialized) {
        return FALSE;
    }

    iStatus = FT_findNode(pcPath, &oNFound);
    if (iStatus != SUCCESS){
        return FALSE;
    }
    if (Node_isFileNode(oNFound) == TRUE) {
        return TRUE;
    }
    else{
        return FALSE;
    }
}

/*--------------------------------------------------------------------*/

int FT_rmFile(const char *pcPath){
    int iStatus;
    Node_T oNFound = NULL;

   assert(pcPath != NULL);

   iStatus = FT_findNode(pcPath, &oNFound);

   if(iStatus != SUCCESS) {
        return iStatus;
        }
    else if (!Node_isFileNode(oNFound)) {
        return NOT_A_FILE;
    }

   ulCount -= Node_free(oNFound);
   if(ulCount == 0)
      oNRoot = NULL;

   return SUCCESS;
}

/*--------------------------------------------------------------------*/

void *FT_getFileContents(const char *pcPath){
    
    Node_T oNFound = NULL;
    void *pvContent = NULL;

    assert(pcPath != NULL); 

    if (!bIsInitialized || pcPath == NULL) {
        return NULL;
    }

    if (FT_findNode(pcPath, &oNFound) != SUCCESS || 
    !Node_isFileNode(oNFound)) {
        return NULL;
    }
    pvContent = Node_getFileContent(oNFound);
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

/*--------------------------------------------------------------------*/

void *FT_replaceFileContents(const char *pcPath, void *pvNewContents, 
        size_t ulNewLength){
    Node_T oNTarget = NULL;
    int iStatus;

    assert(pcPath != NULL);

    if (!bIsInitialized || pcPath == NULL) {
        return NULL;
    }

    iStatus = FT_findNode(pcPath, &oNTarget);
    if (iStatus != SUCCESS || !Node_isFileNode(oNTarget)) {
        return NULL;
    }

    return Node_replaceOldContent(oNTarget, pvNewContents, ulNewLength);
}

/*--------------------------------------------------------------------*/

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

/*--------------------------------------------------------------------*/

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

/*--------------------------------------------------------------------*/
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


