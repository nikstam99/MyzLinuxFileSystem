#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>

typedef struct list* List;
typedef struct list_node* Listnode;
typedef struct metadata* Metadata;
typedef struct header* Header;
typedef struct inode* Inode;

extern int INODES; 

#define LIST_BOF (Listnode)0
#define LIST_EOF (Listnode)0

// Δομή της λίστας
struct list {
    Listnode last; 		// Τελευταίος κόμβος της λίστας
    int size; 			// Μέγεθος λίστας
    Listnode dummy; 	// Πρώτος (βοηθητικός) κόμβος της λίστας χωρίς στοιχεία 
};

// Δομή που περιέχει τις μεταπληροφορίες ενός file/directory
struct metadata {
    uid_t owner;
    gid_t group;
    mode_t access_rights;
    time_t timestamp;
    int nested;
    off_t size;
};

// Κόμβος της λίστας
struct list_node {
    Listnode next; 		    // Δείκτης στον επόμενο κόμβο της λίστας
    struct metadata data;   // Metadata του αρχείου
    Inode myznode;          // Πίνακας που αποθηκεύονται τα περιεχόμενα ενός directory ή το ίδιο το file
    int size;               // Μέγεθος του myz array
};

// Header ενός αρχείου
struct header {
    int size_of_files;     // Το μέγεθος σε bytes όλων των αποθηκευμένων αρχείων στο myz list
};

// Πληροφορίες σχετικά με το inode 
struct inode {              
    int number;            // Ο αριθμός inode που είναι αποθηεκυμένο στη λίστα
    char name[256];        // Το όνομα του αρχείου
};

//Δημιουργία της λίστας
List list_create();

// Εισάγει ένα στοιχείο στη λίστα στη θέση μετά το node
void list_insert_next(List list, Listnode node, struct metadata data, int flag);

// Διαγραφή της λίστας
void list_destroy(List list);

// Επιστρέφει το τελευταίο node της λίστας
Listnode list_last(List list);

// Εκτύπωση των περιεχομένων της λίστας
void print_list(List list);

// Εισαγωγή στοιχείου στο myz array
void insert_to_myz_array(Listnode node, struct inode in);

// Επιστρέφει το inode ενός αρχείου με βάση το όνομά του
int find_i_node(char* arg, List list);

// Επιστρέφει το node που είναι αποθηκευμένο το αρχείο arg στη λίστα
Listnode find_list_node(char* arg, List list);

// Επιστρέφει το node με inode number
Listnode find_node_by_num(List list, int number);

// Επιστρέφει το inode του parent directory του arg
int find_parent_dir(List list, char* arg);

// Επιστρέφει το όνομα ενός αρχείου με βάση το inode του
char* find_file_by_number(List list, int number);

// Κλονοποιεί αναδρομικά ένα directory και όλα τα περιεχόμενα του 
void clone_directory(char *dir_name, List myz_node_list, char* start_dir, int fd);