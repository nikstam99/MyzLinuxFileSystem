#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include "../include/myz_func.h"

int INODES = 0;

int main(int argc, char* argv[]) {
    char start_dir[256];
    getcwd(start_dir, sizeof(start_dir));           // Αποθηκεύουμε στο start_dir το αρχικό working directory
    if (argc < 3) {
        printf("Error!\n");
    }
    char* flag[2];
    flag[0] = " ";
    flag[1] = " ";
    int exists = 0;
    Header H;
    H = malloc(sizeof(struct header));              // Δέσμευση μνήμης για το header του .myz file 
    H->size_of_files = 0;
    char* archive_name;
    int i = 0;
    
    // Διαχείριση των flags 
    for (i = 1; i < 4; i++) {
        if (!strcmp(argv[i], "-c") || !strcmp(argv[i], "-m") ||!strcmp(argv[i], "-q")
        || !strcmp(argv[i], "-p") || !strcmp(argv[i], "-a") || !strcmp(argv[i], "-x") || !strcmp(argv[i], "-j")) {
            flag[i-1] = argv[i];
            continue;
        }
        archive_name = argv[i];
        break;

    }
    int start = ++i;
    
    // Εκτέλεση του -j (zip files and store to myz array)
    if (!strcmp(flag[0], "-j") || !strcmp(flag[1], "-j")) {
        
        for (int i = start; i < argc; i++) {
            compress_file(argv[i], start_dir);
            
            char newArg[256];
            strcpy(newArg, argv[i]);     
            strcat(newArg, ".gz");     
            argv[i] = newArg;
            printf("%s\n", argv[i]);
            chdir(start_dir);
        }
    }
    List newlist;
    Header H2;
    int append = 0;

    // Εκτέλεση του -a (append files to myz array)
    if (!strcmp(flag[0], "-a") || !strcmp(flag[1], "-a")) {
        append = 1;
        chdir(start_dir);
        int fd = open(archive_name, O_RDONLY);
        if (fd == -1) {
            exists = 0;
        }
        else exists = 1;

        if (exists) {
            Header H2;
            lseek(fd, 0, SEEK_SET);
            H2 = read_header_from_file(fd);                                     // Διαβάζουμε το header

            // Τοποθετούμε τον file pointer μετά τα δεδομένα των αρχείων ώστε να διαβάσουμε τη λίστα 
            lseek(fd, sizeof(struct header) + H2->size_of_files, SEEK_SET);
            List newlist = read_list_from_file(fd);

            // Τροποποιούμε το argv[] για να μπουν και τα νέα αρχεία στο myz list
            for (int i = 2; i < newlist->dummy->next->size; i++) {
                argv[argc] = newlist->dummy->next->myznode[i].name;
                argc++;
            }
            // Αλλάζουμε το flag για να εκτελεστεί η εισαγωγή
            if (!strcmp(flag[0], "-a")) flag[0] = "-c";
            if (!strcmp(flag[1], "-a")) flag[1] = "-c";

            // Διαγράφουμε τα περιεχόμενα του .myz
            fd = open(archive_name, O_WRONLY | O_TRUNC);
            exists = 0;
            close(fd);
        }
    }
    // Εκτέλεση του -c (create myz list)
    if (!strcmp(flag[0], "-c") || !strcmp(flag[1], "-c") && !exists) {

        // Δημιουργούμε τη λίστα
        List myz_list = list_create(); 
        int dcount = 0;
        int fcount = 0;
        struct dirent *entry;
        struct stat fileStat;
        char file_path[256];
        FileInfo D;

        // Μετράμε συνολικά regular files και directories στη λίστα από αρχεία που δίνεται από το command prompt
        for (int k = start; k < argc; k++) {
            strcpy(D.filename, argv[k]);
            strcpy(D.path, start_dir);

            // Για κάθε αρχείο βρίσκουμε το parent path του και αλλάζουμε το dir για να το βρει η stat
            D = search_directory(&D, argv[k]);
            chdir(D.parent);

            if (stat(argv[k], &fileStat) == -1) {
                printf("%s \n", argv[k]);
                perror("stat1");
                return 1;
            }
            if (S_ISDIR(fileStat.st_mode)) {
                dcount++;
                
            }
            // Αν βρούμε regular file το εισάγουμε στη λίστα
            if (S_ISREG(fileStat.st_mode)) {
                fcount++;

                // Αν είναι το πρώτο που βρήκαμε δημιουργούμε νέο node κατά την εισαγωγή αλλίως απλά το εισάγουμε στην επόμενη θέση
                if (fcount == 1) create_node(argv[k], myz_list);
                else myz_add_file(argv[k], myz_list, 0);
            }
        }

        // Διατρέχουμε ξανά τα files/directories και τα εισάγουμε στη λίστα
        for (int k = start; k < argc; k++) {
            strcpy(D.filename, argv[k]);
            strcpy(D.path, start_dir);

            // Για κάθε αρχείο βρίσκουμε το parent path του και αλλάζουμε το dir για να το βρει η stat
            D = search_directory(&D, argv[k]);
            chdir(D.parent);

            if (stat(argv[k], &fileStat) == -1) {
                printf("%s \n", argv[k]);
                perror("stat2");
                return 1;
            }

            // Αν βρούμε directory την πρώτη φορά δημιουργούμε νέο κόμβο αλλίως το εισάγουμε στον τελευταίο
            if (S_ISDIR(fileStat.st_mode)) {
                if (fcount) myz_add_file(argv[k], myz_list, 0);
                else {
                    if (k == start)
                        create_node(argv[k], myz_list);
                    else 
                        myz_add_file(argv[k], myz_list, 0);
                }
            }
        }
        int c = 1;

        // Αν έχουμε μόνο ένα file/directory για εισαγωγή
        if (argc == 4) {
            if (stat(argv[3], &fileStat) == -1) {
                printf("%s \n", argv[3]);
                perror("stat3");
                return 1;
            }
            // Εισαγωγή ανάλογα αν είναι regular file ή directory
            if (S_ISDIR(fileStat.st_mode)) {
                dcount++;
                myz_add_dir(argv[3], myz_list, find_i_node(argv[3], myz_list), find_parent_dir(myz_list, argv[3]));
            }
            else {
                myz_add_file(argv[3], myz_list, 1);
                c = 2;
            }
        }
        char name[256];

        // Επανάληψη μέχρι να τελειώσουν τα στοιχεία στο list of files/directories
        while(c <= INODES) {
            
            // Αυξάνοντας το c κατά ένα σε κάθε επανάληψη ελέγχουμε τα περιεχόμενα του myz array σε κάθε node 
            // και τοποθετούμε στο κατάλληλο node τα files/directories που δεν έχουν εισαχθεί
            strcpy(name, find_file_by_number(myz_list, c));
            int node_num = find_i_node(name, myz_list);

            // Βρίσκουμε το node με βάση το inode
            Listnode node = find_node_by_num(myz_list, node_num);

            // Αν έχουμε πάνω από 2 files/directories στο list-of-files/dirs ξεκινάμε από το node 0
            if (argc > 4) {
                node = find_node_by_num(myz_list, c-1);
            }

            // Αν βρούμε regular file σε οποινδήποτε node εκτός του πρώτου τότε αυτό έχει ήδη εισαχθεί
            if (c > 1)
                if (!S_ISDIR(node->data.access_rights)) {
                    c++;
                    continue;
                }

            // Βρίσκουμε το αρχείο με βάση το name του
            strcpy(D.filename, name);
            strcpy(D.path, start_dir);
            D = search_directory(&D, name);
            chdir(D.parent);

            // Κάθε file/directory που υπάρχει στο myz array το εισάγουμε στη λίστα σε επόμενο κόμβο 
            for (int k = 2; k < node->size; k++) {
                strcpy(D.filename, node->myznode[k].name);
                strcpy(D.path, start_dir);
                D = search_directory(&D, node->myznode[k].name);
                chdir(D.parent);
                if (stat(node->myznode[k].name, &fileStat) == -1) {
                    printf("%s\n", node->myznode[k].name);
                    perror("stat4");
                    return 1;
                } 
                // Αν είναι directory το εισάγουμε σε νέο κόμβο και στο myz array βάζουμε τα περιεχόμενά του
                if (S_ISDIR(fileStat.st_mode))  {
                    dcount++; 
                    myz_add_dir(node->myznode[k].name, myz_list, find_i_node(node->myznode[k].name, myz_list), find_parent_dir(myz_list, node->myznode[k].name));   
                }
                // Αν είναι regular file το εισάγουμε σε νέο κόμβο μόνο του 
                else {
                    myz_add_file(node->myznode[k].name, myz_list, 1);     
                }  
            }
            c++;
        }
        chdir(start_dir);

        // Διατρέχουμε όλη τη λίστα και μετράμε το συνολικό μέγεθος των αρχείων
        for (Listnode node = myz_list->dummy->next; node != NULL; node = node->next) {
            if (S_ISREG(node->data.access_rights) && node->myznode->number != 0) {
                strcpy(D.filename, node->myznode->name);
                strcpy(D.path, start_dir);
                D = search_directory(&D, node->myznode->name);
                chdir(D.parent);
                H->size_of_files += node->data.size;
            }
        }
        chdir(start_dir);

        // Ανοίγουμε το .myz 
        int fd = open(archive_name, O_RDWR | O_CREAT, 0644);
        if (fd == -1) {
            perror("Error opening .myz file!");
            return 0;
        }
        chdir(start_dir);
        // Γράφουμε το header στο .myz
        write_header_to_file(fd, H);

        // Διατρέχουμε όλα τα nodes της λίστας και ανοίγουμε τα regular files
        for (Listnode node = myz_list->dummy->next; node != NULL; node = node->next) {
            if (S_ISREG(node->data.access_rights) && node->myznode->number != 0) {
                strcpy(D.filename, node->myznode->name);
                strcpy(D.path, start_dir);
                D = search_directory(&D, node->myznode->name);
                chdir(D.parent);
                int fdo = open(node->myznode->name, O_RDONLY);

                if (fdo == -1) {
                    perror("Error opening file");
                    return 1;
                }

                char buffer[1024];
                int bytes_read;
                int bytes_written;

                // Aντιγράφουμε το περιεχόμενο του κάθε file σε έναν buffer και μετά το γράφουμε στο .myz
                while ((bytes_read = read(fdo, buffer, 1023)) > 0) {
                    buffer[bytes_read] = '\0';
                    bytes_written = write(fd, buffer, bytes_read);
                    if (bytes_written != bytes_read) {
                        perror("Error writing to destination file");
                    }
                }
                if (bytes_read == -1) {
                    perror("Error reading file");
                }
                chdir(start_dir);
                close(fdo);                
            }
        }

        // Γράφουμε όλη τη λίστα στο .myz
        write_list_to_file(fd, myz_list);

        close(fd);
        
        list_destroy(myz_list);
        
        // Aν έχει γίνει zipped κάποιο αρχείο το κάνουμε unzip
        if (!strcmp(flag[0], "-j") || !strcmp(flag[1], "-j")) { 
            for (int i = start; i < argc; i++) {
                printf("%s\n", argv[i]);
                strcpy(D.filename, argv[i]);
                strcpy(D.path, start_dir);
                D = search_directory(&D, argv[i]);
                chdir(D.parent);
                decompress_file(D.filename, start_dir);
                chdir(start_dir);
            }
        }
    }

    // Εκτέλεση του -m (print metadata)
    else if (!strcmp(flag[0], "-m")) {
        chdir(start_dir);
        int fd = open(archive_name, O_RDONLY);
        if (fd == -1) {
            perror("Error opening .myz file!");
            return 1;
        }
        if (fd == 0) printf("empty\n");
        Header H2;

        lseek(fd, 0, SEEK_SET);

        // Διαβάζουμε το header που βρίσκεται στην αρχή του .myz
        H2 = read_header_from_file(fd);

        // Μετακινούμε τον file pointer μετά τα δεδομένα των files 
        lseek(fd, sizeof(struct header) + H2->size_of_files, SEEK_SET);

        // Διαβάζουμε τη λίστα από το .myz
        List newlist = read_list_from_file(fd);

        // Εκτυπώνουμε τη λίστα με τα metadata
        print_list(newlist);
        close(fd);

        // Αποδεσμεύουμε τη μνήμη
        list_destroy(newlist);
        free(H2);
    }

    // Εκτέλεση του -q (question about file/dir)
    else if (!strcmp(flag[0], "-q")) {
        chdir(start_dir);
        int fd = open(archive_name, O_RDONLY);
        if (fd == -1) {
            perror("Error opening .myz file!");
            return 1;
        }
        if (fd == 0) printf("empty\n");
        Header H2;
        lseek(fd, 0, SEEK_SET);

        // Διαβάζουμε το header που βρίσκεται στην αρχή του .myz
        H2 = read_header_from_file(fd);

        printf("%d\n", H2->size_of_files);

        // Μετακινούμε τον file pointer μετά τα δεδομένα των files 
        lseek(fd, sizeof(struct header) + H2->size_of_files, SEEK_SET);

        // Διαβάζουμε τη λίστα από το .myz
        List newlist = read_list_from_file(fd);
        
        // Για κάθε file/directory από το list-of-files/dirs ελέγχουμε αν υπάρχει στη λίστα του .myz
        // και τυπώνουμε ανάλογα αποτελέσματα
        for (int i = start; i < argc; i++) {
            int num = find_i_node(argv[i], newlist);
            printf("\n\n------------------------------\n");
            if (num != -1) printf("%s: --YES-- (INODE: %d)\n", argv[i], num);
            else printf("%s: --NO--\n", argv[i]);
            printf("------------------------------\n");
            
        }
        close(fd);

        // Αποδεσμεύουμε τη μνήμη
        free(H2);
        list_destroy(newlist);
    }

    // Εκτέλεση του -p (print hierarchy)
    else if (!strcmp(flag[0], "-p")) {
        chdir(start_dir);
        int fd = open(archive_name, O_RDONLY);
        if (fd == -1) {
            perror("Error opening .myz file!");
            return 1;
        }
        if (fd == 0) printf("empty\n");
        Header H2;
        lseek(fd, 0, SEEK_SET);

        // Διαβάζουμε το header που βρίσκεται στην αρχή του .myz
        H2 = read_header_from_file(fd);

        // Μετακινούμε τον file pointer μετά τα δεδομένα των files 
        lseek(fd, sizeof(struct header) + H2->size_of_files, SEEK_SET);

        // Διαβάζουμε τη λίστα από το .myz
        List newlist = read_list_from_file(fd);

        int count = 0;

        // Για κάθε node του myz list εκτυπώνουμε τα περιεχόμενα του
        for (int i = 0; i < newlist->size; i++) {
            Listnode node = find_node_by_num(newlist, i);
            if (S_ISDIR(node->data.access_rights)) {
                count++;
                printf("\n\n-----------------------------\n");
                printf("%d. %s\n",count,  find_file_by_number(newlist, i));
                printf("-----------------------------\n");
                if (node->size == 2) printf("EMPTY\n"); 
                for (int j = 2; j < node->size; j++) {
                    printf("%s\n", node->myznode[j].name);
                }
                printf("-----------------------------\n");
            }
        }

        // Αποδεσμεύουμε τη μνήμη
        free(H2);
        list_destroy(newlist);
    }

    // Εκτέλεση του -x (extract all files/directories)
    else if (!strcmp(flag[0], "-x")) {
        chdir(start_dir);
        int fd = open(archive_name, O_RDONLY);
        if (fd == -1) {
            perror("Error opening .myz file!");
            return 1;
        }
        if (fd == 0) printf("empty\n");
        Header H2;
        lseek(fd, 0, SEEK_SET);

        // Διαβάζουμε το header που βρίσκεται στην αρχή του .myz
        H2 = read_header_from_file(fd);
        char buffer[1024];

        // Μετακινούμε τον file pointer μετά τα δεδομένα των files
        lseek(fd, sizeof(struct header) + H2->size_of_files, SEEK_SET);

        // Διαβάζουμε τη λίστα από το .myz
        List newlist = read_list_from_file(fd);
        int size = 0;
        int fdo;

        // Μετακινούμε τον file pointer πίσω στο data segment για να διαβάσουμε τα περιεχόμενα των αρχείων
        lseek(fd, sizeof(struct header), SEEK_SET);

        chdir(start_dir);
        FileInfo D;
        int I = INODES;
        INODES = 0;
        int dirs = 0;
        int count = 1;
        int found = 0;

        // Για κάθε node ελέγχουμε τη list-of-files/dirs και αν υπάρχει το κάνουμε extract
        for (Listnode node = newlist->dummy->next->next; node != NULL; node = node->next) {
            if (argc > 3) {
                for (int i = start; i < argc; i++) {
                    if (!strcmp(argv[i], find_file_by_number(newlist, count))) {
                        found = 1;
                        INODES = count-1;
                        break;
                    }
                }
                // Αν δεν υπάρχει συνεχίζουμε στον επόμενο node
                if (!found) {
                    count++;
                    continue;
                }
            }

            // Αν βρούμε directory το κάνουμε clone ολόκληρο μαζί με τα περιεχόμενά του αναδρομικά
            if (S_ISDIR(node->data.access_rights)) {
                dirs++;
                strcpy(D.filename, find_file_by_number(newlist, node->myznode[0].number));
                strcpy(D.path, start_dir);
                D = search_directory(&D, find_file_by_number(newlist, node->myznode[0].number));
                clone_directory(D.filename, newlist, start_dir, fd);
            }

            // Αν βρούμε αρχείο και είμαστε ακόμα στο πρώτο node τότε το κάνουμε clone ξεχωριστά 
            // αντιγράφοντας τα δεδομένα του στο νέο αρχείο
            else if (S_ISREG(node->data.access_rights) && dirs == 0) {
                size = node->data.size;
                fdo = open(node->myznode->name, O_WRONLY | O_CREAT, 0644);
                read(fd, &buffer, size);
                buffer[size] = '\0';
                write(fdo, &buffer, size);
                chmod(node->myznode->name, S_IRWXU | S_IRGRP | S_IROTH);
                close(fdo);

                // Αν το αρχείο που θέλουμε να κάνουμε extract είναι zipped τότε το κάνουμε unzip πριν το extract (clone)
                size_t len = strlen(node->myznode->name);
                if (strcmp(node->myznode->name + len - 3, ".gz") == 0) {
                    char d[256];
                    getcwd(d, sizeof(d));
                    decompress_file(node->myznode->name, d);
                }
            }
            count++;
        }

        // Μήνυμα επιτυχίας εκτέλεσης του extract 
        printf("\n\n------------------------------------------------------\n");
        printf("Extracted .myz file to current working directory!!!\n");
        printf("------------------------------------------------------\n\n");
        INODES = I;
        close(fd);

        // Αποδεσμέυουμε τη μνήμη
        free(H2);
        list_destroy(newlist);
    }
    free(H);
}