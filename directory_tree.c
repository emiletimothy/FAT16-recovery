#include "directory_tree.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

const mode_t MODE = 0777;

void init_node(node_t *node, char *name, node_type_t type) {
    if (name == NULL) {
        name = strdup("ROOT");
        assert(name != NULL);
    }
    node->name = name;
    node->type = type;
}

file_node_t *init_file_node(char *name, size_t size, uint8_t *contents) {
    file_node_t *node = malloc(sizeof(file_node_t));
    assert(node != NULL);
    init_node((node_t *) node, name, FILE_TYPE);
    node->size = size;
    node->contents = contents;
    return node;
}

directory_node_t *init_directory_node(char *name) {
    directory_node_t *node = malloc(sizeof(directory_node_t));
    assert(node != NULL);
    init_node((node_t *) node, name, DIRECTORY_TYPE);
    node->num_children = 0;
    node->children = NULL;
    return node;
}

void add_child_directory_tree(directory_node_t *dnode, node_t *child) {
    char *child_name;
    size_t i;

    // Increases the number of children by 1
    dnode->num_children++;
    // Assigns 'sizeof(node_t)' extra space to dnode->children.
    dnode->children = realloc(dnode->children, dnode->num_children * sizeof(node_t *));
    // Adds the child to the last memory location of 'dnode->children' which is
    // 'num_children - 1'
    dnode->children[dnode->num_children - 1] = child;
    // Insertion sort to sort children alphabetically

    child_name = child->name;

    // Determines the position at which *child is to be inserted.
    for (i = 0; i < (dnode->num_children - 1); i++) {
        if (strcmp(dnode->children[i]->name, child_name) > 0) {
            break;
        }
    }

    // Inserts the node in the position determined previously.
    for (size_t j = dnode->num_children - 1; j > i; j--) {
        dnode->children[j] = dnode->children[j - 1];
    }
    dnode->children[i] = child;
}

void print_directory_subtree(node_t *node, size_t recursion_level) {
    /*
    param1: node_t *node
    param2: recursion_level, monitors the recursion depth
    */
    size_t loop_index;

    /*
    Regardless of the file_type, the name needs to be printed.
    No type casting is required yet.
    */

    for (loop_index = 0; loop_index < 4 * recursion_level; loop_index++) {
        printf(" ");
    }
    printf("%s\n", node->name);

    if (node->type == DIRECTORY_TYPE) {
        /*
        The node type is DIRECTORY_TYPE, so the node is cast to
        directory_node_t and its list of children is accessed recursively
        with recursion_level + 1.
        */

        // type casting the node to directory_node_t
        directory_node_t *dnode = (directory_node_t *) node;

        for (size_t i = 0; i < dnode->num_children; i++) {
            print_directory_subtree(dnode->children[i], recursion_level + 1);
        }
    }
}

void print_directory_tree(node_t *node) {
    // Uses a helper function 'print_directory_subtree'
    // calls the helper function
    print_directory_subtree(node, 0);
}

void create_directory_subtree(node_t *node, char *path) {
    if (node->type == FILE_TYPE) {
        // opens a file and checks if it opens successfully
        FILE *in;
        in = fopen(path, "w");
        assert(in != NULL);
        // type casting the node to file node
        file_node_t *fnode = (file_node_t *) node;
        // writes the contents of the files to in
        assert(fwrite(fnode->contents, sizeof(uint8_t), fnode->size, in) == fnode->size);
        // closes the file
        assert(fclose(in) == 0);
    }
    else {
        // initializes the name of the child
        char *child_path;

        assert(node->type == DIRECTORY_TYPE);
        assert(mkdir(path, MODE) == 0);
        // type casts the node to directory node
        directory_node_t *dnode = (directory_node_t *) node;

        // creates a recursion for the children of dnode
        for (size_t i = 0; i < dnode->num_children; i++) {
            child_path = malloc(sizeof(char) *
                                (strlen(path) + 2 + strlen(dnode->children[i]->name)));
            strcpy(child_path, path);
            strcat(child_path, "/");
            strcat(child_path, dnode->children[i]->name);
            create_directory_subtree(dnode->children[i], child_path);
            free(child_path);
        }
    }
}

void create_directory_tree(node_t *node) {
    // Uses a helper function 'print_directory_subtree'
    // calls the helper function
    create_directory_subtree(node, node->name);
}

void free_directory_tree(node_t *node) {
    if (node->type == FILE_TYPE) {
        file_node_t *fnode = (file_node_t *) node;
        free(fnode->contents);
    }
    else {
        assert(node->type == DIRECTORY_TYPE);
        directory_node_t *dnode = (directory_node_t *) node;
        for (size_t i = 0; i < dnode->num_children; i++) {
            free_directory_tree(dnode->children[i]);
        }
        free(dnode->children);
    }
    free(node->name);
    free(node);
}