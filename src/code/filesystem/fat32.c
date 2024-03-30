#include "header/stdlib/string.h"
#include "header/filesystem/fat32.h"
#include <stdint.h>
#include <stdbool.h>
static struct FAT32DriverState fat32_driver_state;


void copyStringWithLength(char* destination, const char* source, size_t length) {
    memcpy(destination, source, length);
    destination[length] = '\0'; // Null-terminate the string
}

const uint8_t fs_signature[BLOCK_SIZE] = {
    'C', 'o', 'u', 'r', 's', 'e', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ',  ' ',
    'D', 'e', 's', 'i', 'g', 'n', 'e', 'd', ' ', 'b', 'y', ' ', ' ', ' ', ' ',  ' ',
    'L', 'a', 'b', ' ', 'S', 'i', 's', 't', 'e', 'r', ' ', 'I', 'T', 'B', ' ',  ' ',
    'M', 'a', 'd', 'e', ' ', 'w', 'i', 't', 'h', ' ', '<', '3', ' ', ' ', ' ',  ' ',
    '-', '-', '-', '-', '-', '-', '-', '-', '-', '-', '-', '2', '0', '2', '4', '\n',
    [BLOCK_SIZE-2] = 'O',
    [BLOCK_SIZE-1] = 'k',
};

uint32_t cluster_to_lba(uint32_t cluster){
    /**
     * uh, ini ada di bagian 1.4.2 Disk addressing LBA dan CHS 
     * jadi cluster itu memang merepresentasikan dari block tapi 
     * semua data akan disimpan dalam kelipatan bilangan bulat dari block
     * sehingga kita perlu nilai asli dari block tadi 
    */
   return cluster * CLUSTER_BLOCK_COUNT;
}

void read_clusters(void *ptr, uint32_t cluster_number, uint8_t cluster_count){
    /**
     * seperti yang kita tahu dari guide book sebuah "cluster" adalah nama lain dari block memory 
     * sehingga cluster adalah sama seperti block sehingga kita bisa menggunakan fungsi yang sudah
     * tersedia pada disk.h yang diimplementasikan pada disk.c yaitu read_block()
     * 
     * param read_block menerima cluster_to_lba(cluster_number) karena kita memakai fungsi read_block 
     * yang notabenenya membaca berdasarkan block-block bukan cluster sehingga harus di-convert
     * 
     * 
    */
    read_blocks(ptr, cluster_to_lba(cluster_number), cluster_count * CLUSTER_BLOCK_COUNT);
}

void write_clusters(const void *ptr, uint32_t cluster_number, uint8_t cluster_count){
    /**
     * penjelasan sama kayak read_cluster 
     * cuman ganti manggil fungs write_blocks() aja
     * 
    */
   write_blocks(ptr, cluster_to_lba(cluster_number), cluster_count * CLUSTER_BLOCK_COUNT);
}

void create_empty_dir_table(struct FAT32DirectoryTable* dir_table, uint32_t current_dir, uint32_t parent_dir){
    /**
     * karena kita melakukan inisiasi pada tabel direktori maka semestinya kita melakukan clearing 
     * caranya dengan melakukan memset pada dir_table dan mengatur semua nilanya menjadi 0 
    */
    memset(dir_table, 0, sizeof(struct FAT32DirectoryTable));
    /**
     * pada bagian ini akan dibuat parent directory 
    */
    struct FAT32DirectoryEntry *parent_entry = &(dir_table->table[0]);
    copyStringWithLength(parent_entry->name, "..", 8);
    parent_entry->attribute = ATTR_SUBDIRECTORY;
    parent_entry->user_attribute = UATTR_NOT_EMPTY;
     /**
     * bagian ini diambil dari parameter parent_dir
     * diatur menjadi uint16_t karena parameternya minta begitu 
     * karena ada low dan high dan parameter dari fungsi ada 16, maka perlu dilakukan operasi shifting 
     * cluster_low langsung pake aja
     * cluster_high perlu dishifting ke kanan sebanyak 16 bit.
    */
    parent_entry->cluster_low = (uint16_t) parent_dir;
    parent_entry->cluster_high = (uint16_t) (parent_dir >> 16);
    /**
     * Pada bagian ini dibuat current directory 
     * umumnya nama dari directory saat "kita" berada pada direktori tersebut adalah "."
    */
    struct FAT32DirectoryEntry *curr_entry = &(dir_table->table[1]);
    copyStringWithLength(curr_entry->name, ".", 8); // nama file dari dir hanya bisa 8 karakter saja
    parent_entry->attribute = ATTR_SUBDIRECTORY;
    parent_entry->user_attribute = UATTR_NOT_EMPTY;
    /**
     * bagian ini diambil dari parameter current_dir 
     * diatur menjadi uint16_t karena parameternya minta begitu 
     * karena ada low dan high dan parameter dari fungsi ada 16, maka perlu dilakukan operasi shifting 
     * cluster_low langsung pake aja
     * cluster_high perlu dishifting ke kanan sebanyak 16 bit.
    */
    parent_entry->cluster_low = (uint16_t) current_dir;
    parent_entry->cluster_high = (uint16_t) (current_dir >> 16); 
}

/**
 * this section using this 
 * @param CLUSTER_0_VALUE = 0x0FFFFFF0
 * @param CLUSTER_1_VALUE = 0x0FFFFFFF
 * @param FAT32_FAT_END_OF_FILE 
 * @param FAT32_FAT_EMPTY_ENTRY
*/
void create_fat32(void){
    write_blocks(fs_signature, 0, 1);
    /**
     * pada Allocation table digunakan linked list? 
     * setiap inisiasi cluster_map atau block dari memori akan di-set empty
     * kecuali 3 cluster_map pertama 
     * 1. digunakan untuk cluster 0
     * 2. digunakan untuk cluster 1 
     * 3. digunakan untuk folder root
     * Lebih lanjut coba baca bagian 1.4.4.2
    */
    struct FAT32FileAllocationTable file_table; 
    for(int i = 0; i < CLUSTER_MAP_SIZE; i++){
        file_table.cluster_map[i] = FAT32_FAT_EMPTY_ENTRY;
    }
    file_table.cluster_map[0] = CLUSTER_0_VALUE; 
    file_table.cluster_map[1] = CLUSTER_1_VALUE;
    file_table.cluster_map[2] = FAT32_FAT_END_OF_FILE;
    /**
     * Bagian setelah ini akan membuat reserved values awal dan 
     * root FAT32DirectoryTable 
     * Chapter pada Bagian kode dibawah: 
     * 1.4.4.3
     * 1.4.4.4
    */
    struct FAT32DirectoryTable dir_table; 
    create_empty_dir_table(&dir_table, ROOT_CLUSTER_NUMBER, ROOT_CLUSTER_NUMBER);
    /**
     * eits, setelah kita bikin harusnya kita buat dia ada isi "awal" dong bukan NULL doang 
    */
    write_clusters(&dir_table, ROOT_CLUSTER_NUMBER, 1);
}

bool is_empty_storage(void){
    struct BlockBuffer b; 
    read_blocks(&b, 0, 1);
    /**
     * pada bagian ini dilakukan pengecekan 
     * jika ada block yang kosong maka akan di-return true 
    */
    for(int i = 0; i < BLOCK_SIZE; i++){
        if(fs_signature[i] != b.buf[i]){
            return true;
        }
    }
    return false;
}

void initialize_filesystem_fat32(void){
    /**
     * bilangnya sih kalo kosong panggil create_fat32();
    */
    if(is_empty_storage()){
        create_fat32();
    }
    /**
     * kalo ga kosong baca, I assume ini make read_cluser
     * karenan create_fat32() dia membuat sebuah cluster 
     * maka untuk baca kita pake read_cluster();
     * yang dibaca itu struct fat32_driver_state
    */
    else{
        /**
         * parameter angka 1 maksudnya adalah cluster yang dibaca hanya 1 dalam satu waktu
         * ada 2 
         * 1. buat baca cluster map 
         * artinya kalo passing fat32_driver_state.fat_table.cluster_map kita nyuruh fungsi 
         * buat baca content dari FAT yang mengandung mapping information cluster
         * 2. buat baca directory(?) 
         * artinya kalo passing fat32_driver_state.dir_table_buf.table kita nyuruh fungsi 
         * buat baca content dari directory table buffer yang mana mengandung 
         * directories entry
        */
        read_clusters(&fat32_driver_state.fat_table.cluster_map, FAT_CLUSTER_NUMBER, 1);
        read_clusters(&fat32_driver_state.dir_table_buf.table, ROOT_CLUSTER_NUMBER, 1);
    }
}