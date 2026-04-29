#include <gzm/fs_mgr.h>

#include <string.h>
#include <stdio.h>

#include <zephyr/device.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(fs_mgr, LOG_LEVEL_DBG);

#define XFS_WRITE_CHUNK_SIZE 512
#define XFS_MUTEX_TIMEOUT_MS 5000
#define XFS_FILE_SIZE       (FS_MGR_MAX_FILE_NAME + 10)

static int fs_mgr_lock(void);
static void fs_mgr_unlock(void);
static int fs_mgr_mount_storage(struct fs_mount_t *fsmout);

static struct k_sem g_xfs_mutex;
static bool g_xfs_initialized = false;

#define FS_RET_OK 0

static int fs_mgr_lock(void)
{
    return k_sem_take(&g_xfs_mutex, K_MSEC(XFS_MUTEX_TIMEOUT_MS));
}

static void fs_mgr_unlock(void)
{
    k_sem_give(&g_xfs_mutex);
}

static int fs_mgr_mount_storage(struct fs_mount_t *fsmout)
{
    int res;

    res = fs_mount(fsmout);

    if (res == FS_RET_OK) {
        LOG_INF("Disk mounted at %s", fsmout->mnt_point);
        /* Unmount and remount to ensure clean state */
        res = fs_unmount(fsmout);
        if (res != FS_RET_OK) {
            LOG_ERR("Error unmounting disk");
            return res;
        }
        res = fs_mount(fsmout);
        if (res != FS_RET_OK) {
            LOG_ERR("Error remounting disk");
            return res;
        }
    } else {
        LOG_ERR("Error mounting disk (%d)", res);
    }

    return res;
}

int fs_mgr_init(struct fs_mount_t *fsmout)
{
    int ret;

    LOG_INF("Initializing filesystem...");

    k_sem_init(&g_xfs_mutex, 1, 1);

    ret = fs_mgr_mount_storage(fsmout);
    if (ret != 0) {
        LOG_WRN("Mount failed (%d), attempting format...", ret);
        ret = fs_mgr_format(fsmout);
        if (ret != 0) {
            LOG_ERR("Format + mount failed (%d)", ret);
            return ret;
        }
    }

    g_xfs_initialized = true;
    LOG_INF("Filesystem initialized successfully");
    return 0;
}

int fs_mgr_deinit(struct fs_mount_t *fsmout)
{
    int ret;

    ret = fs_mgr_lock();
    if (ret) {
        return ret;
    }

    ret = fs_unmount(fsmout);
    if (ret != 0) {
        LOG_ERR("Failed to unmount filesystem (%d)", ret);
        fs_mgr_unlock();
        return ret;
    }

    g_xfs_initialized = false;
    fs_mgr_unlock();
    LOG_INF("Filesystem unmounted successfully");
    return 0;
}

int fs_mgr_format(struct fs_mount_t *fsmout)
{
    int ret;

    ret = fs_mgr_lock();
    if (ret) {
        return ret;
    }

    LOG_INF("Formatting %s (type %d)...", fsmout->mnt_point, fsmout->type);

    /* Unmount if currently mounted */
    fs_unmount(fsmout);

#if defined(CONFIG_FILE_SYSTEM_MKFS)
    /* Use fsmout->type to select the correct filesystem at runtime.
     * This allows applications that use both FATFS (SD card) and
     * LittleFS (flash) simultaneously to format either one.
     */
    /* FatFs f_mkfs expects drive path without leading '/' (e.g. "SD:") */
    const char *drv = fsmout->mnt_point;
    if (drv[0] == '/') {
        drv++;
    }
    ret = fs_mkfs(fsmout->type, (uintptr_t)drv, NULL, 0);
    if (ret != 0) {
        LOG_ERR("Format failed (%d)", ret);
        fs_mgr_unlock();
        return ret;
    }
    LOG_INF("Filesystem formatted successfully");
#else
    LOG_WRN("fs_mkfs not available (CONFIG_FILE_SYSTEM_MKFS=n)");
    ret = -ENOTSUP;
    fs_mgr_unlock();
    return ret;
#endif

    /* Remount the filesystem */
    ret = fs_mount(fsmout);
    if (ret != 0) {
        LOG_ERR("Failed to mount after format (%d)", ret);
        fs_mgr_unlock();
        return ret;
    }

    fs_mgr_unlock();
    LOG_INF("Filesystem mounted after format");
    return 0;
}

int fs_mgr_search_file_in_dir(const char *path, char *fname, int fname_len, int *file_size, int *num_files_found)
{
    int result;
    struct fs_dir_t dir;
    struct fs_dirent entry;

    result = fs_mgr_lock();
    if (result) {
        return result;
    }

    *file_size = 0;
    *num_files_found = 0;

    fs_dir_t_init(&dir);
    result = fs_opendir(&dir, path);
    if (result) {
        LOG_ERR("Unable to open %s (err %d)", path, result);
        fs_mgr_unlock();
        return -ENOEXEC;
    }

    while (1) {
        result = fs_readdir(&dir, &entry);
        if (result) {
            LOG_ERR("Unable to read directory");
            break;
        } else if (entry.name[0] == '\0') {
            /* End of directory listing */
            break;
        } else if (entry.type == FS_DIR_ENTRY_FILE) {
            if ((*file_size) == 0) {
                *file_size = entry.size;
                snprintf(fname, fname_len, "%s", entry.name);
            }
            (*num_files_found)++;
        }
    }

    fs_closedir(&dir);

    if (*num_files_found == 0) {
        result = -ENOENT;
    }

    fs_mgr_unlock();
    return result;
}

int fs_mgr_get_num_files_in_dir(const char *path, int *num_files, int *total_size)
{
    int result;
    struct fs_dir_t dir;
    struct fs_dirent entry;

    *num_files = 0;
    *total_size = 0;

    result = fs_mgr_lock();
    if (result) {
        return result;
    }

    fs_dir_t_init(&dir);
    result = fs_opendir(&dir, path);
    if (result) {
        LOG_ERR("Unable to open %s (err %d)", path, result);
        fs_mgr_unlock();
        return -ENOEXEC;
    }

    while (1) {
        result = fs_readdir(&dir, &entry);
        if (result) {
            LOG_ERR("Unable to read directory");
            break;
        } else if (entry.name[0] == '\0') {
            /* End of directory listing */
            break;
        } else if (entry.type == FS_DIR_ENTRY_FILE) {
            (*num_files)++;
            (*total_size) += entry.size;
        }
    }

    fs_closedir(&dir);
    fs_mgr_unlock();

    return result;
}

int fs_mgr_save_file(const char *path, const char *file_name, const void *buf, int buflen)
{
    int result;
    struct fs_file_t file;
    char fname[XFS_FILE_SIZE];
    const uint8_t *data = (const uint8_t *)buf;
    int remaining = buflen;
    int written_total = 0;

    fs_file_t_init(&file);
    
    result = fs_mgr_lock();
    if (result < 0) {
        return result;
    }

    if (path) {
        snprintf(fname, sizeof(fname), "%s/%s", path, file_name);
    } else {
        snprintf(fname, sizeof(fname), "%s", file_name);
    }

    result = fs_open(&file, fname, FS_O_CREATE | FS_O_WRITE | FS_O_TRUNC);
    if (result < 0) {
        LOG_ERR("Open %s file error: %d", fname, result);
        fs_mgr_unlock();
        return result;
    }

    /* Write in chunks to avoid timeout on slow SD cards */
    while (remaining > 0) {
        int chunk_size = (remaining > XFS_WRITE_CHUNK_SIZE) ? XFS_WRITE_CHUNK_SIZE : remaining;

        result = fs_write(&file, data + written_total, chunk_size);
        if (result < 0) {
            LOG_ERR("Write chunk error at offset %d: %d", written_total, result);
            fs_close(&file);
            fs_unlink(fname);
            fs_mgr_unlock();
            return -EIO;
        }

        if (result != chunk_size) {
            LOG_ERR("Partial write: %d of %d at offset %d", result, chunk_size, written_total);
            fs_close(&file);
            fs_unlink(fname);
            fs_mgr_unlock();
            return -EIO;
        }

        written_total += result;
        remaining -= result;
    }

    fs_close(&file);

    if (written_total != buflen) {
        fs_unlink(fname);
        LOG_ERR("Write %s file error: wrote %d of %d", fname, written_total, buflen);
        result = -EIO;
    } else {
        LOG_INF("File saved: %s (%d bytes)", fname, buflen);
        result = 0;
    }

    fs_mgr_unlock();
    return result;
}

int fs_mgr_read_file(const char *path, const char *file_name, void *buf, int buflen)
{
    int result;
    struct fs_file_t file;
    char fname[XFS_FILE_SIZE];

    fs_file_t_init(&file);

    result = fs_mgr_lock();
    if (result < 0) {
        return result;
    }

    memset(fname, 0, sizeof(fname));

    if (path) {
        snprintf(fname, sizeof(fname), "%s/%s", path, file_name);
    } else {
        snprintf(fname, sizeof(fname), "%s", file_name);
    }

    result = fs_open(&file, fname, FS_O_READ);
    if (result < 0) {
        LOG_ERR("Open %s file error: %d", fname, result);
        fs_mgr_unlock();
        return result;
    }

    result = fs_read(&file, buf, buflen);
    fs_close(&file);

    fs_mgr_unlock();

    /* Return bytes read */
    return result;
}

int fs_mgr_delete_file(const char *path, const char *file_name)
{
    int result;
    char fname[XFS_FILE_SIZE];

    result = fs_mgr_lock();
    if (result < 0) {
        return result;
    }

    if (path) {
        snprintf(fname, sizeof(fname), "%s/%s", path, file_name);
    } else {
        snprintf(fname, sizeof(fname), "%s", file_name);
    }

    result = fs_unlink(fname);

    if (result) {
        LOG_ERR("Erasing %s file error: %d", fname, result);
    } else {
        LOG_INF("File deleted: %s", fname);
    }

    fs_mgr_unlock();
    return result;
}

int fs_mgr_get_file_size(const char *path, const char *file_name, int *file_size)
{
    int result;
    char fname[XFS_FILE_SIZE];
    struct fs_dirent dirent;

    result = fs_mgr_lock();
    if (result < 0) {
        return result;
    }

    if (path) {
        snprintf(fname, sizeof(fname), "%s/%s", path, file_name);
    } else {
        snprintf(fname, sizeof(fname), "%s", file_name);
    }

    result = fs_stat(fname, &dirent);
    if (!result) {
        *file_size = dirent.size;
    }

    fs_mgr_unlock();
    return result;
}

int fs_mgr_release_space_in_path(const char *path, int size)
{
    int result;
    struct fs_dir_t dir;
    struct fs_dirent entry;
    char fname[XFS_FILE_SIZE];
    int tot_size = 0;
    int tot_files_deleted = 0;

    if (size <= 0) {
        return -EINVAL;
    }

    result = fs_mgr_lock();
    if (result) {
        return result;
    }

    fs_dir_t_init(&dir);
    result = fs_opendir(&dir, path);
    if (result) {
        LOG_ERR("Unable to open %s (err %d)", path, result);
        fs_mgr_unlock();
        return -ENOEXEC;
    }

    while (1) {
        result = fs_readdir(&dir, &entry);
        if (result) {
            LOG_ERR("Unable to read directory");
            break;
        } else if (entry.name[0] == '\0') {
            /* End of directory listing */
            break;
        } else if (entry.type == FS_DIR_ENTRY_FILE) {
            tot_files_deleted++;
            tot_size += entry.size;
            snprintf(fname, sizeof(fname), "%s/%s", path, entry.name);
            result = fs_unlink(fname);
            LOG_INF("Delete file %s - size: %d - err: %d", fname, entry.size, result);
            if (tot_size >= size) {
                break;
            }
        }
    }

    LOG_INF("Total files deleted: %d - released size: %d bytes", tot_files_deleted, tot_size);
    fs_closedir(&dir);

    fs_mgr_unlock();
    return result;
}

int fs_mgr_delete_files_of_path(const char *path)
{
    int result;
    struct fs_dir_t dir;
    struct fs_dirent entry;
    char fname[XFS_FILE_SIZE];

    result = fs_mgr_lock();
    if (result) {
        return result;
    }

    fs_dir_t_init(&dir);
    result = fs_opendir(&dir, path);
    if (result) {
        LOG_ERR("Unable to open %s (err %d)", path, result);
        fs_mgr_unlock();
        return -ENOEXEC;
    }

    while (1) {
        result = fs_readdir(&dir, &entry);
        if (result) {
            LOG_ERR("Unable to read directory");
            break;
        } else if (entry.name[0] == '\0') {
            /* End of directory listing */
            break;
        } else if (entry.type == FS_DIR_ENTRY_FILE) {
            snprintf(fname, sizeof(fname), "%s/%s", path, entry.name);
            result = fs_unlink(fname);
            LOG_DBG("Deleted: %s (err %d)", fname, result);
        }
    }

    fs_closedir(&dir);
    fs_mgr_unlock();

    return result;
}

int fs_mgr_mkdir_if_not_exists(const char *path)
{
    struct fs_dirent entry;
    int err;

    err = fs_stat(path, &entry);
    if (err == -ENOENT) {
        return fs_mkdir(path);
    } else if (err) {
        return err;
    }

    if (entry.type != FS_DIR_ENTRY_DIR) {
        return -EEXIST;
    }

    return 0;
}

int fs_mgr_mem_info(struct fs_mount_t *fsmout, enum fs_mgr_mem_info ch, float *val)
{
    int result;
    struct fs_statvfs stat;
    long long space = 0;

    if (!val) {
        return -EINVAL;
    }

    result = fs_statvfs(fsmout->mnt_point, &stat);
    if (result) {
        return result;
    }

    switch (ch) {
    case FS_MGR_MEM_SIZE:
        /* block size x total num blocks */
        space = (long long)stat.f_blocks * stat.f_frsize;
        break;

    case FS_MGR_MEM_FREE_SPACE:
        /* block size x num free blocks */
        space = (long long)stat.f_bfree * stat.f_frsize;
        break;

    case FS_MGR_MEM_USED_SPACE:
        /* block size x total num blocks */
        space = (long long)stat.f_blocks * stat.f_frsize;
        /* block size x num free blocks */
        space -= ((long long)stat.f_bfree * stat.f_frsize);
        break;

    default:
        return -ENOTSUP;
    }

    *val = (float)(space);
    return 0;
}

int fs_mgr_list_files(const char *path, struct fs_mgr_file_entry *entries, int max_entries)
{
    int result;
    struct fs_dir_t dir;
    struct fs_dirent dirent;
    int count = 0;

    if (!entries || max_entries <= 0) {
        return -EINVAL;
    }

    result = fs_mgr_lock();
    if (result) {
        return result;
    }

    fs_dir_t_init(&dir);
    result = fs_opendir(&dir, path);
    if (result) {
        LOG_ERR("Unable to open %s (err %d)", path, result);
        fs_mgr_unlock();
        return result;
    }

    while (count < max_entries) {
        result = fs_readdir(&dir, &dirent);
        if (result || dirent.name[0] == '\0') {
            break;
        }

        if (dirent.type == FS_DIR_ENTRY_FILE) {
            strncpy(entries[count].name, dirent.name,
                    sizeof(entries[count].name) - 1);
            entries[count].name[sizeof(entries[count].name) - 1] = '\0';
            entries[count].size = dirent.size;
            count++;
        }
    }

    fs_closedir(&dir);
    fs_mgr_unlock();

    return count;
}
