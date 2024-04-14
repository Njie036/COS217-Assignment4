/*--------------------------------------------------------------------*/
/* nodeFT.c                                                           */
/* Author: Isaac Gyamfi and Ndongo Njie                                       */
/*--------------------------------------------------------------------*/

#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include "dynarray.h"
#include "nodeFT.h"



/* A node in an FT */
struct node {
   /* the object corresponding to the node's absolute path */
   Path_T oPPath;
   /* this node's parent */
   Node_T oNParent;
   /* the object containing links to this node's directory children */
   DynArray_T oDirChildren;

   /* the object containing links to this node's file children */
   DynArray_T oFileChildren;

   /* The pointer to the content of a file node*/
   void *content;

   /* The size of the content of a file node*/
   size_t ulength;

   /* The type of Node*/
   boolean isFileNode;

};

/*
  Compares the string representation of oNfirst with a string
  pcSecond representing a node's path.
  Returns <0, 0, or >0 if oNFirst is "less than", "equal to", or
  "greater than" pcSecond, respectively.
*/
static int Node_compareString(const Node_T oNFirst,
                                 const char *pcSecond) {
   assert(oNFirst != NULL);
   assert(pcSecond != NULL);

   return Path_compareString(oNFirst->oPPath, pcSecond);
}



/*
  Links new child oNChild into oNParent's children array at index
  ulIndex. Returns SUCCESS if the new child was added successfully,
  or  MEMORY_ERROR if allocation fails adding oNChild to the array.
*/
static int Node_addChild(Node_T oNParent, Node_T oNChild) {
    size_t ulIndex;
    assert(oNParent != NULL);
    assert(oNChild != NULL);

    if (oNChild->isFileNode == TRUE){
        /* Wrong index if node is a file*/
        DynArray_bsearch(oNParent->oFileChildren,
            (char*) Path_getPathname(oNChild->oPPath), &ulIndex,
            (int (*)(const void*,const void*)) Node_compareString);
        if (DynArray_addAt(oNParent->oFileChildren, ulIndex, oNChild)){
            return SUCCESS;
        }
        else
            return MEMORY_ERROR;
    }  

    DynArray_bsearch(oNParent->oDirChildren,
            (char*) Path_getPathname(oNChild->oPPath), &ulIndex,
            (int (*)(const void*,const void*)) Node_compareString);
    if(DynArray_addAt(oNParent->oDirChildren, ulIndex, oNChild))
        return SUCCESS;
    else
        return MEMORY_ERROR;
}



/*
  Creates a new node with path oPPath and parent oNParent.  Returns an
  int SUCCESS status and sets *poNResult to be the new node if
  successful. Otherwise, sets *poNResult to NULL and returns status:
  * MEMORY_ERROR if memory could not be allocated to complete request
  * CONFLICTING_PATH if oNParent's path is not an ancestor of oPPath
  * NO_SUCH_PATH if oPPath is of depth 0
                 or oNParent's path is not oPPath's direct parent
                 or oNParent is NULL but oPPath is not of depth 1
  * ALREADY_IN_TREE if oNParent already has a child with this path
*/
int Node_new(Path_T oPPath, Node_T oNParent, boolean bIsFile, void *pvContent, size_t ulength, Node_T *poNResult)
{
    struct node *psNew;
    Path_T oPParentPath = NULL;
    Path_T oPNewPath = NULL;
    size_t ulParentDepth;
    size_t ulIndex;
    int iStatus;

    assert(oPPath != NULL);

    /* allocate space for a new node */
    psNew = malloc(sizeof(struct node));
    if(psNew == NULL) {
        *poNResult = NULL;
        return MEMORY_ERROR;
    }

    /* set the new node's path */
    iStatus = Path_dup(oPPath, &oPNewPath);
    if(iStatus != SUCCESS) {
        free(psNew);
        *poNResult = NULL;
        return iStatus;
    }
    psNew->oPPath = oPNewPath;

    /* validate and set the new node's parent */
   if(oNParent != NULL) {
      size_t ulSharedDepth;

      oPParentPath = oNParent->oPPath;
      ulParentDepth = Path_getDepth(oPParentPath);
      ulSharedDepth = Path_getSharedPrefixDepth(psNew->oPPath,
                                                oPParentPath);
      /* parent must be an ancestor of child */
      if(ulSharedDepth < ulParentDepth) {
         Path_free(psNew->oPPath);
         free(psNew);
         *poNResult = NULL;
         return CONFLICTING_PATH;
      }

      /* parent must be exactly one level up from child */
      if(Path_getDepth(psNew->oPPath) != ulParentDepth + 1) {
         Path_free(psNew->oPPath);
         free(psNew);
         *poNResult = NULL;
         return NO_SUCH_PATH;
      }
        /* Parent must be a directory*/
        if (oNParent->isFileNode){
            Path_free(psNew->oPPath);
            free(psNew);
            *poNResult = NULL;
            return NOT_A_DIRECTORY;
        }

      /* parent must not already have child with this path */
      if(Node_hasDirChild(oNParent, oPPath, &ulIndex)) {
         Path_free(psNew->oPPath);
         free(psNew);
         *poNResult = NULL;
         return ALREADY_IN_TREE;
      }
   }
   else {
      /* new node must be root */
      /* can only create one "level" at a time */
      if(Path_getDepth(psNew->oPPath) != 1) {
         Path_free(psNew->oPPath);
         free(psNew);
         *poNResult = NULL;
         return NO_SUCH_PATH;
      }
      if(bIsFile){
        Path_free(psNew->oPPath);
        free(psNew);
        *poNResult = NULL;
        return CONFLICTING_PATH;

      }
   }
    /* initialize the new node */
    psNew->oNParent = oNParent;
    psNew->isFileNode = bIsFile;
    if (!psNew->isFileNode)
    {
            psNew->content = NULL;
            psNew->ulength = 0;

            psNew->oFileChildren = DynArray_new(0);
            if(psNew->oFileChildren == NULL) {
                Path_free(psNew->oPPath);
                free(psNew);
                *poNResult = NULL;
                return MEMORY_ERROR;
            }
            psNew->oDirChildren = DynArray_new(0);
            if(psNew->oDirChildren == NULL) {
                Path_free(psNew->oPPath);
                DynArray_free(psNew->oFileChildren);
                free(psNew);
                *poNResult = NULL;
                return MEMORY_ERROR;
            }
    }
    else
    {
        psNew->content = pvContent;
        psNew->ulength = ulength;
        psNew->oFileChildren = NULL;
        psNew->oDirChildren = NULL;
    }

    /* Add the child to its parent if it is not NULL. */
    if (oNParent != NULL){
        iStatus = Node_addChild(oNParent, psNew);
        if(iStatus != SUCCESS) {
            Path_free(psNew->oPPath);
            free(psNew);
            *poNResult = NULL;
            return iStatus;
        }
    }
    *poNResult = psNew;
    return SUCCESS;  
    
}

void *Node_replaceOldContent(Node_T oNNode, void *newContent, size_t length) {
    void *oldContents = NULL;
    assert(oNNode != NULL);

    oldContents = oNNode->content;
    oNNode->content = newContent;
    oNNode->ulength = length;

    return oldContents;
}


size_t Node_getFileSize(Node_T oNNode){
    assert(oNNode != NULL);

    return oNNode->ulength;
}

size_t Node_free(Node_T oNNode){
    size_t ulIndex;
    size_t ulCount = 0;

    assert(oNNode != NULL);
    

    if(oNNode->oNParent != NULL) {
        if (oNNode->isFileNode) {  
            if(DynArray_bsearch(
                    oNNode->oNParent->oFileChildren,
                    oNNode, &ulIndex,
                    (int (*)(const void *, const void *)) Node_compare)
                )
                (void) DynArray_removeAt(oNNode->oNParent->oFileChildren,
                                        ulIndex);
        }
        else{
            if(DynArray_bsearch(
                        oNNode->oNParent->oDirChildren,
                        oNNode, &ulIndex,
                        (int (*)(const void *, const void *)) Node_compare)
                    )
                    (void) DynArray_removeAt(oNNode->oNParent->oDirChildren,
                                            ulIndex);
        }
    }

    if(!oNNode->isFileNode) {
        /* recursively remove children in File DynArray*/
        while(DynArray_getLength(oNNode->oFileChildren) != 0) {
            ulCount += Node_free(DynArray_get(oNNode->oFileChildren, 0));
        }
        DynArray_free(oNNode->oFileChildren);
        
        /* recursively remove children in Directory DynArray*/
        while(DynArray_getLength(oNNode->oDirChildren) != 0) {
            ulCount += Node_free(DynArray_get(oNNode->oDirChildren, 0));
        }
        DynArray_free(oNNode->oDirChildren);
    }

    /* remove path */
    Path_free(oNNode->oPPath);

    /* finally, free the struct node */
    free(oNNode);
    ulCount++;
    return ulCount;
}


Path_T Node_getPath(Node_T oNNode) {
   assert(oNNode != NULL);

   return oNNode->oPPath;
}

boolean Node_isFileNode(Node_T oNNode){
    assert(oNNode != NULL);
    
    return oNNode->isFileNode;
}

void *Node_getFileContent(Node_T oNNode){
    assert(oNNode != NULL);
    
    return oNNode->content;
}

boolean Node_hasDirChild(Node_T oNParent, Path_T oPPath,
                         size_t *pulChildID) {
    boolean found;
    assert(oNParent != NULL);
    assert(oPPath != NULL);
    assert(pulChildID != NULL);

    if (oNParent->isFileNode)
        return FALSE;


    /* Is it a directory child */
    found = DynArray_bsearch(oNParent->oDirChildren, 
    (char*) Path_getPathname(oPPath), pulChildID,
    (int (*)(const void*,const void*)) Node_compareString);
    
    if(found == TRUE) return TRUE;

    return FALSE;
}


boolean Node_hasFileChild(Node_T oNParent, Path_T oPPath,
                         size_t *pulChildID) {
    boolean found;
    assert(oNParent != NULL);
    assert(oPPath != NULL);
    assert(pulChildID != NULL);

    if (oNParent->isFileNode)
        return FALSE;

    /* Is it a file child */
    found = DynArray_bsearch(oNParent->oFileChildren, 
    (char*) Path_getPathname(oPPath), pulChildID,
    (int (*)(const void*,const void*)) Node_compareString);
    
    if(found == TRUE) return TRUE;

    return FALSE;
}
/*
size_t Node_getNumChildren(Node_T oNParent) {
    size_t numofChildren;
    assert(oNParent != NULL);

    numofChildren = 0;
    if (oNParent->isFile)
        return numofChildren;

    numofChildren = DynArray_getLength(oNParent->oFileChildren) + DynArray_getLength(oNParent->oDirChildren);

    return numofChildren;
}
*/

/* helper static function that takes a node as an argument  returns  */
size_t Node_getNumFileChildren(Node_T oNParent){
    assert(oNParent != NULL);

    return DynArray_getLength(oNParent->oFileChildren);

}

size_t Node_getNumDirChildren(Node_T oNParent){
    assert(oNParent != NULL);

    return DynArray_getLength(oNParent->oDirChildren);

}


int  Node_getChild(Node_T oNParent, size_t ulChildID, boolean bIsFile,
                   Node_T *poNResult) {

   assert(oNParent != NULL);
   assert(poNResult != NULL);

   /* ulChildID is the index into oNParent->oDChildren */
   if (bIsFile){
        if(ulChildID >= DynArray_getLength(oNParent->oFileChildren)) {
            *poNResult = NULL;
            return NO_SUCH_PATH;
        }
        else {
            *poNResult = DynArray_get(oNParent->oFileChildren, ulChildID);
            return SUCCESS;
        }
    }
    if(ulChildID >=DynArray_getLength(oNParent->oDirChildren)) {
        *poNResult = NULL;
        return NO_SUCH_PATH;
    }
    else {
        *poNResult = DynArray_get(oNParent->oDirChildren, ulChildID);
        return SUCCESS;
    }

   
}

Node_T Node_getParent(Node_T oNNode) {
   assert(oNNode != NULL);

   return oNNode->oNParent;
}

int Node_compare(Node_T oNFirst, Node_T oNSecond) {
   assert(oNFirst != NULL);
   assert(oNSecond != NULL);

   return Path_comparePath(oNFirst->oPPath, oNSecond->oPPath);
}


char *Node_toString(Node_T oNNode) {
   char *copyPath;

   assert(oNNode != NULL);

   copyPath = malloc(Path_getStrLength(Node_getPath(oNNode))+1);
   if(copyPath == NULL)
      return NULL;
   else
      return strcpy(copyPath, Path_getPathname(Node_getPath(oNNode)));
}

