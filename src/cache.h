#ifndef CACHE_H
#define CACHE_H

#include "link.h"

#include <pthread.h>

/**
 * \file cache.h
 * \brief cache related structures and functions
 * \details
 *   - We store the metadata and the actual data separately in two
 * separate folders.
 */

/**
 * \brief Type definition for a cache segment
 */
typedef uint8_t Seg;

/**
 * \brief cache in-memory data structure
 */
typedef struct {
    char *path; /**< the path to the file on the web server */
    Link *link; /**< the Link associated with this cache data set */
    long time; /**<the modified time of the file */
    off_t content_length; /**<the size of the file */

    pthread_t bgt; /**< background download pthread */
    pthread_mutex_t bgt_lock; /**< mutex for the background download thread */
    pthread_mutexattr_t bgt_lock_attr; /**< attributes for bgt_lock */
    off_t next_offset; /**<the offset of the next segment to be
    downloaded in background*/

    pthread_mutex_t rw_lock; /**< mutex for read/write operation */
    pthread_mutexattr_t rw_lock_attr; /**< attributes for rw_lock */

    FILE *dfp; /**< The FILE pointer for the data file*/
    FILE *mfp; /**< The FILE pointer for the metadata */
    int blksz; /**<the block size of the data file */
    long segbc; /**<segment array byte count */
    Seg *seg; /**< the detail of each segment */
} Cache;

/**
 * \brief whether the cache system is enabled
 */
extern int CACHE_SYSTEM_INIT;

/**
 * \brief The size of each download segment
 */
extern int DATA_BLK_SZ;

/**
 * \brief The maximum segment count for a single cache file
 */
extern int MAX_SEGBC;

/**
 * \brief initialise the cache system directories
 * \details This function basically sets up the following variables:
 *  - META_DIR
 *  - DATA_DIR
 *
 * If these directories do not exist, they will be created.
 * \note Called by parse_arg_list(), verified to be working
 */
void CacheSystem_init(const char *path, int path_supplied);

/**
 * \brief Create directories under the cache directory structure, if they do
 * not already exist
 * \return
 *  -   -1 failed to create metadata directory.
 *  -   -2 failed to create data directory.
 *  -   -3 failed to create both metadata and data directory.
 * \note Called by LinkTable_new()
 */
int CacheDir_create(const char *fn);

/**
 * \brief open a cache file set
 * \note This function is called by fs_open()
 */
Cache *Cache_open(const char *fn);

/**
 * \brief Close a cache data structure
 * \note This function is called by fs_release()
 */
void Cache_close(Cache *cf);

/**
 * \brief create a cache file set if it doesn't exist already
 * \return
 *  -   0, if the cache file already exists, or was created succesfully.
 *  -   -1, otherwise
 * \note Called by fs_open()
 */
int Cache_create(Link *this_link);

/**
 * \brief delete a cache file set
 * \note Called by fs_open()
 */
void Cache_delete(const char *fn);

/**
 * \brief Intelligently read from the cache system
 * \details If the segment does not exist on the local hard disk, download from
 * the Internet
 * \param[in] cf the cache in-memory data structure
 * \param[out] output_buf the output buffer
 * \param[in] len the requested segment size
 * \param[in] offset the start of the segment
 * \return the length of the segment the cache system managed to obtain.
 * \note Called by fs_read(), verified to be working
 */
long Cache_read(Cache *cf, char *output_buf, off_t len, off_t offset);

#endif
