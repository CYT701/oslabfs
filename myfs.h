#ifndef _MYFS_H
#define _MYFS_H

#include <linux/types.h> // 包含基礎類型定義
#include <linux/fs.h>
#include <stdint.h>
#include <linux/bitmap.h>  // 用於位圖操作

// 文件系統的基本參數
#define MYFS_MAGIC 0xEF53
#define BLOCK_SIZE 4096  // 每個數據塊大小為 4KB
#define INODE_COUNT 1024 // 文件系統中最多有 1024 個 inode
#define DATA_BLOCK_COUNT 1024 // 假設有 1024 個數據塊
#define BITMAP_SIZE (INODE_COUNT / 8) // 假設 bitmap 的大小為 128 bytes
#define SIMPLEFS_FILES_PER_BLOCK \
    (BLOCK_SIZE / sizeof(struct simplefs_file))
#define SIMPLEFS_FILENAME_LEN 255

/*directory entry*/
struct osfs_dir_entry {
    char filename[SIMPLEFS_FILENAME_LEN]; 
    uint32_t inode;       
};

/*directory entry in data block*/
struct osfs_dir_block {
    struct osfs_dir_entry files[SIMPLEFS_FILES_PER_BLOCK];
};

/* in-memory superblock structure */
struct osfs_sb_info {
    uint32_t magic;             // 文件系統的魔數，用於標識文件系統
    uint32_t block_size;        // 每個數據塊的大小
    uint32_t inode_count;       // 文件系統中的 inode 總數
    uint32_t nr_free_inodes;    // 空閒的 inode 數量
    unsigned long *inode_bitmap;  // 指向 inode 位圖的指針
    unsigned long *data_bitmap;   // 指向數據塊位圖的指針
    void *inode_table;          // 指向 inode table 的指針
    void *data_blocks;          // 指向數據塊區域的指針
};


struct osfs_inode {
    uint32_t i_ino;          // inode 編號
    uint32_t i_size;         // 文件大小（以字節為單位）
    uint32_t i_blocks;       // 文件佔用的塊數
    uint32_t i_block[12];    // 指向數據塊的指針（直接指針）
    uint16_t i_mode;         // 文件模式（權限和類型，如 S_IFDIR 表示目錄）
    uint16_t i_links_count;  // 連接數（硬鏈接數量）
    uint32_t i_uid;          // 文件所有者的 UID
    uint32_t i_gid;          // 文件所有者的 GID
    uint32_t i_atime;        // 最後訪問時間（Access time）
    uint32_t i_ctime;        // 創建時間（Creation time）
    uint32_t i_mtime;        // 最後修改時間（Modification time）
    uint32_t i_generation;   // inode generation，用於防止重複使用同一個 inode
    void *i_private;         // 私有數據，文件系統專用
    struct osfs_dir_entry *i_dir_entries; // 目錄條目指針
    uint32_t i_dir_entry_count;           // 目錄條目數量
};


// 定義與 inode 操作相關的函數原型
struct inode *get_inode(struct superblock *sb_data, int ino);

#endif /* _MYFS_H */
