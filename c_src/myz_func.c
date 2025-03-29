#include "../include/myz_func.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

void print_permissions(mode_t mode) {
    printf((mode & S_IRUSR) ? "r" : "-");
    printf((mode & S_IWUSR) ? "w" : "-");
    printf((mode & S_IXUSR) ? "x" : "-");
    printf((mode & S_IRGRP) ? "r" : "-");
    printf((mode & S_IWGRP) ? "w" : "-");
    printf((mode & S_IXGRP) ? "x" : "-");
    printf((mode & S_IROTH) ? "r" : "-");
    printf((mode & S_IWOTH) ? "w" : "-");
    printf((mode & S_IXOTH) ? "x" : "-");
    printf("\n");
}

int myz_add_dir(char* arg, List myz_list, int num, int parent) {
    DIR *dir = opendir(arg);    // Ανοίγει το arg directory και το αποθηκεύει στο dir
    struct dirent *entry;
    if (dir == NULL) {
        printf("%s \n", arg);
        perror("opendir");
        return 1;
    }
    int fcount = 0;
    int dcount = 0;
    int count = 0;

    while ((entry = readdir(dir)) != NULL) {                // Μετράμε πόσα files και πόσα directories περιέχει το dir
        count++;
        if (entry->d_type == 4) dcount++;
        else if (entry->d_type == 8) fcount++;
    }
    closedir(dir);
    dir = opendir(arg);
    struct stat fileStat;
    Inode array = malloc(sizeof(struct inode) * count);     // Δέσμευση μνήμης για το myz node array, ανάλογα με το πόσα αρχεία έχει το directory
    struct metadata meta;
    int j = 0;
    int files = 0;
    int dirs = 0;
    if (stat(arg, &fileStat) == -1) {                       // Αποθήκευση μεταπληροφοριών του arg στο filestat
        perror("stat");
        return 1;
    }
    while ((entry = readdir(dir)) != NULL) {               // Αποθηκέυουμε το . και το .. directories στο array
        if (!strcmp(entry->d_name, ".")) {
            array[0].number = num;
            array[0].name[0] = '.';
            array[0].name[1] = '\0';
        }
        else if (!strcmp(entry->d_name, "..")) {
            array[1].number = parent;
            array[1].name[0] = '.';
            array[1].name[1] = '.';
            array[1].name[2] = '\0';
        }
        else {                                             // Αποθηκεύουμε τα ονόματα των αρχείων στο array
            if (entry->d_type == 8) {
                files++;
                for(int i = 0; i < 256; i++) {
                    array[files+1].name[i] = entry->d_name[i];
                }
                
            }
            else if (entry->d_type == 4){
                dirs++;
                for(int i = 0; i < 256; i++) {
                    array[dirs+fcount+1].name[i] = entry->d_name[i];
                }
            }
        }
        j++;
    }
    meta.group = fileStat.st_gid;                                // Κάνουμε προσπέλαση στα metadata του directory 
    meta.owner = fileStat.st_uid;
    meta.timestamp = fileStat.st_mtime;
    meta.access_rights = fileStat.st_mode;
    meta.size = fileStat.st_size;
    if (count == 2) meta.nested = 0;
    else meta.nested = 1;

    closedir(dir);
    list_insert_next(myz_list, list_last(myz_list), meta, 1);   // Προσθέτουμε έναν νέο node στη λίστα και αποθηκέυουμε σε αυτό τα metadata
    for (int i = 0; i < count; i++) {
        if (strcmp(array[i].name, ".") && strcmp(array[i].name, "..")) array[i].number = ++INODES;  // Αυξάνουμε τα συνολικά INODES που υπάρχουν στη λίστα
        insert_to_myz_array(list_last(myz_list), array[i]);                                         // Προσθέτουμε το array στο myz node
    }
    free(array);
    return 0;
}

int myz_add_file(char* arg, List myz_list, int insert_next) {
    struct stat fileStat;
    if (stat(arg, &fileStat) == -1) {                           // Αποθήκευση μεταπληροφοριών του arg στο filestat
        perror("stat");
        return 1;
    }
    struct metadata meta;
    struct inode in;
    meta.group = fileStat.st_gid;                               // Κάνουμε προσπέλαση στα metadata του file
    meta.owner = fileStat.st_uid;
    meta.timestamp = fileStat.st_mtime;
    meta.access_rights = fileStat.st_mode;
    meta.size = fileStat.st_size;
    meta.nested = 0;
    strcpy(in.name, arg);

    in.number = 0;
    if (S_ISDIR(fileStat.st_mode)) in.number = ++INODES;            // Αυξάνουμε τα συνολικά INODES που υπάρχουν στη λίστα
    if (insert_next) {
        list_insert_next(myz_list, list_last(myz_list), meta, 1);   // Προσθέτουμε έναν νέο node στη λίστα και αποθηκέυουμε σε αυτό τα metadata 
        in.number = myz_list->size-1;                               // Ο αριθμός του inode είναι το μέγεθος της λίστας -1 (αφόυ ξεκινάμε από το 0)
    }
    insert_to_myz_array(list_last(myz_list), in);                   // Προσθέτουμε το file στο myz array
    return 0;
}

int create_node(char* arg, List myz_list) {
    Inode array = malloc(sizeof(struct inode) * 3);                 // Δέσμευση μνήμης για το myz array με αρχική χωρητικότητα 3 files/dirs (., .. και arg)

    array[0].number = 0;                                            // Αποθηκέυουμε το . και το .. directories στο array
    array[0].name[0] = '.';
    array[0].name[1] = '\0';

    array[1].number = 0;
    array[1].name[0] = '.';
    array[1].name[1] = '.';
    array[1].name[2] = '\0';

    struct stat fileStat;                                           // Αποθήκευση μεταπληροφοριών του arg στο filestat
    if (stat(arg, &fileStat) == -1) {       
        perror("stat");
        return 1;
    }

    struct metadata meta;                                           // Κάνουμε προσπέλαση στα metadata του file
    meta.group = fileStat.st_gid;
    meta.owner = fileStat.st_uid;
    meta.timestamp = fileStat.st_mtime;
    meta.access_rights = fileStat.st_mode;
    meta.size = fileStat.st_size;
    meta.nested = 1;

    strcpy(array[2].name, arg);
    array[2].number = ++INODES;                                                         // Το arg file θα έχει Inode INODES(συνολικά inodes) + 1
    list_insert_next(myz_list, list_last(myz_list), meta, 1);                           // Προσθέτουμε έναν νέο node στη λίστα και αποθηκέυουμε σε αυτό τα metadata 
    for (int i = 0; i < 3; i++) insert_to_myz_array(list_last(myz_list), array[i]);     // Προσθέτουμε το array στο myz node
    free(array);
    return 0;
}

FileInfo search_directory(FileInfo *info, const char *target) {
    DIR *dir = opendir(info->path);
    FileInfo not_found_info = {"", "", ""};  // Αν δεν βρεθεί το αρχείο
    if (dir == NULL) {
        perror("opendir failed");
        return not_found_info;  // Επιστρέφουμε το empty struct
    }

    struct dirent *entry;
    FileInfo new_info;

    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;

                                                               
        strcpy(new_info.path, info->path);                       // Δημιουργία του πλήρους path
        strcat(new_info.path, "/");
        strcpy(new_info.filename, entry->d_name);
        strcat(new_info.path, new_info.filename);

        
        strcpy(new_info.parent, info->path);                    // Ορισμός του parent_dir πριν την αναδρομή
        
        if (strcmp(entry->d_name, target) == 0) {               // Έλεγχος αν το όνομα ταιριάζει με το target file
            closedir(dir);
            return new_info;                                    // Επιστρέφουμε το struct με τα αποτελέσματα
        }

        
        struct stat file_stat;
        if (stat(new_info.path, &file_stat) == 0 && S_ISDIR(file_stat.st_mode)) {   // Αν είναι φάκελος, κάνουμε αναδρομική αναζήτηση
            FileInfo result = search_directory(&new_info, target);
            if (strlen(result.path) > 0) {                                          // Αν βρούμε το αρχείο, επιστρέφουμε το struct με τα αποτελέσματα
                closedir(dir);
                return result;                                                              
            }
        }
    }

    closedir(dir);
    return not_found_info;                                                          // Επιστρέφουμε το empty struct αν δεν βρεθεί το αρχείο
}

void write_list_to_file(int fd, List list) {
    if (!list) return;

    
    write(fd, &list->size, sizeof(int));                                // Γράφουμε το μέγεθος της λίστας

    int i = 0;
    Listnode node = find_node_by_num(list, i);
    while (i < list->size) {
        write(fd, &node->data, sizeof(struct metadata));                // Γράφουμε τα metadata

        write(fd, &node->size, sizeof(int));                            // Γράφουμε τον αριθμό inodes

        for (int i = 0; i < node->size; i++) {
            write(fd, &node->myznode[i], sizeof(struct inode));         // Γράφουμε το array του myznode
        }
        i++;
        node = find_node_by_num(list, i);
    }
}

List read_list_from_file(int fd) {
    List list = list_create();
    
    int size;
    read(fd, &size, sizeof(int));                                       // Διαβάζουμε το μέγεθος της λίστας
    
    for (int i = 0; i < size; i++) {
        struct metadata data;
        int node_size;

        read(fd, &data, sizeof(struct metadata));                       // Διαβάζουμε τα metadata

        read(fd, &node_size, sizeof(int));                              // Διαβάζουμε τον αριθμό inodes

        list_insert_next(list, list_last(list), data, 0);
        Listnode new_node = list_last(list);
        new_node->size = node_size;

        new_node->myznode = malloc(node_size * sizeof(struct inode));   // Δεσμεύουμε μνήμη για το myz array

        read(fd, new_node->myznode, node_size * sizeof(struct inode));  // Διαβάζουμε τα περιεχόμενα του myz array
    }

    return list;
}

void write_header_to_file(int fd, Header header) {
    write(fd, &header->size_of_files, sizeof(int));         
}

Header read_header_from_file (int fd) {
    Header H = malloc(sizeof(struct header));
    int size;
    read(fd, &size, sizeof(int));
    H->size_of_files = size;
    return H;
}

void compress_file(const char *filename, char* dir) {
    FileInfo D;
    pid_t pid = fork();                             // Δημιουργία νέου process
    
    if (pid == -1) { 
        perror("fork failed");
        exit(1);
    } 
    else if (pid == 0) {                            // Child process
        strcpy(D.filename, filename);
        strcpy(D.path, dir);
        D = search_directory(&D, filename);
        chdir(D.parent);
        execlp("gzip", "gzip", D.filename, NULL);
        perror("execlp failed");  
        exit(1);
    } 
    else {                                          // Parent process
        int status;
        waitpid(pid, &status, 0);                   // Περιμένει το child να τελειώσει
    }
}

void decompress_file(const char *filename, char *dir) {
    FileInfo D;
    pid_t pid = fork();                            // Δημιουργία νέου process
    
    if (pid == -1) { 
        perror("fork failed");
        exit(1);
    } 
    else if (pid == 0) {                            // Child process
        strcpy(D.filename, filename);
        strcpy(D.path, dir);
        D = search_directory(&D, filename);
        chdir(D.parent);
        execlp("gunzip", "gunzip", filename, NULL);
        perror("execlp failed");        
        exit(1);
    } 
    else {                                          // Parent process
        int status;
        waitpid(pid, &status, 0);                   // Περιμένει το child να τελειώσει
    }
}