#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "myz_node_list.h"

// Struct που περιέχει πληροφορίες σχετικά με ένα αρχείο
typedef struct {
    char filename[256]; // όνομα αρχείου
    char path[256];     // full path αρχείου
    char parent[256];   // parent path αρχείου
} FileInfo;

// Εκτυπώνει τα permissions ενός αρχείου
void print_permissions(mode_t mode);

// Προσθέτει ένα Directory και τα περιεχόμενά του στο myz list
int myz_add_dir(char* arg, List myzlist, int num, int parent);

// Δημιουργεί ένα νέο node στο myz list (αν insert_next = 0) και προσθέτει ένα αρχείο στο myz node array
int myz_add_file(char* arg, List myz_list, int insert_next);

// Δημιουργεί ένα νέο node στο myz list
int create_node(char* arg, List myz_list);

// Διατρέχει αναδρομικά το current directory μέχρι να βρει ένα target file
FileInfo search_directory(FileInfo *info, const char *target);

// Γράφει το myz list σε ένα αρχείο
void write_list_to_file(int fd, List list);

// Διαβάζει το myz list από ένα αρχείο με βάση τον fd του και επιστρέφει τη λίστα
List read_list_from_file(int fd);

// Γράφει το Header στο αρχείο με file descriptor fd
void write_header_to_file(int fd, Header header);

// Διαβάζει το Header από ένα αρχείο
Header read_header_from_file (int fd);

// Κάνει compress το αρχείο filename με τη χρήση του gzip
void compress_file(const char *filename, char* dir);

// Κάνει decompress το αρχείο filename με τη χρήση του gunzip
void decompress_file(const char *filename, char *dir);