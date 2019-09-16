#ifndef _NTREE_H_
#define _NTREE_H_

// <CLASS>
// <NAME>NTreeNode</NAME>
// <SUMMARY>
// NTreeNode is a base class used to create a n-child tree.  Rather than having pointers to each individual child, it has a pointer to
// the first child and that child node has pointers to all of its sibling nodes.  Each node also has a pointer back to the parent it belongs to.
// </SUMMARY>
// <NOTE>Each node is only meant to be the child of ONE node.</NOTE>
// </CLASS>

#include <assert.h>

//===============================================================================
template <class VARTYPE> 
class NTreeNode
{
public:
  NTreeNode(void)                         { parent = child = sibling = NULL; }
  ~NTreeNode(void)                        { destroy_tree(); }

  //LINKING FUNCTIONS
  NTreeNode<VARTYPE>* add_child(const VARTYPE& new_child);

  NTreeNode<VARTYPE>* find_node(const VARTYPE& value);

  void destroy_tree();

  int count_nodes();

public:
  VARTYPE				data;	
  NTreeNode<VARTYPE>	*parent;   
  NTreeNode<VARTYPE>	*child;    
  NTreeNode<VARTYPE>	*sibling;
};


template <class VARTYPE>
inline NTreeNode<VARTYPE>* NTreeNode<VARTYPE>::add_child (const VARTYPE& new_child)
{
  NTreeNode<VARTYPE> *new_child_node;
  new_child_node = new(NTreeNode<VARTYPE>);

  //assign parent pointer to new child
  new_child_node->parent = this;	
  new_child_node->data   = new_child;

  if (child == NULL) {
	  //We're adding our first child
	  child = new_child_node;
    return child;
  } else {
	  //go to the end of the child list
	  NTreeNode<VARTYPE> * p;
	  for (p = child; p->sibling != NULL; p = p->sibling); 
	  //put the new child on the end
	  p->sibling = new_child_node;
    return p->sibling;
  }
}

template <class VARTYPE>
inline NTreeNode<VARTYPE>* NTreeNode<VARTYPE>::find_node (const VARTYPE& value)
{
  if (data == value) {
    return this;
  } else {
    if (sibling) {
      NTreeNode<VARTYPE>* sibling_value = sibling->find_node(value);
      if (sibling_value) { 
        return sibling_value;
      }
    }
    if (child) {
      NTreeNode<VARTYPE>* child_value = child->find_node(value);
      if (child_value) {
        return child_value;
      }
    }

    return NULL;
  }
}

//Does not delete memory pointed to by data, if any.
template <class VARTYPE>
inline void NTreeNode<VARTYPE>::destroy_tree () 
{
	if(sibling)
	{
		sibling->destroy_tree();
	}
	if(child)
	{
    child->destroy_tree();
	}
}


//===============================================================================
template <class VARTYPE>
inline int NTreeNode<VARTYPE>::count_nodes (void)
{
  int retval = 1;

  if (sibling) {
    retval += sibling->count_nodes();
  }
  if (child) {
    retval += child->count_nodes();
  }

  return retval;
}

#endif
