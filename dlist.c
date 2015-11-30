 /*
  *  fonulator 3 - foneBRIDGE configuration daemon
  *  (C) 2005-2007 Redfone Communications, LLC.
  *   www.red-fone.com
  *
  * Doubly Linked List Implementation
  * from "Mastering Algorithms with C"
  */
#include <stdlib.h>
#include <string.h>

#include "fonulator.h"

/*******************************************************************************
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use, copy,
 * modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES, OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF, OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE, OR ANY OTHER DEALINGS IN
 * THE SOFTWARE.
 ********************************************************************************/

/*****************************************************************************
*                                                                            *
*  ------------------------------ dlist_init ------------------------------  *
*                                                                            *
*****************************************************************************/

void
dlist_init (DList * list, void (*destroy) (void *data))
{

/*****************************************************************************
*                                                                            *
*  Initialize the list.                                                      *
*                                                                            *
*****************************************************************************/

  list->size = 0;
  list->destroy = destroy;
  list->head = NULL;
  list->tail = NULL;

  return;

}

/*****************************************************************************
*                                                                            *
*  ---------------------------- dlist_destroy -----------------------------  *
*                                                                            *
*****************************************************************************/

void
dlist_destroy (DList * list)
{

  void *data;

/*****************************************************************************
*                                                                            *
*  Remove each element.                                                      *
*                                                                            *
*****************************************************************************/

  while (dlist_size (list) > 0)
    {

      if (dlist_remove (list, dlist_tail (list), (void **) &data) == 0
	  && list->destroy != NULL)
	{

      /***********************************************************************
      *                                                                      *
      *  Call a user-defined function to free dynamically allocated data.    *
      *                                                                      *
      ***********************************************************************/

	  list->destroy (data);

	}

    }

/*****************************************************************************
*                                                                            *
*  No operations are allowed now, but clear the structure as a precaution.   *
*                                                                            *
*****************************************************************************/

  memset (list, 0, sizeof (DList));

  return;

}

/*****************************************************************************
*                                                                            *
*  ---------------------------- dlist_ins_next ----------------------------  *
*                                                                            *
*****************************************************************************/

int
dlist_ins_next (DList * list, DListElmt * element, const void *data)
{

  DListElmt *new_element;

/*****************************************************************************
*                                                                            *
*  Do not allow a NULL element unless the list is empty.                     *
*                                                                            *
*****************************************************************************/

  if (element == NULL && dlist_size (list) != 0)
    return -1;

/*****************************************************************************
*                                                                            *
*  Allocate storage for the element.                                         *
*                                                                            *
*****************************************************************************/

  if ((new_element = (DListElmt *) malloc (sizeof (DListElmt))) == NULL)
    return -1;

/*****************************************************************************
*                                                                            *
*  Insert the new element into the list.                                     *
*                                                                            *
*****************************************************************************/

  new_element->data = (void *) data;

  if (dlist_size (list) == 0)
    {

   /**************************************************************************
   *                                                                         *
   *  Handle insertion when the list is empty.                               *
   *                                                                         *
   **************************************************************************/

      list->head = new_element;
      list->head->prev = NULL;
      list->head->next = NULL;
      list->tail = new_element;

    }

  else
    {

   /**************************************************************************
   *                                                                         *
   *  Handle insertion when the list is not empty.                           *
   *                                                                         *
   **************************************************************************/

      new_element->next = element->next;
      new_element->prev = element;

      if (element->next == NULL)
	list->tail = new_element;
      else
	element->next->prev = new_element;

      element->next = new_element;

    }

/*****************************************************************************
*                                                                            *
*  Adjust the size of the list to account for the inserted element.          *
*                                                                            *
*****************************************************************************/

  list->size++;

  return 0;

}

/*****************************************************************************
*                                                                            *
*  ---------------------------- dlist_ins_prev ----------------------------  *
*                                                                            *
*****************************************************************************/


int
dlist_ins_prev (DList * list, DListElmt * element, const void *data)
{

  DListElmt *new_element;

/*****************************************************************************
*                                                                            *
*  Do not allow a NULL element unless the list is empty.                     *
*                                                                            *
*****************************************************************************/

  if (element == NULL && dlist_size (list) != 0)
    return -1;

/*****************************************************************************
*                                                                            *
*  Allocate storage to be managed by the abstract datatype.                  *
*                                                                            *
*****************************************************************************/

  if ((new_element = (DListElmt *) malloc (sizeof (DListElmt))) == NULL)
    return -1;

/*****************************************************************************
*                                                                            *
*  Insert the new element into the list.                                     *
*                                                                            *
*****************************************************************************/

  new_element->data = (void *) data;

  if (dlist_size (list) == 0)
    {

   /**************************************************************************
   *                                                                         *
   *  Handle insertion when the list is empty.                               *
   *                                                                         *
   **************************************************************************/

      list->head = new_element;
      list->head->prev = NULL;
      list->head->next = NULL;
      list->tail = new_element;

    }


  else
    {

   /**************************************************************************
   *                                                                         *
   *  Handle insertion when the list is not empty.                           *
   *                                                                         *
   **************************************************************************/

      new_element->next = element;
      new_element->prev = element->prev;

      if (element->prev == NULL)
	list->head = new_element;
      else
	element->prev->next = new_element;

      element->prev = new_element;

    }


/*****************************************************************************
*                                                                            *
*  Adjust the size of the list to account for the new element.               *
*                                                                            *
*****************************************************************************/

  list->size++;

  return 0;

}

/*****************************************************************************
*                                                                            *
*  ----------------------------- dlist_remove -----------------------------  *
*                                                                            *
*****************************************************************************/

int
dlist_remove (DList * list, DListElmt * element, void **data)
{

/*****************************************************************************
*                                                                            *
*  Do not allow a NULL element or removal from an empty list.                *
*                                                                            *
*****************************************************************************/

  if (element == NULL || dlist_size (list) == 0)
    return -1;

/*****************************************************************************
*                                                                            *
*  Remove the element from the list.                                         *
*                                                                            *
*****************************************************************************/

  *data = element->data;

  if (element == list->head)
    {

   /**************************************************************************
   *                                                                         *
   *  Handle removal from the head of the list.                              *
   *                                                                         *
   **************************************************************************/

      list->head = element->next;

      if (list->head == NULL)
	list->tail = NULL;
      else
	element->next->prev = NULL;

    }

  else
    {

   /**************************************************************************
   *                                                                         *
   *  Handle removal from other than the head of the list.                   *
   *                                                                         *
   **************************************************************************/

      element->prev->next = element->next;

      if (element->next == NULL)
	list->tail = element->prev;
      else
	element->next->prev = element->prev;

    }

/*****************************************************************************
*                                                                            *
*  Free the storage allocated by the abstract datatype.                      *
*                                                                            *
*****************************************************************************/

  free (element);

/*****************************************************************************
*                                                                            *
*  Adjust the size of the list to account for the removed element.           *
*                                                                            *
*****************************************************************************/

  list->size--;

  return 0;

}
