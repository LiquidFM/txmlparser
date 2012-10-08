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

#ifndef	_TXML_PARSER_H
#define	_TXML_PARSER_H

/* *******************************************************************
 * Public TXML types.
 * *******************************************************************/
typedef void* txml_node;
typedef void* txml_attr;


/* *******************************************************************
 * Public TXML Functions.
 * *******************************************************************/
txml_node txml_parse(char* xml_content, unsigned int xml_size);

void txml_free(txml_node node);

char* txml_node_get_name(txml_node node);

char* txml_node_get_text(txml_node node);

char* txml_node_get_prop(txml_node node, const char* name);

txml_attr txml_node_get_first_attr(txml_node node);

txml_attr txml_node_get_next_attr(txml_attr attr);

void txml_node_get_attr_name_value(txml_attr attr, char** name, char** value);

txml_node txml_node_get_first_child(txml_node node);

txml_node txml_node_get_next_sibling(txml_node node);

#endif /* _TXML_PARSER_H */

