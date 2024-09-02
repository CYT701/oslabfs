#include <linux/fs.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/mount.h>
#include <linux/pagemap.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/vmalloc.h>  // 使用 vmalloc
#include "myfs.h"

#define MYFS_MAGIC 0xEF53
#define BLOCK_SIZE 4096   // 每個數據塊大小為 4KB
#define INODE_COUNT 1024  // 文件系統中最多有 1024 個 inode
#define BITMAP_SIZE (INODE_COUNT / 8)  // 簡單假設每個 bitmap 大小
#define DATA_BLOCK_COUNT 1024          // 假設有 1024 個數據塊


static int myfs_fill_super(struct super_block *sb, void *data, int silent)
{
    struct inode *root_inode = NULL;
    // 計算總內存大小，包括 osfs_sb_info, bitmaps, inode table, data blocks
    size_t total_memory_size = sizeof(struct osfs_sb_info) + 2 * BITMAP_SIZE +
                               (INODE_COUNT * sizeof(struct osfs_inode)) +
                               (DATA_BLOCK_COUNT * BLOCK_SIZE);
    void *memory_region = vmalloc(total_memory_size);  // 使用 vmalloc 分配內存
    if (!memory_region)
        return -ENOMEM;

    struct osfs_sb_info *sb_info = (struct osfs_sb_info *) memory_region;
    sb_info->magic = MYFS_MAGIC;
    sb_info->block_size = BLOCK_SIZE;
    sb_info->inode_count = INODE_COUNT;

    // 劃分內存區域
    sb_info->block_bitmap =
        (unsigned long *) (memory_region + sizeof(struct osfs_sb_info));
    sb_info->inode_bitmap =
        (unsigned long *) (sb_info->block_bitmap + BITMAP_SIZE);
    sb_info->inode_table = (void *) (sb_info->inode_bitmap + BITMAP_SIZE);
    sb_info->data_blocks = (void *) (sb_info->inode_table +
                                     (INODE_COUNT * sizeof(struct osfs_inode)));

    // 初始化 bitmaps, inode table 和 data blocks
    memset(sb_info->block_bitmap, 0, BITMAP_SIZE);
    memset(sb_info->inode_bitmap, 0, BITMAP_SIZE);
    memset(sb_info->inode_table, 0, INODE_COUNT * sizeof(struct osfs_inode));
    memset(sb_info->data_blocks, 0, DATA_BLOCK_COUNT * BLOCK_SIZE);

    // 設置 VFS 的 superblock
    sb->s_magic = sb_info->magic;
    sb->s_op = &myfs_super_ops;
    sb->s_fs_info = sb_info;

    // 創建根目錄的 inode
    root_inode = new_inode(sb);
    if (!root_inode) {
        vfree(memory_region);  // 使用 vfree 釋放內存
        return -ENOMEM;
    }
    root_inode->i_ino = 1;
    root_inode->i_sb = sb;
    root_inode->i_op = &simple_dir_inode_operations;
    root_inode->i_fop = &simple_dir_operations;
    root_inode->i_mode = S_IFDIR | 0755;
    inode_init_owner(&init_user_ns, root_inode, NULL, S_IFDIR | 0755);
    sb->s_root = d_make_root(root_inode);
    if (!sb->s_root) {
        iput(root_inode);
        vfree(memory_region);  // 使用 vfree 釋放內存
        return -ENOMEM;
    }

    return 0;
}


// 卸載文件系統時釋放內存
static void myfs_kill_superblock(struct super_block *sb)
{
    void *memory_region = sb->s_fs_info;
    vfree(memory_region);  // 使用 vfree 釋放內存
    kill_litter_super(sb);
}

static struct file_system_type myfs_type = {
    .owner = THIS_MODULE,
    .name = "myfs",
    .mount = myfs_mount,
    .kill_sb = myfs_kill_superblock,
    .fs_flags = FS_USERNS_MOUNT,
};

// 模組初始化
static int __init myfs_init(void)
{
    int ret;

    ret = register_filesystem(&myfs_type);
    if (likely(ret == 0))
        pr_info("myfs: Successfully registered\n");
    else
        pr_err("myfs: Failed to register. Error:[%d]\n", ret);

    return ret;
}

// 模組退出
static void __exit myfs_exit(void)
{
    int ret;

    ret = unregister_filesystem(&myfs_type);
    if (likely(ret == 0))
        pr_info("myfs: Successfully unregistered\n");
    else
        pr_err("myfs: Failed to unregister. Error:[%d]\n", ret);
}

module_init(myfs_init);
module_exit(myfs_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Your Name");
MODULE_DESCRIPTION("A simple memory-based file system kernel module");
