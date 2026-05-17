/*  usage example:

safe_files_operations_init();

//some code...

file_t new_file;
if(safe_open(&new_file, "file.txt", O_CREAT | O_WRONLY, 0666, NORMAL_FILE) != 0)
{
    //error handling if needed ....
}

//some other code ...

int count = safe_write(&new_file, buffer, buff_size);

.
.
.

safe_close(&new_file);

*/


#ifndef SAFE_FILE_OPERATION_H
#define SAFE_FILE_OPERATION_H

#define ERROR_INITIALIZING 1
#define ERROR_ACCESSING_NEW_FILE 2
#define INVALID_INPUT 3
#define ERROR_INCOMPATIBLE_FILE_TYPE 4
#define ERROR_OPENING_FILE 5
#define ERROR_CREATING_LOCK 6
#define ERROR_DESTROYING_LOCK 7

#define NORMAL_FILE 0
#define LOG_FILE 1

typedef struct open_file
{
    struct stat file_stat;
    pthread_rwlock_t file_lock;
    int num_of_fds;
    int file_fd; // have the fd in case file_type is LOG_FILE
                 // otherwise it is the first fd that opened that file
    int file_type;
    struct open_file *next;
    struct open_file *prev;
} open_file;

typedef struct file_t
{
    int file_fd;
    open_file *file_info;
} file_t;



//==============================================================//
// These functions are for the user to use                      //
//==============================================================//
//                       USER FUNCTIONS                         //
//==============================================================//

/**
 * @brief initializes the global data structures
 *        that is used to implement safe file operations
 * @return 0: everything is okay,
 *         ERROR_INITIALIZING: something wrong happening when
 *         obtaining the semaphore, see @p errno
 */
int safe_files_operations_init(void);

/**
 * @brief safely open a file
 * @param file the file_t to be initialized and used to identify the file afterwards
 *             note that it is considered a replacement for fd
 * @param pathname the path to the file to be opened
 * @param flags the flags used to open the file ORed togather, some of which are:
 *              O_RDONLY: open the file as read only
 *              O_WRONLY: open the file as write only
 *              O_RDWR: open the file as read/write
 *              O_APPEND: open the file in append mode
 *              O_CREAT: if the file doesn't exist, then create it
 * @param mode specifies the permissions of the newly created file,
 *             must be specified when O_CREAT is used,
 *             there are many flags but for convenience set it to 0666
 * @param file_type define the file type, could be:
 *                  LOG_FILE: if the file would be used as log file
 *                  NORMAL_FILE: used with any other file that isn't a log file
 * @return 0: everything is okay,
 *         INVALID_INPUT: some input is invalid
 *         ERROR_ACCESSING_NEW_FILE: something wrong happend when calling stat(), check errno
 *         ERROR_INCOMPATIBLE_FILE_TYPE: The file is already open but with another file type
 *         ERROR_OPENING_FILE: opening the file has failed, check errno
 *         ERROR_CREATING_LOCK: error creating a new lock
 */
int safe_open(file_t *file, const char *pathname, int flags, mode_t mode, int file_type);

/**
 * @brief safely close an open file
 * @param file the file_t that is used to identify the open file
 * @return 0: everything is okay,
 *         INVALID_INPUT: file is invalid
 *         ERROR_DESTROYING_LOCK: destrying the rwlock has failed
 */
int safe_close(file_t *file);

/**
 * @brief safely read from a file without race conditions
 * @param file the file_t that is used to identify the open file
 * @param buf the buffer that the data will be read into
 * @param count the maximum number of bytes read
 * @return INVALID_INPUT: either file or buf is invalid
 *         other values: the returned values is handled as with
 *         the normal read() syscall
 */
ssize_t safe_read(file_t *file, void *buf, size_t count);

/**
 * @brief safely write to a file without race conditions
 * @param file the file_t that is used to identify the open file
 * @param buf the buffer that the data will be read from
 * @param count the maximum number of bytes written to the buffer
 * @return INVALID_INPUT: either file or buf is invalid
 *         other values: the returned values is handled as with
 *         the normal write() syscall
 */
ssize_t safe_write(file_t *file, const void *buf, size_t count);


//==========================================================================//
// These functions are helper function and should not be used by the user   //
//==========================================================================//
//                           HELPER FUNCTIONS                               //
//==========================================================================//

/**
 * @brief a helper function to open a new file and create a new entry in open_files
 * @param file the file_t to be initialized and used to identify the file afterwards
 *             note that it is considered a replacement for fd
 * @param pathname the path to the file to be opened
 * @param flags the flags used to open the file ORed togather, some of which are:
 *              O_RDONLY: open the file as read only
 *              O_WRONLY: open the file as write only
 *              O_RDWR: open the file as read/write
 *              O_APPEND: open the file in append mode
 *              O_CREAT: if the file doesn't exist, then create it
 * @param mode specifies the permissions of the newly created file,
 *             must be specified when O_CREAT is used,
 *             there are many flags but for convenience set it to 0666
 * @param file_type define the file type, could be:
 *                  LOG_FILE: if the file would be used as log file
 *                  NORMAL_FILE: used with any other file that isn't a log file
 * @return 0: everything is okay,
 *         ERROR_ACCESSING_NEW_FILE: something wrong happend when calling stat(), check errno
 *         ERROR_OPENING_FILE: opening the file has failed, check errno
 *         ERROR_CREATING_LOCK: error creating a new lock
 */
int _open_new_file(file_t *file, const char *pathname, int flags,
                   mode_t mode, int file_type);

/**
 * @brief a helper function to open an existing file and have pointer
 *        an entry in the open_files list
 * @param file the file_t to be initialized and used to identify the file afterwards
 *             note that it is considered a replacement for fd
 * @param pathname the path to the file to be opened
 * @param flags the flags used to open the file ORed togather, some of which are:
 *              O_RDONLY: open the file as read only
 *              O_WRONLY: open the file as write only
 *              O_RDWR: open the file as read/write
 *              O_APPEND: open the file in append mode
 *              O_CREAT: if the file doesn't exist, then create it
 * @param mode specifies the permissions of the newly created file,
 *             must be specified when O_CREAT is used,
 *             there are many flags but for convenience set it to 0666
 * @param file_type define the file type, could be:
 *                  LOG_FILE: if the file would be used as log file
 *                  NORMAL_FILE: used with any other file that isn't a log file
 * @param existing_file pointer to the entry of open_files that contain the file
 * @return 0: everything is okay,
 *         ERROR_OPENING_FILE: opening the file has failed, check errno
 */
int _open_existing_file(file_t *file, const char *pathname, int flags, mode_t mode,
                        int file_type, open_file *existing_file);

 /**
 * @brief a helper function to open an existing  log file and have pointer
 *        an entry in the open_files list
 * @param file the file_t to be initialized and used to identify the file afterwards
 *             note that it is considered a replacement for fd
 * @param existing_file pointer to the entry of open_files that contain the file
 * @return 0: everything is okay,
 * @details this function doesn't create a new fd by opening the file by calling open
 *          instead it uses the same fd  
 */
int _open_existing_log_file(file_t *file, open_file *existing_file);


/**
 * @brief a helper function to open an existing normal file and have pointer
 *        an entry in the open_files list
 * @param file the file_t to be initialized and used to identify the file afterwards
 *             note that it is considered a replacement for fd
 * @param pathname the path to the file to be opened
 * @param flags the flags used to open the file ORed togather, some of which are:
 *              O_RDONLY: open the file as read only
 *              O_WRONLY: open the file as write only
 *              O_RDWR: open the file as read/write
 *              O_APPEND: open the file in append mode
 *              O_CREAT: if the file doesn't exist, then create it
 * @param mode specifies the permissions of the newly created file,
 *             must be specified when O_CREAT is used,
 *             there are many flags but for convenience set it to 0666
 * @param existing_file pointer to the entry of open_files that contain the file
 * @return 0: everything is okay,
 *         ERROR_OPENING_FILE: opening the file has failed, check errno
 */
int _open_existing_normal_file(file_t *file, const char *pathname, int flags, mode_t mode,
                               open_file *existing_file);

/**
 * @brief a helper function to check whether a file whose
 *        stat data structure is new_file is open by another thread
 * @param new_file the stat data structure whose st_dev and st_ino
 *                 are compared against each entry of open_files to 
 *                 check whether the file is open or not
 * @return a pointer: to the open_file entry of open_files list 
 *         of which has the same st_dev and st_ino as new_file
 *         NULL: no such entry exist
 */
open_file *_does_exist(struct stat *new_file);


/**
 * @brief a helper function that is called when a file is closed,
 *        used to check whether the the file_entry needs to be 
 *        deleted from open_files list, and if so deletes it
 * @param file_entry the entry which will be checked if it requires
 *                   to be deleted
 * @return 0: everything is okay
 *         ERROR_DESTROYING_LOCK: failed to destroy the rwlock
 */
inline int _check_and_delete_entry(open_file *file_entry);

#endif
