/*--------------------------------------------------------------------*/
/* nodeFT.h                                                           */
/* Author: Isaac Gyamfi and Ndongo Njie                                        */
/*--------------------------------------------------------------------*/

#ifndef NODE_INCLUDED
#define NODE_INCLUDED

#include <stddef.h>
#include "a4def.h"
#include "path.h"


/* A Node_T is a node in a File Tree */
typedef struct node *Node_T;

/*
  Creates a new node in the File Tree, with path oPPath and
  parent oNParent. Sets the file status of the node to the boolean bIsFile,
  assigns the contents of the node the pvContent and the size of the content
  to ulength and Returns an int SUCCESS status and sets *poNResult
  to be the new node if successful. Otherwise, sets *poNResult to NULL
  and returns status:
  * MEMORY_ERROR if memory could not be allocated to complete request
  * CONFLICTING_PATH if oNParent's path is not an ancestor of oPPath
  * NO_SUCH_PATH if oPPath is of depth 0
                 or oNParent's path is not oPPath's direct parent
                 or oNParent is NULL but oPPath is not of depth 1
  * ALREADY_IN_TREE if oNParent already has a child with this path
*/
int Node_new(Path_T oPPath, Node_T oNParent, boolean bIsFile, void *pvContent, 
size_t ulength, Node_T *poNResult);

/*
  Destroys and frees all memory allocated for the subtree rooted at
  oNNode, i.e., deletes this node and all its descendents. Returns the
  number of nodes deleted.
*/
size_t Node_free(Node_T oNNode);

/*Replaces the old content. Takes oNNode, newContent, and length as arguments
and return a void pointer. */
void *Node_replaceOldContent(Node_T oNNode, void *newContent, size_t length);

/* Takes oNNode as an argument and returns the path object representing 
oNNode's absolute path. */
Path_T Node_getPath(Node_T oNNode);

/* Checks if it is file. Takes oNNode as an argument and return TRUE 
when the node is a file but FALSE otherwiae. */
boolean Node_isFileNode(Node_T oNNode);

/*Takes oNNode as an argument and return its content */
void *Node_getFileContent(Node_T oNNode);

/* Returns the number of file children that oNParent has. */
size_t Node_getNumFileChildren(Node_T oNParent);

/* Returns the number of directory children that oNParent has. */
size_t Node_getNumDirChildren(Node_T oNParent);

/*It takes a node oNNode and returns the size of the file. If it is not 
a file, it returns 0 if it is a directory */
size_t Node_getFileSize(Node_T oNNode);

/*
  Returns TRUE if oNParent has a directory Child with path oPPath.
  Returns FALSE if it does not.

  If oNParent has such a child, stores in *pulChildID the child's
  identifier (as used in Node_getChild). If oNParent does not have
  such a child, stores in *pulChildID the identifier that such a
  child _would_ have if inserted.
*/
boolean Node_hasDirChild(Node_T oNParent, Path_T oPPath,
                         size_t *pulChildID);


/*
  Returns TRUE if oNParent has a file child with path oPPath. Returns
  FALSE if it does not.

  If oNParent has such a child, stores in *pulChildID the child's
  identifier (as used in Node_getChild). If oNParent does not have
  such a child, stores in *pulChildID the identifier that such a
  child _would_ have if inserted.
*/
boolean Node_hasFileChild(Node_T oNParent, Path_T oPPath,
                         size_t *pulChildID);

/*
  Takes oNParent, ulChildID, bIsFile *poNResult as arguments. 
  bIsFile checks if it is a file or directory. Returns an int SUCCESS 
  status and sets *poNResult to be the child node of oNParent with 
  identifier ulChildID, if one exists.Otherwise, sets *poNResult to 
  NULL and returns status:
  * NO_SUCH_PATH if ulChildID is not a valid child for oNParent
*/
int Node_getChild(Node_T oNParent, size_t ulChildID, boolean bIsFile,
                  Node_T *poNResult);

/*
  Returns a the parent node of oNNode.
  Returns NULL if oNNode is the root and thus has no parent.
*/
Node_T Node_getParent(Node_T oNNode);

/*
  Compares oNFirst and oNSecond lexicographically based on their paths.
  Returns <0, 0, or >0 if onFirst is "less than", "equal to", or
  "greater than" oNSecond, respectively.
*/
int Node_compare(Node_T oNFirst, Node_T oNSecond);

/*
  Returns a string representation for oNNode, or NULL if
  there is an allocation error.

  Allocates memory for the returned string, which is then owned by
  the caller!
*/
char *Node_toString(Node_T oNNode);

#endif
