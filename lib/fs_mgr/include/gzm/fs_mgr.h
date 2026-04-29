#ifndef GZM_FS_MGR_H
#define GZM_FS_MGR_H

/* C++ detection */
#ifdef __cplusplus
extern "C" {
#endif

#include <zephyr/kernel.h>
#include <zephyr/fs/fs.h>
#include <stdbool.h>
#include <stddef.h>

#define FS_MGR_MAX_FILE_NAME   MAX_FILE_NAME

enum fs_mgr_mem_info {
	FS_MGR_MEM_SIZE = 0,
	FS_MGR_MEM_FREE_SPACE,
	FS_MGR_MEM_USED_SPACE,
	FS_MGR_MEM_MAX
};

/**
 * @brief Initialize the filesystem.
 *
 * @param fsmout Pointer to the filesystem mount structure.
 * @return 0 on success, negative errno on failure.
 */
int fs_mgr_init(struct fs_mount_t *fsmout);

/**
 * @brief Deinitialize the filesystem.
 *
 * @param fsmout Pointer to the filesystem mount structure.
 * @return 0 on success, negative errno on failure.
 */
int fs_mgr_deinit(struct fs_mount_t *fsmout);

/**
 * @brief Format the filesystem.
 *
 * @param fsmout Pointer to the filesystem mount structure.
 * @return 0 on success, negative errno on failure.
 */
int fs_mgr_format(struct fs_mount_t *fsmout);

/**
 * @brief Search for a file in a directory.
 *
 * @param path Directory path to search.
 * @param fname Buffer to store the found file name.
 * @param fname_len Length of the fname buffer.
 * @param file_size Pointer to store the file size.
 * @param num_files_found Pointer to store the number of files found.
 * @return 0 on success, negative errno on failure.
 */
int fs_mgr_search_file_in_dir(const char *path, char *fname, int fname_len,
			    int *file_size, int *num_files_found);

/**
 * @brief Get the number of files in a directory.
 *
 * @param path Directory path.
 * @param num_files Pointer to store the number of files.
 * @param total_size Pointer to store the total size of all files.
 * @return 0 on success, negative errno on failure.
 */
int fs_mgr_get_num_files_in_dir(const char *path, int *num_files, int *total_size);

/**
 * @brief Save data to a file.
 *
 * @param path Directory path (or NULL for root).
 * @param file_name File name.
 * @param buf Pointer to data buffer.
 * @param buflen Number of bytes to write.
 * @return 0 on success, negative errno on failure.
 */
int fs_mgr_save_file(const char *path, const char *file_name, const void *buf,
		  int buflen);

/**
 * @brief Read data from a file.
 *
 * @param path Directory path (or NULL for root).
 * @param file_name File name.
 * @param buf Pointer to destination buffer.
 * @param buflen Maximum number of bytes to read.
 * @return Number of bytes read on success, negative errno on failure.
 */
int fs_mgr_read_file(const char *path, const char *file_name, void *buf,
		  int buflen);

/**
 * @brief Delete a file.
 *
 * @param path Directory path (or NULL for root).
 * @param file_name File name.
 * @return 0 on success, negative errno on failure.
 */
int fs_mgr_delete_file(const char *path, const char *file_name);

/**
 * @brief Delete all files in a directory.
 *
 * @param path Directory path.
 * @return 0 on success, negative errno on failure.
 */
int fs_mgr_delete_files_of_path(const char *path);

/**
 * @brief Get the size of a file.
 *
 * @param path Directory path (or NULL for root).
 * @param file_name File name.
 * @param file_size Pointer to store the file size.
 * @return 0 on success, negative errno on failure.
 */
int fs_mgr_get_file_size(const char *path, const char *file_name, int *file_size);

/**
 * @brief Release space in a directory by deleting oldest files.
 *
 * @param path Directory path.
 * @param size Minimum number of bytes to release.
 * @return 0 on success, negative errno on failure.
 */
int fs_mgr_release_space_in_path(const char *path, int size);

/**
 * @brief Create a directory if it does not exist.
 *
 * @param path Directory path.
 * @return 0 on success, negative errno on failure.
 */
int fs_mgr_mkdir_if_not_exists(const char *path);

/**
 * @brief Get filesystem memory information.
 *
 * @param fsmout Pointer to the filesystem mount structure.
 * @param ch Memory info channel to query.
 * @param val Pointer to store the result in bytes.
 * @return 0 on success, negative errno on failure.
 */
int fs_mgr_mem_info(struct fs_mount_t *fsmout, enum fs_mgr_mem_info ch, float *val);

/**
 * @brief File entry returned by fs_mgr_list_files.
 */
struct fs_mgr_file_entry {
	char name[32];
	int  size;
};

/**
 * @brief List files in a directory (single mutex lock).
 *
 * @param path        Directory path.
 * @param entries     Array to fill with file info.
 * @param max_entries Maximum entries to return.
 * @return Number of entries filled, or negative error.
 */
int fs_mgr_list_files(const char *path, struct fs_mgr_file_entry *entries,
		   int max_entries);

/* C++ detection */
#ifdef __cplusplus
}
#endif

#endif /* GZM_FS_MGR_H */
