/*--------------------------------------------------------------------*/
/* checkerDT.c                                                        */
/* Author:                                                            */
/*--------------------------------------------------------------------*/

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "checkerDT.h"
#include "dynarray.h"
#include "path.h"



/* see checkerDT.h for specification */
boolean CheckerDT_Node_isValid(Node_T oNNode) {
   Node_T oNParent;
   Path_T oPNPath;
   Path_T oPPPath;

   /* Sample check: a NULL pointer is not a valid node */
   if(oNNode == NULL) {
      fprintf(stderr, "A node is a NULL pointer\n");
      return FALSE;
   }


   /* Sample check: parent's path must be the longest possible
      proper prefix of the node's path */
   oNParent = Node_getParent(oNNode);
   if(oNParent != NULL) {
      oPNPath = Node_getPath(oNNode);
      oPPPath = Node_getPath(oNParent);

      if(Path_getSharedPrefixDepth(oPNPath, oPPPath) !=
         Path_getDepth(oPNPath) - 1) {
         fprintf(stderr, "P-C nodes don't have P-C paths: (%s) (%s)\n",
                 Path_getPathname(oPPPath), Path_getPathname(oPNPath));
         return FALSE;
      }
   }

   return TRUE;
}

/*
   Performs a pre-order traversal of the tree rooted at oNNode. Passes the 
   number of nodes in the DT to pRealCount.
   Returns FALSE if a broken invariant is found and
   returns TRUE otherwise.

*/
static boolean CheckerDT_treeCheck(Node_T oNNode, size_t *pRealCount) {
   size_t ulIndex;
   Node_T oNPrevChild = NULL;
   if(oNNode!= NULL) {

      /* Sample check on each node: node must be valid */
      /* If not, pass that failure back up immediately */
      if(!CheckerDT_Node_isValid(oNNode))
         return FALSE;
      (*pRealCount)++;

      /* Recur on every child of oNNode */
      for(ulIndex = 0; ulIndex < Node_getNumChildren(oNNode); ulIndex++)
      {
         Node_T oNChild = NULL;
         int iStatus = Node_getChild(oNNode, ulIndex, &oNChild);

         if(iStatus != SUCCESS) {
            fprintf(stderr, "getNumChildren claims more children than getChild returns\n");
            return FALSE;
         }

         /*Look for duplicates or children out of order. */
         if (oNPrevChild != NULL) {
            int isEqual = Path_comparePath(Node_getPath(oNPrevChild), Node_getPath(oNChild));
            
            if (isEqual == 0) {
               fprintf(stderr, "Duplicate children\n");
               return FALSE;

            }
            if (isEqual > 0) {
               fprintf(stderr, "Children not in order\n");
               return FALSE;
            }


         }

         /* if recurring down one subtree results in a failed check
            farther down, passes the failure back up immediately */
         if(!CheckerDT_treeCheck(oNChild, pRealCount))
            return FALSE;
         oNPrevChild = oNChild;
      }
   }
   return TRUE;
}

/* see checkerDT.h for specification */
boolean CheckerDT_isValid(boolean bIsInitialized, Node_T oNRoot,
                          size_t ulCount) {

   size_t realCount = 0; /*used to track the number of nodes on DT*/
   boolean checkerTreeGood; /* stores boolean status of CheckerDT_treeCheck*/

   /* Sample check on a top-level data structure invariant:
      if the DT is not initialized, its count should be 0. */
   if(!bIsInitialized) {
      if(ulCount != 0) {
         fprintf(stderr, "Not initialized, but count is not 0\n");
         return FALSE;
      }

      if(oNRoot != NULL) {
         fprintf(stderr, "Not initialized but root is not NULL\n");
         return FALSE;
      }
      return TRUE;
   }
   
   /*If we make it to this point, we know the tree is initialized*/
   if(oNRoot == NULL) {
      if(ulCount != 0) {
         fprintf(stderr, "Tree is initialized and root is NULL but ulCount is not 0\n");
         return FALSE;
      }
   }
   else {
      if (ulCount == 0) {
         fprintf(stderr, "Tree is initialized and root is not NULL but ulCount is 0\n");
         return FALSE;

      }
      if (Node_getParent(oNRoot) != NULL) {
         fprintf(stderr, "Tree is initialized but root parent is not null\n");
         return FALSE;

      }
   }
   
   /* Now checks invariants recursively at each node from the root. */
   checkerTreeGood = CheckerDT_treeCheck(oNRoot, &realCount);

   if (checkerTreeGood == TRUE) {
      if(ulCount != realCount) {
         fprintf(stderr, "ulCount does not match the number of nodes in the tree\n");
         return FALSE;
      }
   }
   return checkerTreeGood;
}
