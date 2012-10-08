/*
 * Copyright (C) 2012 by Satyanarayana Reddy Thadvai<satyalive@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining 
 * a copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation 
 * the rights to use, copy, modify, merge, publish, distribute, sublicense, 
 * and/or sell copies of the Software, and to permit persons to whom the 
 * Software is furnished to do so, subject to the following conditions:

 * The above copyright notice and this permission notice shall be included 
 * in all copies or substantial portions of the Software.

 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR 
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL 
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR 
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, 
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR 
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#ifdef __KERNEL__
#include <linux/slab.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/string.h>
#include <linux/ctype.h>
#define TXML_ALLOC(x)		kzalloc(x, GFP_KERNEL)
#define TXML_FREE(x)			kfree(x)
#define TXML_PRINT			printk
#else
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#define TXML_ALLOC(x)		calloc(1, x)
#define TXML_FREE(x)			free(x)
#define TXML_PRINT			printf
#endif
#include "txml_parser.h"

/*
* Structures
*/
struct _txml_attr {
	char* name;
	char* value;
	struct _txml_attr* next;
};

struct _txml_text {
	unsigned char isCDATA; /* is it a CDATA text */
	char* str;	  /* string to hold text data */
};

/* Structure representing XML Node Tree */
struct _txml_node {
	char* tag;		/* always non-NULL */
	struct _txml_text text;		/* array of strings, NULL marks end */
	struct _txml_attr*  attrs;	/* array of strings, NULL marks end */
	struct  _txml_node* parent;
	struct  _txml_node* first_child;
	struct  _txml_node* next_sibling;
};

/* TXML Status Codes */
#define TXML_SUCCESS 			0
#define TXML_FAILURE 			-1
#define TXML_INVALIDARG			-2
#define TXML_OUTOFMEMORY		-3
#define TXML_PARSEERROR			-4

#define TXML_CHK_MEM(expr) {               \
            if ( 0 == (expr) )    \
            {                        \
                TXML_PRINT("%s[%d]: out of memory\n" , __func__, __LINE__); \
                status = TXML_OUTOFMEMORY; \
                goto ErrorExit;     \
            }                       \
        }

#define TXML_CHK_ARG(expr) {               \
            if ( !(expr) )       \
            {                       \
                TXML_PRINT("%s[%d]: invalid args\n" , __func__, __LINE__); \
                status = TXML_INVALIDARG; \
                goto ErrorExit;     \
            }                       \
        }

#define TXML_CHK_STATUS(expr) {               \
            status = (expr);            \
            if ((status != TXML_SUCCESS))   \
            {                       \
                TXML_PRINT("%s[%d]: functional error ret-%d\n" , __func__, __LINE__, status); \
                goto ErrorExit;     \
            }                       \
        }

#define TXML_SET_STRING(str, start, end) {               \
		int iLength = ((end - start) + 1)*sizeof(char); \
		str = TXML_ALLOC(iLength); \
		TXML_CHK_MEM(str); \
		memcpy((str), &pxml_content[start], iLength-1); \
	}


/* XML State Diagram */
#define TXML_START	 			0x00000000L
#define TXML_OPENTAG_START			0x00000001L
#define TXML_OPENTAG_NAME		 	0x00000002L
#define TXML_AFTER_OPENTAG_NAME			0x00000003L
#define TXML_ATTR_NAME		 		0x00000004L
#define TXML_AFTER_ATTR_NAME		 	0x00000005L
#define TXML_AFTER_EQUAL		 	0x00000006L
#define TXML_ATTR_VAL_STRING	 		0x00000007L
#define TXML_ATTR_VAL_QUOTES			0x00000008L
#define TXML_AFTER_ATTR_VAL			0x00000009L
#define TXML_CLOSETAG_START		 	0x0000000AL
#define TXML_CLOSETAG_NAME		 	0x0000000BL
#define TXML_AFTER_CLOSETAG_NAME		0x0000000CL
#define TXML_OPENTAG_END		 	0x0000000DL
#define TXML_SKIPTAG_START	 		0x0000000EL
#define TXML_AFTER_EXLM				0x0000000FL
#define TXML_AFTER_OPEN_HYPEN1			0x00000010L
#define TXML_AFTER_OPEN_HYPEN2			0x00000011L
#define TXML_AFTER_CLOSE_HYPEN1			0x00000012L
#define TXML_AFTER_CLOSE_HYPEN2			0x00000013L
#define TXML_TAG_BODY				0x00000014L
#define TXML_CDATA_START			0x00000015L
#define TXML_END		 		0xFFFFFFFFL

/* XML Tokens */
#define _IsWhiteSpace		(pxml_content[curr_index] > 0 && pxml_content[curr_index] < '!')
#define _IsSlash			(pxml_content[curr_index] == '/')
#define _IsLessThan			(pxml_content[curr_index] == '<')
#define _IsGreaterThan		(pxml_content[curr_index] == '>')
#define _IsEqualTo			(pxml_content[curr_index] == '=')
#define _IsQuote			(pxml_content[curr_index] == '\'' || pxml_content[curr_index] == '\"')
#define _IsQuestion			(pxml_content[curr_index] == '?')
#define _IsExclamation		(pxml_content[curr_index] == '!')
#define _IsHyphen			(pxml_content[curr_index] == '-')
#define _IsSquareBracket	(pxml_content[curr_index] == '[')

#define _IsAlphaNumeric		((pxml_content[curr_index] >= '0' && pxml_content[curr_index] <= '9')	\
							||(pxml_content[curr_index] >= 'a' && pxml_content[curr_index] <= 'z')	\
							||(pxml_content[curr_index] >= 'A' && pxml_content[curr_index] <= 'Z'))

#define _IsSymbol			(_IsAlphaNumeric		\
							|| pxml_content[curr_index] == '_'	\
							|| pxml_content[curr_index] == ':'	\
							|| pxml_content[curr_index] == '-'	\
							|| pxml_content[curr_index] == '.' )


#define NEXT_CHAR            \
{                            \
    if (++curr_index >= xml_size) \
    {                        \
        break;            \
    }                        \
}    

#define NEXT_OFFSET(offset)			\
{									\
	curr_index = curr_index + (offset);	\
	if (curr_index >= xml_size) {        \
		break;					\
	}                               \
}

#define STRIP_SPACES           \
{                             \
    while (curr_index < xml_size   \
       &&  (pxml_content[curr_index] > 0 && pxml_content[curr_index] < '!'))\
    {                         \
        curr_index++;            \
    }                         \
    if (curr_index >= xml_size)    \
    {                         \
        break;                \
    }                         \
}

static void _txml_delete_node(struct _txml_node* current_node)
{
	struct _txml_attr* attr;

	if (current_node->first_child)
	{
		_txml_delete_node(current_node->first_child);
	}

	if(current_node->next_sibling)
	{
		_txml_delete_node(current_node->next_sibling);
	}

	/* free the root node contents*/
	if (current_node->tag) {
		TXML_FREE(current_node->tag);
	}
	if (current_node->text.str) {
		TXML_FREE(current_node->text.str);
	}
	if (current_node->attrs) {
		for (attr = current_node->attrs; attr != 0; attr = attr->next) {
			if (attr->name) {
				TXML_FREE(attr->name);
			}
			if (attr->value) {
				TXML_FREE(attr->value);
			}
		}
	}

	TXML_FREE(current_node);
}

txml_node txml_parse(char* pxml_content, unsigned int xml_size) 
{
	int status = TXML_SUCCESS;
	unsigned long txml_state = TXML_START;
	struct _txml_node* pcurrent_node = 0;
	struct _txml_attr* pcurrent_attr = 0;
	unsigned int curr_index = 0;
	unsigned int tag_offset = -1, text_offset = -1, attr_offset = -1;
	struct _txml_node* pxml_root = NULL;

	for (;;) {
		
		switch (txml_state) {
			
			case TXML_START: /* eat whitespace before < */
			{
				STRIP_SPACES;

				if (_IsLessThan) { /*<*/
					txml_state = TXML_OPENTAG_START;
				} else { 
					TXML_CHK_STATUS(TXML_PARSEERROR); 
				}
				break;
			}
			
			case TXML_OPENTAG_START: /* handle the first char after < */
			{
				if (_IsSlash) { /*</*/
					txml_state = TXML_CLOSETAG_START;
				} else if (_IsQuestion) { /*<?*/
					txml_state = TXML_SKIPTAG_START; 
				} else if (_IsExclamation) { /*<!*/
					txml_state = TXML_AFTER_EXLM; 
				} else if (_IsSymbol) { /*<t*/
					if (pxml_root == NULL) {
						pxml_root = (struct _txml_node*)TXML_ALLOC(sizeof(struct _txml_node));
						TXML_CHK_MEM(pxml_root);
						pxml_root->parent = 0;
						pcurrent_node = pxml_root;
					} else {
						if (!pcurrent_node->first_child) {
							pcurrent_node->first_child = (struct _txml_node*)TXML_ALLOC(sizeof(struct _txml_node));
							TXML_CHK_MEM(pcurrent_node->first_child);
							pcurrent_node->first_child->parent = pcurrent_node;
							pcurrent_node = pcurrent_node->first_child;
						} else {
							struct _txml_node* tempNode = pcurrent_node->first_child;

							while( tempNode->next_sibling != 0) tempNode = tempNode->next_sibling;

							tempNode->next_sibling = (struct _txml_node*)TXML_ALLOC(sizeof(struct _txml_node));
							TXML_CHK_MEM(tempNode->next_sibling);
							tempNode->next_sibling->parent = pcurrent_node;
							pcurrent_node = tempNode->next_sibling;

						}
					}

					tag_offset = curr_index;
					txml_state = TXML_OPENTAG_NAME;
				} else { 
					TXML_CHK_STATUS(TXML_PARSEERROR); 
				}
				break;
			}
			
			case TXML_OPENTAG_NAME: /* store tag name and a single space after it */
			{
				if (_IsWhiteSpace) { /*<t */
					TXML_SET_STRING(pcurrent_node->tag, tag_offset, curr_index);
					txml_state = TXML_AFTER_OPENTAG_NAME;
				} else if (_IsSlash) { /*<t/*/
					TXML_SET_STRING(pcurrent_node->tag, tag_offset, curr_index);
					txml_state=TXML_OPENTAG_END; 
				} else if (_IsGreaterThan) { /*<t>*/
					TXML_SET_STRING(pcurrent_node->tag, tag_offset, curr_index);
					txml_state=TXML_TAG_BODY;
				}
				else if (!_IsSymbol) { /*<t*/ /*-1*/
					TXML_CHK_STATUS(TXML_PARSEERROR); 
				}
				break;
			}

			case TXML_AFTER_OPENTAG_NAME: /* eat whitespace after tag name */
			{
				STRIP_SPACES;

				if (_IsSlash) { /*<tag /*/
					txml_state = TXML_OPENTAG_END;
				} else if (_IsGreaterThan) { /*<tag >*/
					txml_state = TXML_TAG_BODY;
				} else if (_IsSymbol) { /*<tag t*/
					if (!pcurrent_node->attrs) {
						pcurrent_node->attrs = TXML_ALLOC(sizeof(struct _txml_attr));
						TXML_CHK_MEM(pcurrent_node->attrs);
						pcurrent_attr = pcurrent_node->attrs;
					} else {
						pcurrent_attr->next = TXML_ALLOC(sizeof(struct _txml_attr));
						TXML_CHK_MEM(pcurrent_attr->next);
						pcurrent_attr = pcurrent_attr->next;
					}
					attr_offset = curr_index;
					txml_state = TXML_ATTR_NAME;
				} else { 
					TXML_CHK_STATUS(TXML_PARSEERROR);
				}
				break;
			}
			
			case TXML_ATTR_NAME: /* store attribute name */
			{
				if (_IsWhiteSpace) { /*<tag a */
					TXML_SET_STRING(pcurrent_attr->name, attr_offset, curr_index);
					txml_state = TXML_AFTER_ATTR_NAME; 
				} else if (_IsEqualTo) { /*<tag a=*/
					TXML_SET_STRING(pcurrent_attr->name, attr_offset, curr_index);
					txml_state = TXML_AFTER_EQUAL;
				} else if (!_IsAlphaNumeric) { /*<tag a*/ /*-t1*/
					TXML_CHK_STATUS(TXML_PARSEERROR); 
				}
				break;
			}
			
			case TXML_AFTER_ATTR_NAME: /* eat whitespace after attribute name */
			{
				STRIP_SPACES;

				if (_IsEqualTo) { /*<tag attr =*/
					txml_state = TXML_AFTER_EQUAL; 
				} else {
					TXML_CHK_STATUS(TXML_PARSEERROR);
				}
				break;
			}
			
			case TXML_AFTER_EQUAL: /* eat whitespace after = */
			{
				STRIP_SPACES;

				if (_IsQuote) { /*<tag attr=" or '*/
					attr_offset = curr_index+1;
					txml_state = TXML_ATTR_VAL_QUOTES;
				} else {
					TXML_CHK_STATUS(TXML_PARSEERROR);
				}
				break;
			}
			
			case TXML_ATTR_VAL_QUOTES: /* store the attribute value ('value') */
			{
				if (_IsQuote) { /*<tag attr='' or attr=""*/
					TXML_SET_STRING(pcurrent_attr->value, attr_offset, curr_index);
					txml_state=TXML_AFTER_ATTR_VAL;
				} else if (_IsLessThan) { 
					TXML_CHK_STATUS(TXML_PARSEERROR);
				}
				break;
			}
			
			case TXML_AFTER_ATTR_VAL: /* eat whitespace after attribute value */
			{
				if (_IsWhiteSpace) { /*<tag attr="value" */
					txml_state = TXML_AFTER_OPENTAG_NAME;
				} else if (_IsSlash) { /*..."/*/
					txml_state=TXML_OPENTAG_END;
				} else if (_IsGreaterThan) { /*...">*/
					txml_state=TXML_TAG_BODY;
				} else { 
					TXML_CHK_STATUS(TXML_PARSEERROR); 
				}
				break;
			}
			
			case TXML_CLOSETAG_START: /* handle the first char after </ */
			{
				if (_IsSymbol) {  /*</t*/
					tag_offset = curr_index;
					txml_state = TXML_CLOSETAG_NAME;
				} else { 
					TXML_CHK_STATUS(TXML_PARSEERROR); 
				}
				break;
			}
			
			case TXML_CLOSETAG_NAME: /* store tag name and a single space after it */
			{
				if (_IsWhiteSpace) { /*</t */
					if (memcmp(pcurrent_node->tag, &pxml_content[tag_offset], (curr_index - tag_offset)*sizeof(char)) != 0) {
						TXML_CHK_STATUS(TXML_PARSEERROR);
					}					
					txml_state = TXML_AFTER_CLOSETAG_NAME;
				} else if (_IsGreaterThan) { /*</t>*/
					if (memcmp(pcurrent_node->tag, &pxml_content[tag_offset], (curr_index - tag_offset)*sizeof(char)) != 0) {
						TXML_CHK_STATUS(TXML_PARSEERROR);
					}
					pcurrent_node = pcurrent_node->parent;
					txml_state=TXML_START; 
				} else if (!_IsSymbol) {  /*</t*/ /*-t*/
					TXML_CHK_STATUS(TXML_PARSEERROR);
				}
				break;
			}
			
			case TXML_AFTER_CLOSETAG_NAME: /* eat whitespace before > */
			{
				STRIP_SPACES;

				if (_IsGreaterThan) { /*</tag >*/
					pcurrent_node = pcurrent_node->parent;
					txml_state=TXML_START;
				} else { 
					TXML_CHK_STATUS(TXML_PARSEERROR);
				}
				break;
			}

			case TXML_OPENTAG_END: /* ensure final > */
			{
				if (_IsGreaterThan) {  /*<tag />*/
					pcurrent_node = pcurrent_node->parent;
					txml_state=TXML_START;
				} else {
					TXML_CHK_STATUS(TXML_PARSEERROR);
				}
				break;
			}

			case TXML_TAG_BODY: /* check for body of the tag */
			{
				STRIP_SPACES;

				if (_IsLessThan) { /* <tag> < */
					if (text_offset != -1) {
						TXML_SET_STRING(pcurrent_node->text.str, text_offset, curr_index);
						text_offset = -1;
					}					
					txml_state = TXML_OPENTAG_START;
				} else {
					if (text_offset == -1) {
						text_offset = curr_index;
					}
				}
				break;
			}

			case TXML_CDATA_START: /* check for CDATA */
			{
				char cdatastart[6] = { 'C', 'D', 'A', 'T', 'A', '['};
				char cdataend[3]= { ']', ']', '>'};

				if (memcmp(&pxml_content[curr_index], cdatastart, sizeof(cdatastart)) == 0) { /*<![CDATA[*/
					curr_index = curr_index + 5;
					text_offset = -1;
				} else if (memcmp(&pxml_content[curr_index], cdataend, sizeof(cdataend)) == 0) { /*]]>*/
					if (text_offset != -1) {
						pcurrent_node->text.isCDATA = 1;
						TXML_SET_STRING(pcurrent_node->text.str, text_offset, curr_index);
						text_offset = -1;
					}
					curr_index = curr_index + 2;
					txml_state = TXML_START;
				} else {
					if (text_offset == -1) {
						text_offset = curr_index;
					}
				}
				break;
			}

			case TXML_SKIPTAG_START: /* skip tag (<? tag) */
			{
				if (_IsGreaterThan) { /*<?>*/
					txml_state=TXML_START;
				}
				break;
			}
			
			case TXML_AFTER_EXLM: /* find first - (<! tag) */
			{
				if (_IsGreaterThan) { /*<!>*/
					txml_state=TXML_START;
				} else if (_IsHyphen) { /*<!-*/
					txml_state=TXML_AFTER_OPEN_HYPEN1; 
				} else if (_IsSquareBracket) { /*![*/
					txml_state = TXML_CDATA_START;
				} else { 
					txml_state=TXML_SKIPTAG_START;
				}
				break;
			}
			
			case TXML_AFTER_OPEN_HYPEN1: /* find second - (<!- tag) */
			{
				if (_IsGreaterThan) { /*<!->*/
					txml_state=TXML_START; 
				} else if (_IsHyphen) { /*<!--*/
					txml_state = TXML_AFTER_OPEN_HYPEN2; 
				} else { 
					txml_state=TXML_SKIPTAG_START; 
				}
				break;
			}
			
			case TXML_AFTER_OPEN_HYPEN2: /* skip comment */ 
			{
				if (_IsHyphen) { /*-*/
					txml_state = TXML_AFTER_CLOSE_HYPEN1; 
				}
				break;
			}
			
			case TXML_AFTER_CLOSE_HYPEN1: /* find second - in --> */
			{
				if (_IsHyphen) { /*--*/
					txml_state = TXML_AFTER_CLOSE_HYPEN2;
				} else { 
					txml_state=TXML_AFTER_OPEN_HYPEN2;
				}
				break;
			}

			case TXML_AFTER_CLOSE_HYPEN2: /* find > in --> */
			{
				if (_IsGreaterThan) { /*-->*/
					txml_state=TXML_START;
				} else { 
					TXML_CHK_STATUS(TXML_PARSEERROR);
				} 
				break;
			}
		}	

		NEXT_CHAR;
	}

ErrorExit:
	if (txml_state != TXML_START || status != TXML_SUCCESS) {
		if (pxml_root != NULL)
		{
			_txml_delete_node(pxml_root);
			pxml_root = NULL;
		}
	}
	return (txml_node)pxml_root;
}

void txml_free(txml_node node) 
{
	struct _txml_node* pxml_node = (struct _txml_node *)node;

	if (pxml_node != NULL)
	{
		_txml_delete_node(pxml_node);
	}
}

char* txml_node_get_name(txml_node node)
{
	struct _txml_node* pxml_node = (struct _txml_node *)node;

	return pxml_node->tag;
}

char* txml_node_get_text(txml_node node)
{
	struct _txml_node* pxml_node = (struct _txml_node *)node;

	return pxml_node->text.str;
}

char* txml_node_get_prop(txml_node node, const char* name)
{
	struct _txml_node* pxml_node = (struct _txml_node *)node;
	struct _txml_attr* curr_attr;

	if (pxml_node) {
		for (curr_attr = pxml_node->attrs; curr_attr != NULL; curr_attr=curr_attr->next)
		{
			if (!strcasecmp(curr_attr->name, name))
			{
				return curr_attr->value;
			}
		}
	}

	return NULL;
}

txml_attr txml_node_get_first_attr(txml_node node)
{
	struct _txml_node* pxml_node = (struct _txml_node *)node;

	if (pxml_node) {
		if (pxml_node->attrs) {
			return (txml_attr)pxml_node->attrs;
		} 
	} 

	return (txml_attr)NULL;
}

txml_attr txml_node_get_next_attr(txml_attr attr)
{
	struct _txml_attr* curr_attr = (struct _txml_attr* )attr;

	return (txml_node)(curr_attr) ? curr_attr->next: NULL;
}

void txml_node_get_attr_name_value(txml_attr attr, char** name, char** value)
{
	struct _txml_attr* curr_attr = (struct _txml_attr* )attr;

	if  (curr_attr)
	{
		if (name)
			*name = curr_attr->name;
		if (value)
			*value = curr_attr->value;
	}
}

txml_node txml_node_get_first_child(txml_node node)
{
	struct _txml_node* pxml_node = (struct _txml_node *)node;

	return (txml_node)pxml_node->first_child;
}

txml_node txml_node_get_next_sibling(txml_node node)
{
	struct _txml_node* pxml_node = (struct _txml_node *)node;

	return (txml_node)pxml_node->next_sibling;
}

