#include <linux/errno.h>  // 標準錯誤代碼定義
#include <linux/fs.h>  // 基本的文件系統結構（如 inode, dentry, super_block）
#include <linux/slab.h>    // 動態內存分配函數（如 kmalloc, kzalloc）
#include <linux/string.h>  // 標準字符串操作（如 memset）
#include <linux/time.h>    // 獲取和設置時間的相關函數
#include "myfs.h"

struct osfs_inode *osfs_get_inode(struct osfs_sb_info *sbi, int ino)
{
    if (ino < 0 || ino >= sbi->inode_count) {
        pr_err("get_inode: Invalid inode number %d\n", ino);
        return NULL;
    }
    // calculate the inode pointer using inode number
    struct osfs_inode *inode_ptr =
        (struct osfs_inode *) ((char *) sbi->inode_table +
                               ino * sizeof(struct osfs_inode));
    return inode_ptr;
}



static uint32_t get_free_inode(struct osfs_sb_info *sbi)
{
    uint32_t ino;
    // 遍歷 inode bitmap，找到一個空閒的 inode
    for (ino = 0; ino < sbi->inode_count; ino++) {
        if (!test_bit(ino, sbi->inode_bitmap)) {
            // 標記該 inode 為已使用
            set_bit(ino, sbi->inode_bitmap);
            sbi->nr_free_inodes--;  // 減少空閒 inode 計數
            return ino;
        }
    }
    return 0;  // 沒有可用的 inode
}


static int osfs_create(struct inode *dir,
                       struct dentry *dentry,
                       umode_t mode,
                       bool excl)
{
    struct inode *inode;
    struct osfs_inode *osfs_inode;
    struct super_block *sb = dir->i_sb;
    struct osfs_sb_info *sbi = sb->s_fs_info;
    struct osfs_inode *parent_inode = (struct osfs_inode *) dir->i_private;
    uint32_t ino;

    /*check whether the file already exists*/

    for (int i = 0; i < parent_inode->i_dir_entry_count; i++) {
        if (strcmp(parent_inode->i_dir_entries[i].name, dentry->d_name.name) ==
            0) {
            return -EEXIST;
        }
    }

    /*check file name length*/
    if (strlen(dentry->d_name.name) > SIMPLEFS_FILENAME_LEN)
        return -ENAMETOOLONG;

    // find a free inode number from inode bitmap
    ino = get_free_inode(sbi);
    if (ino == 0) {
        return -ENOSPC;
    }

    // allocate and new inode structure
    inode = new_inode(sb);
    if (!inode) {
        return -ENOMEM;
    }

    // init inode attribute
    inode->i_ino = ino;
    inode->i_mode = mode | S_IFREG; /*setting inode flag as file*/
    inode->i_atime = inode->i_mtime = inode->i_ctime = current_time(inode);
    inode->i_op = &osfs_file_inode_operations;
    inode->i_fop = &osfs_file_operations;

    // 初始化文件系統特定的 inode 資訊
    osfs_inode = osfs_get_inode(sbi, ino);
    memset(osfs_inode, 0, sizeof(*osfs_inode));

    osfs_inode->i_ino = ino;
    osfs_inode->i_mode = mode | S_IFREG;
    osfs_inode->i_size = 0;
    osfs_inode->i_blocks = 0;

    // 將新 inode 與 dentry 關聯
    d_instantiate(dentry, inode);

    // 更新父目錄的修改時間
    dir->i_mtime = dir->i_ctime = current_time(dir);


    return 0;
}


static const struct inode_operations osfs_dir_inode_operations = {
    .create = osfs_create,
};