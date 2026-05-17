#define _GNU_SOURCE // pthread_rwlock_t is undefined without this macro

#include <unistd.h>
#include <pthread.h>
#include <stdlib.h>
#include <semaphore.h>
#include <fcntl.h> //for O_CREAT flag
#include <string.h>
#include <sys/stat.h>
#include <errno.h>

#include "../../include/safe_file_operation.h"

open_file *open_files; // a pointer to the list of the open files
sem_t *open_files_sem; // a semaphore that is used to protect open_files
                       // list from race conditions when editing it

int safe_files_operations_init(void)
{
    // set open_files to NULL to safely check if the list is empty
    open_files = NULL;

    // initialize the semaphore that would be used in opening or closing files
    if ((open_files_sem = sem_open("SAFE_FILE_OPERATION", O_CREAT, 0666, 0)) == SEM_FAILED)
    {
        return ERROR_INITIALIZING;
    }

    return 0;
}

int safe_open(file_t *file, const char *pathname, int flags, mode_t mode, int file_type)
{
    // check inputs
    if ((pathname == NULL) || ((file_type != NORMAL_FILE) && (file_type != LOG_FILE)))
    {
        return INVALID_INPUT;
    }

    int return_val;
    int existing_file = 1;

    // the st_dev and st_ino members of new_file and
    //  file_stat of open_files will be used to comapre files
    struct stat new_file;

    // get the stat of the new file
    if ((return_val = stat(pathname, &new_file)) == -1)
    {
        if (return_val == ENOENT)
        {
            existing_file = 0;
        }
        else
        {
            return ERROR_ACCESSING_NEW_FILE;
        }
    }

    // block any other thread from opening or deleting
    // any file until this thread is finished
    sem_wait(open_files_sem);

    // search the list to see whether this file is open
    open_file *result;
    if (existing_file)
    {
        result = _does_exist(&new_file);
    }

    if ((result == NULL) || (existing_file == 0))
    { // file isn't open, open it and create a new entry in open_files
        if ((return_val = _open_new_file(file, pathname, flags, mode, file_type)) != 0)
        {
            return return_val;
        }
    }
    else
    { // file is already open by another thread, check it
        // open it and modify the corresponding open_files entry

        // the file type normal/log must match
        if (file_type != result->file_type)
        {
            return ERROR_INCOMPATIBLE_FILE_TYPE;
        }

        if ((return_val = _open_existing_file(file, pathname, flags, mode, file_type, result)) != 0)
        {
            return return_val;
        }
    }

    // release the lock and signal another thread
    sem_post(open_files_sem);

    return 0;
}

int _open_new_file(file_t *file, const char *pathname, int flags,
                   mode_t mode, int file_type)
{
    // no input check is needed since this is checked
    // by the calling funciton: safe_open()

    // make sure that the O_APPEND flag is set if it is
    // a log file, since this is necessary for correct execution
    if (file_type == LOG_FILE)
    {
        flags = flags | O_APPEND;
    }

    // open the new file
    int fd;
    if ((fd = open(pathname, flags, mode)) == -1)
    {
        return ERROR_OPENING_FILE;
    }

    // allocate space for the new entry
    open_file *new_open_file = (open_file *)malloc(sizeof(open_file));

    // initialize new open_file data structure
    new_open_file->file_type = file_type;
    new_open_file->num_of_fds = 1;
    new_open_file->file_fd = fd;

    //get the stat of the new file
    if (fstat(fd, &new_open_file->file_stat) == -1)
    {
        return ERROR_ACCESSING_NEW_FILE;
    }

    // initialize the read/write lock
    if (pthread_rwlock_init(&new_open_file->file_lock, NULL) != 0)
    {
        return ERROR_CREATING_LOCK;
    }

    // add the new open_file to the end of the list
    new_open_file->next = open_files;
    new_open_file->prev = NULL;

    if (open_files != NULL)
    {
        open_files->prev = new_open_file;
    }

    open_files = new_open_file;

    // fill the file_t with the information required
    file->file_fd = fd;
    file->file_info = new_open_file;

    return 0;
}

int _open_existing_file(file_t *file, const char *pathname, int flags, mode_t mode,
                        int file_type, open_file *existing_file)
{
    int return_val;
    if (file_type == NORMAL_FILE)
    {
        if ((return_val = _open_existing_normal_file(file, pathname, flags, mode, existing_file)) != 0)
        {
            return return_val;
        }
    }
    else
    {
        _open_existing_log_file(file, existing_file);
    }

    return 0;
}

int _open_existing_log_file(file_t *file, open_file *existing_file)
{
    // simply take the same file descriptor
    file->file_fd = existing_file->file_fd;
    existing_file->num_of_fds++; // increment the number of open fds to  keep track
                                 // of how many thread is using this file this used to
                                 //  determine when to delete the open_files entry and close the file
    file->file_info = existing_file;

    return 0;
}

int _open_existing_normal_file(file_t *file, const char *pathname, int flags, mode_t mode,
                               open_file *existing_file)
{
    // open the file
    int fd;
    if ((fd = open(pathname, flags, mode)) == -1)
    {
        return ERROR_OPENING_FILE;
    }

    // fill file_t data structure
    file->file_fd = fd;
    existing_file->num_of_fds++; // increment the number of open fds to  keep track
                                 // of how many thread is using this file this used to
                                 //  determine when to delete the open_files entry
    file->file_info = existing_file;

    return 0;
}

open_file *_does_exist(struct stat *new_file)
{
    open_file *current = open_files;

    // check the open files to see if there is a match
    while (current != NULL)
    {
        // check
        if ((current->file_stat.st_dev == new_file->st_dev) &&
            (current->file_stat.st_ino == new_file->st_ino))
        {
            return current;
        }

        current->next;
    }

    return NULL;
}

int safe_close(file_t *file)
{
    if (file == NULL)
    {
        return INVALID_INPUT;
    }

    sem_wait(open_files_sem);

    if (file->file_info->file_type == NORMAL_FILE)
    {
        close(file->file_fd);
    }

    int return_val;
    file->file_info->num_of_fds--;
    if ((return_val = _check_and_delete_entry(file->file_info)) != 0)
    {
        return return_val;
    }

    sem_wait(open_files_sem);

    return 0;
}

inline int _check_and_delete_entry(open_file *file_entry)
{
    // if num_of_fds is zero then no one have this file open
    // so delete it
    if (file_entry->num_of_fds == 0)
    {
        // if it is a log file then close the log file
        if (file_entry->file_type == LOG_FILE)
        {
            close(file_entry->file_fd);
        }

        // destroy the rwlock, as it isn't needed anymore
        if (pthread_rwlock_destroy(&file_entry->file_lock) != 0)
        {
            return ERROR_DESTROYING_LOCK;
        }

        // next remove this entry from open_files list
        // using the same way it is done in a doubly-linked list

        if (file_entry->next != NULL)
        {
            file_entry->next->prev = file_entry->prev;
        }

        if (file_entry->prev == NULL)
        {
            open_files = file_entry->next;
        }
        else
        {
            file_entry->prev->next = file_entry->next;
        }

        // finally free the memory used by this entry
        free(file_entry);
    }

    return 0;
}

ssize_t safe_read(file_t *file, void *buf, size_t count)
{
    if ((file == NULL) || (buf == NULL))
    {
        return INVALID_INPUT;
    }

    pthread_rwlock_rdlock(&file->file_info->file_lock);

    int return_val = read(file->file_fd, buf, count);

    pthread_rwlock_unlock(&file->file_info->file_lock);

    return return_val;
}

ssize_t safe_write(file_t *file, const void *buf, size_t count)
{
    if ((file == NULL) || (buf == NULL))
    {
        return INVALID_INPUT;
    }

    pthread_rwlock_rwlock(&file->file_info->file_lock);

    int return_val = write(file->file_fd, buf, count);

    pthread_rwlock_unlock(&file->file_info->file_lock);

    return return_val;
}
