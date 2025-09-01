#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "directory.h"

#define TRUE 1
#define FALSE 0

struct entryNode {
    char * name;
    struct entryNode * next;
    int isDirectory;
    struct entryNode * parent;
    union {
        char * contents;
        struct entryNode * entryList;
    } entry;
};

struct entryNode * root;

void insertInOrder(struct entryNode **list, struct entryNode *newNode) {
    struct entryNode *current = *list;
    struct entryNode *prev = NULL;

    while (current != NULL && strcmp(current->name, newNode->name) < 0) {
        prev = current;
        current = current->next;
    }
    newNode->next = current;
    if (prev == NULL) {
        *list = newNode;
    } else {
        prev->next = newNode;
    }
}

struct entryNode * initialFileSystem() {
    if (root == NULL) {
        root = (struct entryNode *) malloc(sizeof(struct entryNode));
        root->name = strdup("/");
        root->isDirectory = 1;
        root->parent = root;
        root->entry.entryList = NULL;
        root->next = NULL;
    }
    return root;
}

struct entryNode * located(char * name, struct entryNode * list) {
    if (list == NULL) return NULL;
    if (strcmp(list->name, name) == 0) return list;
    return located(name, list->next);
}

void createFile(struct entryNode * wd, char * fileName) {
    if (located(fileName, wd->entry.entryList)) {
        printf("create: %s: File exists\n", fileName);
        return;
    }
    struct entryNode * newFile = malloc(sizeof(struct entryNode));
    newFile->name = strdup(fileName);
    newFile->isDirectory = 0;
    newFile->parent = wd;
    newFile->next = NULL;

    size_t capacity = 128;
    size_t length = 0;
    char * buffer = malloc(capacity);
    int prevChar = 0, currChar;
    while ((currChar = getchar()) != EOF) {
        if (length + 1 >= capacity) {
            capacity *= 2;
            buffer = realloc(buffer, capacity);
        }
        buffer[length++] = currChar;
        if (prevChar == '\n' && currChar == '\n') {
            buffer[length - 1] = '\0';
            break;
        }
        prevChar = currChar;
    }
    newFile->entry.contents = strdup(buffer);
    free(buffer);

    insertInOrder(&wd->entry.entryList, newFile);
}

void createDir(struct entryNode * wd, char * dirName) {
    if (located(dirName, wd->entry.entryList)) {
        printf("mkdir: %s: File exists\n", dirName);
        return;
    }
    struct entryNode * newDir = malloc(sizeof(struct entryNode));
    newDir->name = strdup(dirName);
    newDir->isDirectory = 1;
    newDir->parent = wd;
    newDir->entry.entryList = NULL;
    newDir->next = NULL;

    insertInOrder(&wd->entry.entryList, newDir);
}

void removeFile(struct entryNode * wd, char * fileName) {
    struct entryNode * file = located(fileName, wd->entry.entryList);
    if (file == NULL) {
        printf("rm: %s: No such file or directory.\n", fileName);
        return;
    } else if (file->isDirectory) {
        printf("rm: %s: is a directory.\n", fileName);
        return;
    }

    struct entryNode * current = wd->entry.entryList;
    struct entryNode * prev = NULL;
    while (current != NULL && current != file) {
        prev = current;
        current = current->next;
    }
    if (prev == NULL) {
        wd->entry.entryList = file->next;
    } else {
        prev->next = file->next;
    }
    free(file->name);
    free(file->entry.contents);
    free(file);
}

void removeDir(struct entryNode * wd, char * dirName) {
    struct entryNode * dir = located(dirName, wd->entry.entryList);
    if (dir == NULL) {
        printf("rmdir: %s: No such file or directory.\n", dirName);
        return;
    } else if (!dir->isDirectory) {
        printf("rmdir: %s: Not a directory.\n", dirName);
        return;
    } else if (dir->entry.entryList != NULL) {
        printf("rmdir: %s: Directory not empty\n", dirName);
        return;
    }

    struct entryNode * current = wd->entry.entryList;
    struct entryNode * prev = NULL;
    while (current != NULL && current != dir) {
        prev = current;
        current = current->next;
    }
    if (prev == NULL) {
        wd->entry.entryList = dir->next;
    } else {
        prev->next = dir->next;
    }
    free(dir->name);
    free(dir);
}

void copyFile(struct entryNode * wd, char * from, char * to) {
    if (located(to, wd->entry.entryList)) {
        printf("cp: %s: File exists\n", to);
        return;
    }
    struct entryNode * file = located(from, wd->entry.entryList);
    if (file == NULL) {
        printf("cp: %s: No such file or directory.\n", from);
        return;
    }

    struct entryNode * newFile = malloc(sizeof(struct entryNode));
    newFile->name = strdup(to);
    newFile->isDirectory = file->isDirectory;
    newFile->parent = wd;
    newFile->next = NULL;

    if (file->isDirectory) {
        newFile->entry.entryList = NULL;
    } else {
        newFile->entry.contents = file->entry.contents ? strdup(file->entry.contents) : NULL;
    }

    insertInOrder(&wd->entry.entryList, newFile);
}

void moveFile(struct entryNode * wd, char * from, char * to) {
    struct entryNode * file = located(from, wd->entry.entryList);
    struct entryNode * toEntry = located(to, wd->entry.entryList);

    if (file == NULL) {
        printf("mv: %s: No such file or directory.\n", from);
        return;
    }

    if (toEntry != NULL && !toEntry->isDirectory) {
        printf("mv: %s: error: File exists\n", to);
        return;
    }

    struct entryNode * prev = NULL, * current = wd->entry.entryList;
    while (current != NULL && current != file) {
        prev = current;
        current = current->next;
    }
    if (prev == NULL) wd->entry.entryList = file->next;
    else prev->next = file->next;

    if (toEntry != NULL && toEntry->isDirectory) {
        if (located(from, toEntry->entry.entryList)) {
            printf("mv: %s: error: File exists in the target directory\n", from);
            insertInOrder(&wd->entry.entryList, file);
            return;
        }
        file->parent = toEntry;
        insertInOrder(&toEntry->entry.entryList, file);
    } else {
        if (located(to, wd->entry.entryList)) {
            printf("mv: %s: error: File with new name already exists\n", to);
            insertInOrder(&wd->entry.entryList, file);
            return;
        }
        free(file->name);
        file->name = strdup(to);
        insertInOrder(&wd->entry.entryList, file);
    }
}

void pwdHelper(struct entryNode * wd);

void printWorkingDir(struct entryNode * wd) {
    if (strcmp(wd->name, "/") == 0) {
        printf("/\n");
    } else {
        pwdHelper(wd);
        printf("\n");
    }
}

void pwdHelper(struct entryNode * wd) {
    if (strcmp(wd->name, "/") != 0) {
        pwdHelper(wd->parent);
        printf("/%s", wd->name);
    }
}

struct entryNode * newWorkingDir(struct entryNode * wd, char * dirName) {
    if (strcmp(dirName, "/") == 0) return root;
    if (strcmp(dirName, "..") == 0) return wd->parent;

    struct entryNode * newWd = located(dirName, wd->entry.entryList);
    if (newWd == NULL) {
        printf("cd: %s: No such file or directory.\n", dirName);
        return wd;
    } else if (!newWd->isDirectory) {
        printf("cd: %s: Not a directory.\n", dirName);
        return wd;
    } else {
        return newWd;
    }
}

void listWorkingDir(struct entryNode * wd) {
    struct entryNode * p = wd->entry.entryList;
    while (p != NULL) {
        printf("%s\n", p->name);
        p = p->next;
    }
}

void listWithinWorkingDir(struct entryNode * wd, char * name) {
    struct entryNode * entryPtr = located(name, wd->entry.entryList);
    if (entryPtr == NULL) {
        printf("ls: %s: No such file or directory\n", name);
    } else if (entryPtr->isDirectory) {
        listWorkingDir(entryPtr);
    } else {
        printf("%s\n", name);
    }
}

void listFileContents(struct entryNode * wd, char * name) {
    struct entryNode * entryPtr = located(name, wd->entry.entryList);
    if (entryPtr == NULL) {
        printf("cat: %s: No such file or directory\n", name);
    } else if (entryPtr->isDirectory) {
        printf("cat: %s: Operation not permitted\n", name);
    } else {
        printf("%s", entryPtr->entry.contents);
    }
}
