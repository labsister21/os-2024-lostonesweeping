#include "header/filesystem/fat32.h"
#include "header/stdlib/string.h"
#include "header/text/framebuffer.h"
#include <stdint.h>
#include <stdbool.h>

static struct FAT32DriverState fat32_driver_state = {0};


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

void init_directory_table(struct FAT32DirectoryTable *dir_table, char *name, uint32_t parent_dir_cluster) {
    // Calculate the size of a directory entry
    memset(dir_table, 0x00, CLUSTER_SIZE);
    // Initialize self entry
    struct FAT32DirectoryEntry *self_entry = &(dir_table->table[0]);
    copyStringWithLength(self_entry->name, name, 8); // Copy name (up to 8 characters)
    self_entry->attribute = ATTR_SUBDIRECTORY;
    self_entry->user_attribute = UATTR_NOT_EMPTY;
    self_entry->cluster_low = (uint16_t)parent_dir_cluster;
    self_entry->cluster_high = (uint16_t)(parent_dir_cluster >> 16);
    self_entry->filesize = 0;

    struct FAT32DirectoryEntry *parent_entry = &(dir_table->table[1]);
    copyStringWithLength(parent_entry->name, "..", 8); 
    parent_entry->attribute = ATTR_SUBDIRECTORY;
    parent_entry->user_attribute = UATTR_NOT_EMPTY;
    parent_entry->cluster_low = (uint16_t)parent_dir_cluster; // Set to current directory cluster
    parent_entry->cluster_high = (uint16_t)(parent_dir_cluster >> 16); // For FAT32, consider higher bits of current cluster
    parent_entry->filesize = 0;
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
    struct FAT32FileAllocationTable file_table = fat32_driver_state.fat_table; 
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
    write_clusters(&file_table, FAT_CLUSTER_NUMBER, 1);
    struct FAT32DirectoryTable *dir_table = &(fat32_driver_state.dir_table_buf); 
    // create_empty_dir_table(&dir_table, ROOT_CLUSTER_NUMBER, ROOT_CLUSTER_NUMBER);
    init_directory_table(dir_table, "root", ROOT_CLUSTER_NUMBER);
    /**
     * eits, setelah kita bikin harusnya kita buat dia ada isi "awal" dong bukan NULL doang 
    */
    write_clusters(dir_table, ROOT_CLUSTER_NUMBER, 1);
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
        read_clusters(&fat32_driver_state.fat_table, FAT_CLUSTER_NUMBER, 1);
        read_clusters(&fat32_driver_state.dir_table_buf, ROOT_CLUSTER_NUMBER, 1);
}

int32_t driver_dir_table_linear_scan(char name[8], char ext[3], bool find_empty) {
    for (uint32_t i = 0; i < TOTAL_DIRECTORY_ENTRY; i++) {
        struct FAT32DirectoryEntry entry = fat32_driver_state.dir_table_buf.table[i];
        bool is_entry_not_empty          = (entry.user_attribute & UATTR_NOT_EMPTY);
        bool search_and_found_empty      = find_empty && !is_entry_not_empty;
        bool name_match                  = is_entry_not_empty && !memcmp(entry.name, name, 8) && !memcmp(entry.ext, ext, 3);
        if (search_and_found_empty || name_match)
            return i;
    }
    return -1;
}

bool get_dir_table_from_cluster(uint32_t cluster, struct FAT32DirectoryTable *dir_entry) {
    if (fat32_driver_state.fat_table.cluster_map[cluster] != FAT32_FAT_END_OF_FILE)
        return false;

    read_clusters(dir_entry, cluster, 1);
    // if (strcmp(dir_entry->table[1].name, "..", 8) == 0 &&
    //         dir_entry->table[1].attribute == ATTR_SUBDIRECTORY &&
        if( strcmp(dir_entry->table[1].name, "..", 2), dir_entry->table[0].attribute == ATTR_SUBDIRECTORY)
            return true;
    return false;
}


int8_t read_directory(struct FAT32DriverRequest request) {
    struct FAT32DirectoryTable *dir_table = &fat32_driver_state.dir_table_buf;

    // Ensure parent directory is valid
    bool isParentValid = get_dir_table_from_cluster(request.parent_cluster_number, dir_table);
    if (!isParentValid) {
        return -1;
    }

    // Use the driver_dir_table_linear_scan function to find the directory entry
    int32_t entry_index = driver_dir_table_linear_scan(request.name, request.ext, false);

    if (entry_index == -1) {
        // Directory not found
        return 2;
    }

    // Directory entry found, check if it's a subdirectory
    struct FAT32DirectoryEntry entry = dir_table->table[entry_index];
    if (entry.attribute != ATTR_SUBDIRECTORY) {
        // Not a directory
        return 1;
    }

    // Extract cluster number from the directory entry
    uint32_t cluster_number = entry.cluster_low + ((uint32_t)entry.cluster_high << 16);

    // Read the cluster content into the provided buffer
    read_clusters(request.buf, cluster_number, 1);

    return 0;
}

int8_t read(struct FAT32DriverRequest request){
    struct FAT32DirectoryTable *dir_table = &fat32_driver_state.dir_table_buf; 
    bool isParentValid = get_dir_table_from_cluster(request.parent_cluster_number, dir_table);
    if (!isParentValid){
        return -1;
    }
    bool found = false;
    int i;
    int idx;
    int j = 0;

    read_clusters(dir_table, request.parent_cluster_number, 1);
    for (i=0; i<TOTAL_DIRECTORY_ENTRY; i++){
        if (dir_table->table[i].user_attribute == UATTR_NOT_EMPTY
        && strcmp(dir_table->table[i].name, request.name, 8) == 0
        && strcmp(dir_table->table[i].ext, request.ext, 3) == 0){
            if(dir_table->table[i].attribute == ATTR_SUBDIRECTORY){ //Bukan sebuah file
                return 1;
            }
            if(request.buffer_size < dir_table->table[i].filesize){ //Ukuran request tidak cukup
                return -1;
            }
            found = true;
            idx = i;
            break;
        }
    }

    //Jika file tidak tertemu, return 2
    if(!found) return 2;
    //file ketemu
    struct FAT32FileAllocationTable *fat_table = &fat32_driver_state.fat_table;
    read_clusters(fat_table, FAT_CLUSTER_NUMBER, 1);
    uint32_t cluster_number = dir_table->table[idx].cluster_low + (((uint32_t)dir_table->table[idx].cluster_high) >> 16);
    while (cluster_number != FAT32_FAT_END_OF_FILE){
        read_clusters(request.buf+CLUSTER_SIZE*j, cluster_number, 1);
        cluster_number = fat_table->cluster_map[cluster_number];
        j++;
    }
    //file ketemu
    return 0;

}

int8_t write(struct FAT32DriverRequest request){
    //pada bagian isFile mengecek apakah request termasuk ke dalam file atau folder 
    bool isFolder = (request.buffer_size == 0);
    struct FAT32DirectoryTable dir_table;
    bool isParentValid = get_dir_table_from_cluster(request.parent_cluster_number, &dir_table);
    if (!isParentValid){
        return 2;
    }

    bool found = false;
    int i;

    //Iterasi setiap file dalam directoryTable parent
    /**
     * pada bagian kita memastikan folder atau file yang dicari 
     * berada dalam direktori parent tidak ada. 
    */

    for(i=0; i<TOTAL_DIRECTORY_ENTRY; i++){

        if( fat32_driver_state.dir_table_buf.table[i].user_attribute == UATTR_NOT_EMPTY
        && strcmp(fat32_driver_state.dir_table_buf.table[i].name, request.name, 8) == 0
        && (isFolder || strcmp(fat32_driver_state.dir_table_buf.table[i].ext, request.ext, 3) == 0)
        ){
            found = true;
            break;
        }
    }
    //Jika ada file/folder dengan nama sama atau sudah ADA, return 1
    if (found) return 1;

    /**
     * kasus dimana file/folder belum ada. kita nyari yang kosong
    */
    int idx_empty_entry = -1;
    for(i = 0; i < TOTAL_DIRECTORY_ENTRY; i++){
        //kasus kalo? file/folder (entry) tidak ada
        if(fat32_driver_state.dir_table_buf.table[i].user_attribute != UATTR_NOT_EMPTY){
            idx_empty_entry = i;
            break;
        }
    }

    /**
     * kasus dimana sudah melakukan iterasi sampe akhir dan belum ditemukan sama sekali 
     * sehingga bisa dibilang directorynya penuh 
    */
    if(idx_empty_entry == -1){
        return -1;
    }

    /**
     * Algoritma utama
    */
    //tentuin folder atau file? 
    uint32_t filesize;
    if(request.buffer_size == 0){filesize = CLUSTER_SIZE;}
    else filesize = request.buffer_size;

    //hitung filesize dalam cluster_size; 
    int alloc_cluster = (filesize + CLUSTER_SIZE - 1) / CLUSTER_SIZE;
    uint32_t empty_clusters[alloc_cluster];

    //ini hanyalah sebuah iterator
    int curr_cluster = 0; 
    int empty_cluster = 0; 
    /**
     * kalo misalkan empty cluster belum sepenuhnya terisi dan current cluster tidak melebihi dari size cluster map
    */
    while(empty_cluster < alloc_cluster && curr_cluster < CLUSTER_MAP_SIZE){
        /**bagian ini memastikan cluster block kosong atau tidak sehingga bisa diisi
         * FAT32_FAT_EMPTY_ENTRY menandakan block dari cluster adalah block kosong yang dapat terisi 
         */
        uint32_t is_cluster_empty = fat32_driver_state.fat_table.cluster_map[curr_cluster];
        if(is_cluster_empty == FAT32_FAT_EMPTY_ENTRY){
            empty_clusters[empty_cluster++] = curr_cluster;
        }
        curr_cluster++;
    }   
    /**
     * kasus dimana file allocation tidak berhasil 
     * sehingga akan direturn menjadi -1 sebagai error;
    */
    if(empty_cluster < alloc_cluster){
        return -1;
    }

    /**
     * menulis ke dalam direktori
    */
    struct FAT32DirectoryEntry *dir_entry = &fat32_driver_state.dir_table_buf.table[idx_empty_entry];
    dir_entry->user_attribute = 0x0;
    dir_entry->filesize = filesize; 
    //bagian ini masukin atribut folder atau file
    if(!isFolder){
        dir_entry->attribute = 0;
    }
    else dir_entry->attribute = ATTR_SUBDIRECTORY;
    //karena udah keisi otomatis gak mungkin kosong dong
    dir_entry->user_attribute = UATTR_NOT_EMPTY;
    /**
     * untuk bagian cluster kita ambil dari empty_clusters yang udah kita isi 
     * masing-masing clusternya
    */
    dir_entry->cluster_low = (uint16_t) empty_clusters[0] & 0xFFFF; //ekstrak bit-bit yang ada
    dir_entry->cluster_high = (uint16_t) (empty_clusters[0] >> 16); //ekstrak bit-bit yang ada
    //transfer nama dari request ke dir_entry
    //kalo ini adalah file
    copyStringWithLength(dir_entry->name, request.name, 8);
    if(!isFolder){
        memcpy(dir_entry->ext, request.ext, 3);
    }
    write_clusters(&fat32_driver_state.dir_table_buf, request.parent_cluster_number, 1);


    /**
     * kalo misalkan folder kita perlu bikin folder baru?
     * ini bener atau enggak sih 
    */
    struct FAT32DirectoryTable new_table;
    void *ptr = request.buf;
    if(isFolder){
        //membuat folder tabel baru
        init_directory_table(&new_table, request.name, request.parent_cluster_number);
        ptr = (void *)&new_table;
        // write_clusters(&new_table, empty_clusters[0], alloc_cluster);
    }
     
    // menggunakan linked list 
    for (int i = 0; i < alloc_cluster; i++) {
        uint32_t cluster = empty_clusters[i];   
        uint32_t next_cluster;
        if (i + 1 == alloc_cluster) {
            struct ClusterBuffer buffer;
            next_cluster = FAT32_FAT_END_OF_FILE;
            int current_size = filesize % CLUSTER_SIZE;
            if (current_size != 0) {
                memcpy(&buffer, ptr, current_size);
                memset(&buffer.buf[current_size], 0x00, CLUSTER_SIZE - current_size);
                // for (int j = current_size; j < CLUSTER_SIZE; ++j)
                //     buffer.buf[j] = 0x0;
                // request.buf = (void *)&buffer;
                ptr = (void *)&buffer;
            }
        }
        else{
            next_cluster = empty_clusters[i + 1];
        }
        write_clusters(ptr, cluster, 1);
        ptr += CLUSTER_SIZE;
        fat32_driver_state.fat_table.cluster_map[cluster] = next_cluster;
    }
    write_clusters(fat32_driver_state.fat_table.cluster_map, FAT_CLUSTER_NUMBER, 1);

    return 0; //kasus berhasil 
}

int8_t delete(struct FAT32DriverRequest request){
    //dir_table merupakan directoryTable milik parent
    struct FAT32DirectoryTable *dir_table = &fat32_driver_state.dir_table_buf;
    bool isParentValid = get_dir_table_from_cluster(request.parent_cluster_number, dir_table);
    if (!isParentValid){
        return -1;
    }

    //Cari file/folder yang di-request
    bool found = false;
    bool isFolder = false;
    int rc; //request cluster
    for(rc = 0; rc<TOTAL_DIRECTORY_ENTRY; rc++){
        if(dir_table->table[rc].user_attribute == UATTR_NOT_EMPTY
        && strcmp(dir_table->table[rc].name, request.name, 8) == 0
        && (strcmp(dir_table->table[rc].ext, request.ext, 3) == 0)
        ){
            found = true;
            isFolder = dir_table->table[rc].attribute == ATTR_SUBDIRECTORY; //Perlu testing, apakah jika request berupa folder ini true?
            break;
        }
    }

    if (!found) return 1;
    else{
        if (isFolder){
            //Validasi bahwa folder kosong
            bool isKosong = true;
            struct FAT32DirectoryTable *request_dt = {0};
            get_dir_table_from_cluster(rc, request_dt);

            int i; //i dimulai dari 2 karena 0 dan 1 berupa "." dan ".."
            for(i = 2; i<TOTAL_DIRECTORY_ENTRY; i++){
                if(request_dt->table[i].user_attribute != UATTR_NOT_EMPTY //Perlu testing
                && strcmp(request_dt->table[i].name, request.name, 8)
                && (strcmp(request_dt->table[i].ext, request.ext, 3))
                ){
                    isKosong = false;
                    break;
                }
            }
            if (isKosong) return 2;
        }

        //Jalankan operasi delete
        struct FAT32FileAllocationTable *fat_table = &fat32_driver_state.fat_table;
        int prev;

        //Kosongkan dir_table pada rc
        // (Tidak semua dikosongkan, tetapi yang terpenting adalah .attribute)
        dir_table->table[rc].attribute = !UATTR_NOT_EMPTY;
        for (int i=0; i<8; i++){
            dir_table->table[rc].name[i] = '\0';     
        }  
        for (int i=0; i<3; i++){
            dir_table->table[rc].ext[i] = '\0';     
        }  

        //Linked list allocationTable pada indeks rc jika ketemu file dalam folder, sekaligus delete cluster isi file/folder
        //rc++ Karena cluster indexing pada dir_table dimulai dari 0 (?)
        rc++;
        struct ClusterBuffer emptyBuffer = {0};
        while (fat_table->cluster_map[rc] != FAT32_FAT_END_OF_FILE){
            write_clusters(&emptyBuffer, rc, 1);
            prev = rc;
            rc = fat_table->cluster_map[rc]; 
            fat_table->cluster_map[prev] = FAT32_FAT_EMPTY_ENTRY;
        }
        write_clusters(&emptyBuffer, rc, 1);
        fat_table->cluster_map[rc] = FAT32_FAT_EMPTY_ENTRY;
        
        //Delete dengan overwrite HDD dengan FAT, dir_table, dan cluster yang dimodifikasi
        write_clusters(fat_table, FAT_CLUSTER_NUMBER, 1);
        write_clusters(dir_table, request.parent_cluster_number, 1);
    }

    
    return 0;
}