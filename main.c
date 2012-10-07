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

/*********** EXAMPLE CODE ***********************/

#include "txml_parser.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

static void print_nodes(txml_node node)
{
	txml_attr attr = NULL;
	char *name, *value;
	
	printf("TAG=%s TEXT=%s\n", 
		txml_node_get_name(node), txml_node_get_text(node));

	attr = txml_node_get_first_attr(node);
	while (attr != NULL) {
		txml_node_get_attr_name_value(attr, &name, &value);
		printf("\tATTR NAME=%s VALUE=%s\n", name, value);
		attr = txml_node_get_next_attr(attr);
	} 

	if (txml_node_get_first_child(node))
		print_nodes(txml_node_get_first_child(node));
	
	if (txml_node_get_next_sibling(node))
		print_nodes(txml_node_get_next_sibling(node));

	return;
}

int main(int argc, char* argv[])
{
	int fd, ret;	
	char* xml_data;
	txml_node tree;
	struct stat statbuf;

	if (argc != 2)
	{
		printf("usage: %s <xmlfile>\n", argv[0]);
		return -1;
	}

	fd = open(argv[1], O_RDONLY, 0);
	if (fd < 0)
	{
		printf("failed to open xml\n");
		return -1;
	}

	fstat(fd, &statbuf);
	xml_data = malloc(statbuf.st_size);
	if (xml_data == NULL)
	{
		printf("failed to allocate");
		return -1;
	}

	ret = read(fd, xml_data, statbuf.st_size);
	if (ret < 0)
	{
		printf("failed to read xml\n");
		return -1;	
	}

	tree = txml_parse(xml_data, (unsigned int)statbuf.st_size);
	if (tree == NULL)
	{
		printf("xml parse failed!");
		return -1;
	}

	print_nodes(tree);

	txml_free(tree);

	free(xml_data);

	return 0;

}
