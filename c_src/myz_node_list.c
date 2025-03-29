#include "../include/myz_func.h"

List list_create() {
    List list = malloc(sizeof(*list)); // Δέσμευση μνήμης για τη λίστα
    list->size = 0;	
    list->dummy = malloc(sizeof(*list->dummy)); // δέσμευση μνήμης για τον dummy κόμβο
	list->dummy->next = NULL;	
    list->last = list->dummy;
    return list;
}

void list_insert_next(List list, Listnode node, struct metadata data, int flag) {
	// Αν δεν δωθεί κόμβος εισάγουμε μετά τον dummy
	if (node == NULL)
		node = list->dummy;
	// Δημιουργία του νέου κόμβου
	Listnode new = malloc(sizeof(*new));
    new->data = data;
    if (flag) new->myznode = malloc(sizeof(*new->myznode));
    new->size = 0;
	// Σύνδεση του new ανάμεσα στο node και το node->next
	new->next = node->next;
	node->next = new;
	// Ενημέρωση των size & last
	list->size++;
	if (list->last == node)
		list->last = new;
}

void list_destroy(List list) {
	Listnode node = list->dummy;
	while (node != NULL) {
		Listnode next = node->next;		
		if (node != list->dummy) {
			if (node->size > 0) {
				free(node->myznode);
			}
		}
		free(node);	// Απελευθερώνουμε τον ίδιο τον κόμβο
		node = next;
	}

	free(list);	// Απελευθερώνουμε τη λίστα
}

Listnode list_last(List list) {
	// Αν η λίστα είναι κενή ο last είναι ο dummy και επιστρέφουμε 0 αλλίως επιστρέφουμε το last της λίστας
	if (list->last == list->dummy)
		return LIST_EOF;		
	else
		return list->last;
}

void print_list(List list) {
	struct tm *timeinfo;
	char buffer[80];
	for (Listnode node = list->dummy->next; node != NULL; node = node->next) {
		timeinfo = localtime(&node->data.timestamp);								// Μετατροπή του timestamp σε local time
		strftime(buffer, sizeof(buffer), "%d/%m/%Y %H:%M:%S", timeinfo);			// Εκτύπωση της ώρας
        printf("\n\n-------------------------\n");
		printf("Owner: %u  Group: %u\n", node->data.owner, node->data.group);
		print_permissions(node->data.access_rights);
		printf("Date modified: %s  \nNested: %d\n", buffer, node->data.nested);
		printf("-------------------------\n");
		for (int i = 0; i < node->size; i++) {
            printf("%s %d\n",node->myznode[i].name, node->myznode[i].number);
        }
		printf("-------------------------\n");
	}
}

void insert_to_myz_array(Listnode node, struct inode in) {
    node->size++;
	node->myznode = realloc(node->myznode, node->size*sizeof(*node->myznode));		// Realloc για επιμήκυνση του πίνακα και δέσμευση νέας μνήμης
    for (int i = 0; i < 256; i++) {
        node->myznode[node->size-1].name[i] = in.name[i]; 
    }
    node->myznode[node->size-1].number = in.number;
}

int find_i_node(char* arg, List list) {
	for (Listnode node = list->dummy->next; node != NULL; node = node->next) {
		for (int i = 0; i < node->size; i++)
			if (!strcmp(node->myznode[i].name, arg)) return node->myznode[i].number;
	}
	return -1;
}

Listnode find_list_node(char* arg, List list) {
	for (Listnode node = list->dummy->next; node != NULL; node = node->next) {
		if (node->myznode[0].number == find_i_node(arg, list)) return node;
	}
	return NULL;
}

Listnode find_node_by_num(List list, int number) {
	int i = 0;
	for (Listnode node = list->dummy->next; node != NULL; node = node->next) {
		if (i == number) return node;
		i++;
	}
	return NULL;
}

int find_parent_dir(List list, char* arg) {
	for (Listnode node = list->dummy->next; node != NULL; node = node->next) {
		for (int i = 0; i < node->size; i++)
			if (!strcmp(node->myznode[i].name, arg)) return node->myznode[0].number;
	}	
}

char* find_file_by_number(List list, int number) {
	for (Listnode node = list->dummy->next; node != NULL; node = node->next) {
		for (int i = 2; i < node->size; i++)
			if (node->myznode[i].number == number) return node->myznode[i].name;
	}		
}




void clone_directory(char *dir_name, List myz_node_list, char* start_dir, int fd) {
    Listnode dir_node = find_list_node(dir_name, myz_node_list);
    if (!dir_node) {
        printf("Directory '%s' not found in myz_node_list.\n", dir_name);
        return;
    }

    // Δημιουργία clone directory
	if (INODES < find_i_node(dir_name, myz_node_list)) {
    	if (mkdir(dir_name, 0755) == -1) {
        	return;
    	}
		else INODES++;
	}

    // Αλλάζουμε current working directory στον νέο φάκελο
    if (chdir(dir_name) == -1) {
        return;
    }
	struct stat fileStat;
    for (int i = 2; i < dir_node->size; i++) {
		Listnode node2 = find_list_node(dir_node->myznode[i].name, myz_node_list);		// Βρίσκουμε το node που είναι αποθηκευμένο το file
		if (S_ISDIR(node2->data.access_rights)) {										// Αν είναι directory καλούμε και πάλι την clone directory για να διατρέξουμε όλα τα πιθανά files
            clone_directory(dir_node->myznode[i].name, myz_node_list, start_dir, fd);
        } 
		else {																			// Αν είναι regular file το δημιουργόυμε από την αρχή και αντιγράφουμε το περιεχόμενό του
			char current_dir[256];
			getcwd(current_dir, sizeof(current_dir));
			FileInfo D;
			strcpy(D.filename, dir_node->myznode[i].name);
			strcpy(D.path, start_dir);
			chdir(start_dir);
			D = search_directory(&D, dir_node->myznode[i].name);						// Βρίσκουμε το file 
			chdir(current_dir);

            int dest_fd = open(dir_node->myznode[i].name, O_WRONLY | O_CREAT, 0644);	// Δημιουργόυμε το αντίγραφο 
            if (dest_fd == -1) {
                continue;
            }
			else INODES++;

            char buffer[1024];															// Ψάχνουμε στο data section του .myz και μετακινούμε τον δείκτη μέχρι να βρούμε την αρχή του file
			int total = sizeof(struct header);
			lseek(fd, total , SEEK_SET);												
			int num = 1, count = 0;
			for (Listnode node3 = myz_node_list->dummy->next->next; node3 != NULL; node3 = node3->next) {
				if (find_file_by_number(myz_node_list, num) == dir_node->myznode[i].name) break;
				else if (S_ISREG(node3->data.access_rights)) {
					count++;
					total+= node3->data.size;											// Αυξάνουμε τον δείκτη κατά file size κάθε φορά που βρίσκουμε αρχείο στο myz list 
				}
				num++;
			}

			lseek(fd, total, SEEK_SET);
			node2 = find_node_by_num(myz_node_list, num);
			int bytes_read;
			int bytes_written;
			while ((bytes_read = read(fd, buffer, sizeof(buffer))) > 0) {				// Αντιγράφουμε τα περιεχόμενα του file από το .myz στο clone
				bytes_written = write(dest_fd, buffer, bytes_read);
				if (bytes_written != bytes_read) {
					perror("Error writing to destination file");
				}
			}
			
			chmod(dir_node->myznode[i].name, S_IRWXU | S_IRGRP | S_IROTH);				// Τροποποιούμε τα permissions του file
            close(dest_fd);
			size_t len = strlen(dir_node->myznode[i].name);
			if (strcmp(dir_node->myznode[i].name + len - 3, ".gz") == 0) {				// Αν το αρχείο είναι zipped, το κάνουμε unzip
				char d[256];
				getcwd(d, sizeof(d));
				decompress_file(dir_node->myznode[i].name, d);
			}
			chdir(current_dir);
        }
    }

    // Επιστροφή στον αρχικό φάκελο
    chdir("..");
}
