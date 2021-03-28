#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include "directory_tree.h"
#include "fat16.h"

const size_t MASTER_BOOT_RECORD_SIZE = 0x20B;

void follow_file(FILE *disk, directory_entry_t d_entry, directory_node_t *node,
                 bios_parameter_block_t bpb, char *file_name) {
    // Moves the file pointer to the contents of the next directory entry.
    fseek(disk, get_offset_from_cluster(d_entry.first_cluster, bpb), SEEK_SET);
    // Reads the contents of the directory entry to the variable next_location_contents
    uint8_t *next_location_contents =
        (uint8_t *) malloc(sizeof(uint8_t) * d_entry.file_size);
    assert(fread(next_location_contents, sizeof(uint8_t), d_entry.file_size, disk) ==
           d_entry.file_size);
    // Creates a file node and adds it to the child directory tree.
    file_node_t *fnode =
        init_file_node(file_name, d_entry.file_size, next_location_contents);
    add_child_directory_tree(node, (node_t *) fnode);
}

void follow(FILE *disk, directory_node_t *node, bios_parameter_block_t bpb) {
    while (true) {
        directory_entry_t d_entry;
        assert(fread(&d_entry, sizeof(directory_entry_t), 1, disk) == 1);
        // Finds the position of the file pointer to be used when resetting.
        size_t current_position = ftell(disk);
        char *file_name = get_file_name(d_entry);
        // Checks if the first character of the file name is NULL and breaks if it is.
        if (file_name[0] == '\0') {
            free(file_name);
            break;
        }
        if (!is_hidden(d_entry)) {
            if (!is_directory(d_entry)) {
                follow_file(disk, d_entry, node, bpb, file_name);
            }
            else {
                // Creates a directory node and adds it to the child directory tree.
                fseek(disk, get_offset_from_cluster(d_entry.first_cluster, bpb),
                      SEEK_SET);
                directory_node_t *dnode = init_directory_node(file_name);
                add_child_directory_tree(node, (node_t *) dnode);
                // Calls the follow function recursively with the child as a parameter
                follow(disk, dnode, bpb);
            }
        }
        else {
            free(file_name);
        }
        fseek(disk, current_position, SEEK_SET);
    }
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "USAGE: %s <image filename>\n", argv[0]);
        return 1;
    }

    FILE *disk = fopen(argv[1], "r");
    if (disk == NULL) {
        fprintf(stderr, "No such image file: %s\n", argv[1]);
        return 1;
    }

    bios_parameter_block_t bpb;
    // read the bios parameter block into the variable bpb.
    fseek(disk, MASTER_BOOT_RECORD_SIZE, SEEK_SET);
    size_t read = fread(&bpb, sizeof(bios_parameter_block_t), 1, disk);
    assert(read == 1);

    // Skips to the beginning of the root directory entries block.
    fseek(disk, get_root_directory_location(bpb), SEEK_SET);
    directory_node_t *root = init_directory_node(NULL);

    follow(disk, root, bpb);
    print_directory_tree((node_t *) root);
    create_directory_tree((node_t *) root);
    free_directory_tree((node_t *) root);

    int result = fclose(disk);
    assert(result == 0);
}
