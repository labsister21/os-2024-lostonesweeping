#include "header/filesystem/fat32.h"
#include "header/stdlib/string.h"
#include <stdint.h>
#include <stdbool.h>

static struct FAT32DriverState fat32_driver_state;


void copyStringWithLength(char* destination, const char* source, size_t length) {
    for (size_t i = 0; i < length; ++i) {
        destination[i] = source[i];
        if (source[i] == '\0') // If source string ends earlier than specified length
            break;
    }
    destination[length] = '\0'; // Null-terminate the destination string
}

void *my_memcpy(void *dest, const void *src, size_t length) {
    char *dest_ptr = (char *)dest;
    const char *src_ptr = (const char *)src;

    for (size_t i = 0; i < length; ++i) {
        dest_ptr[i] = src_ptr[i];
    }

    return dest;  // Return a pointer to the destination memory
}

void my_memset(void *ptr, int value, size_t num) {
    uint8_t *p = (uint8_t *)ptr;
    for (size_t i = 0; i < num; i++) {
        p[i] = (uint8_t)value;
    }
}

int my_memcmp(const void *ptr1, const void *ptr2, size_t n) {
    const unsigned char *p1 = ptr1;
    const unsigned char *p2 = ptr2;
    for (size_t i = 0; i < n; ++i) {
        if (p1[i] != p2[i]) {
            return (p1[i] > p2[i]) - (p1[i] < p2[i]);
        }
    }
    return 0;
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

void init_directory_table(struct FAT32DirectoryTable *dir_table, char *name, uint32_t parent_dir_cluster) {
    // Calculate the size of a directory entry
    size_t entry_size = sizeof(struct FAT32DirectoryEntry);

    // Initialize self entry
    struct FAT32DirectoryEntry self_entry;
    memset(&self_entry, 0, entry_size); // Initialize with zeros
    copyStringWithLength(self_entry.name, name, 8); // Copy name (up to 8 characters)
    self_entry.attribute = ATTR_SUBDIRECTORY;
    self_entry.user_attribute = UATTR_NOT_EMPTY;
    self_entry.cluster_low = (uint16_t)parent_dir_cluster;
    self_entry.cluster_high = (uint16_t)(parent_dir_cluster >> 16);

    // Initialize parent entry
    struct FAT32DirectoryEntry parent_entry;
    memset(&parent_entry, 0, entry_size); // Initialize with zeros
    if (memcmp(name, ".", 4) == 0) {
        copyStringWithLength(parent_entry.name, ".", 8); // Set name to "."
        parent_entry.cluster_low = (uint16_t)parent_dir_cluster; // Set to current directory cluster
        parent_entry.cluster_high = (uint16_t)(parent_dir_cluster >> 16); // For FAT32, consider higher bits of current cluster
    } else {
        copyStringWithLength(parent_entry.name, "..", 8); // Set name to ".."
        parent_entry.attribute = ATTR_SUBDIRECTORY;
        parent_entry.user_attribute = UATTR_NOT_EMPTY;
        parent_entry.cluster_low = (uint16_t)(parent_dir_cluster); // Set to parent directory cluster
        parent_entry.cluster_high = (uint16_t)(parent_dir_cluster >> 16); // For FAT32, consider higher bits of parent cluster
    }

    memcpy(dir_table, &self_entry, entry_size);
    memcpy((char *)dir_table + entry_size, &parent_entry, entry_size);
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
    write_clusters(&file_table, FAT_CLUSTER_NUMBER, 1);
    struct FAT32DirectoryTable dir_table; 
    // create_empty_dir_table(&dir_table, ROOT_CLUSTER_NUMBER, ROOT_CLUSTER_NUMBER);
    init_directory_table(&dir_table, ".", ROOT_CLUSTER_NUMBER);
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

bool cmp_string_with_fixed_length(const char *a, const char *b, int l){
    int i;
    for (i=0; i<l; i++){
        if (a[i] != b[i]){
            return false;
        }
    }
    return true;
}

bool get_dir_table_from_cluster(uint32_t cluster, struct FAT32DirectoryTable *dir_entry) {
    if (fat32_driver_state.fat_table.cluster_map[cluster] !=
            FAT32_FAT_END_OF_FILE)
        return false;
    read_clusters(dir_entry, cluster, 1);
    if (cmp_string_with_fixed_length(dir_entry->table[0].name, ".", 8) == 0 &&
            dir_entry->table[0].attribute == ATTR_SUBDIRECTORY &&
            cmp_string_with_fixed_length(dir_entry->table[1].name, "..", 8) == 0 &&
            dir_entry->table[1].attribute == ATTR_SUBDIRECTORY)
        return true;
    return false;
}


int8_t read_directory(struct FAT32DriverRequest request){
    //Asumsi parent merupakan folder valid
    //int8_t parent = fat32_driver_state.fat_table.cluster_map[request.parent_cluster_number];

    // bool isParentValid = get_dir_table_from_cluster(request.parent_cluster_number, &fat32_driver_state.dir_table_buf);
    // if (!isParentValid){
    //     return -1;
    // }

    bool found = false;
    int i;

    //Iterasi setiap file dalam directoryTable parent
    for (i=0; i<TOTAL_DIRECTORY_ENTRY; i++){
        if (fat32_driver_state.dir_table_buf.table[i].user_attribute != UATTR_NOT_EMPTY
        && cmp_string_with_fixed_length(fat32_driver_state.dir_table_buf.table[i].name, request.name, 8)){
            if(fat32_driver_state.dir_table_buf.table[i].attribute == ATTR_SUBDIRECTORY){ //Bukan sebuah folder
                return 1;
            }
            if(request.buffer_size < fat32_driver_state.dir_table_buf.table[i].filesize){ //Ukuran request tidak cukup
                return -1;
            }
            found = true;
            break;
        }
    }

    //Jika file tidak tertemu, return 2
    if (!found) return 2;

    //Folder terdapat dalam allocationTable
    uint32_t cluster_number = fat32_driver_state.dir_table_buf.table[i].cluster_low + (fat32_driver_state.dir_table_buf.table[i].cluster_high >> 16);
    read_clusters(request.buf, cluster_number, 1);

    return 0;
}

int8_t read(struct FAT32DriverRequest request){
    //Asumsi parent merupakan folder valid
    //int8_t parent = fat32_driver_state.fat_table.cluster_map[request.parent_cluster_number];

    // bool isParentValid = get_dir_table_from_cluster(request.parent_cluster_number, &fat32_driver_state.dir_table_buf);
    // if (!isParentValid){
    //     return -1;
    // }

    bool found = false;
    int i;
    int j = 0;

    //Iterasi setiap file dalam directoryTable parent
    for (i=0; i<TOTAL_DIRECTORY_ENTRY; i++){
        if (fat32_driver_state.dir_table_buf.table[i].user_attribute != UATTR_NOT_EMPTY
        && cmp_string_with_fixed_length(fat32_driver_state.dir_table_buf.table[i].name, request.name, 8)
        && cmp_string_with_fixed_length(fat32_driver_state.dir_table_buf.table[i].ext, request.ext, 3)){
            if(fat32_driver_state.dir_table_buf.table[i].attribute != ATTR_SUBDIRECTORY){ //Bukan sebuah file
                return 1;
            }
            if(request.buffer_size < fat32_driver_state.dir_table_buf.table[i].filesize){ //Ukuran request tidak cukup
                return -1;
            }
            found = true;
            break;
        }
    }

    //Jika file tidak tertemu, return 2
    if (!found) return 2;

    //Linked list allocationTable pada indeks i jika ketemu file dalam folder
    uint32_t cluster_number = fat32_driver_state.dir_table_buf.table[i].cluster_low + (fat32_driver_state.dir_table_buf.table[i].cluster_high >> 16);
    while (fat32_driver_state.fat_table.cluster_map[i] != FAT32_FAT_END_OF_FILE){
        read_clusters(request.buf + CLUSTER_SIZE*j, cluster_number, 1);
        i = fat32_driver_state.fat_table.cluster_map[i];
        j++;
    }

    return 0;
}

int8_t write(struct FAT32DriverRequest request){
    //pada bagian isFile mengecek apakah request termasuk ke dalam file atau folder 
    bool isFolder = (request.buffer_size == 0);
    // bool isParentValid = get_dir_table_from_cluster(request.parent_cluster_number, &fat32_driver_state.dir_table_buf);
    // if (!isParentValid){
    //     return -1;
    // }

    bool found = false;
    int i;

    //Iterasi setiap file dalam directoryTable parent
    /**
     * pada bagian kita memastikan folder atau file yang dicari 
     * berada dalam direktori parent tidak ada. 
    */


    for(i=0; i<TOTAL_DIRECTORY_ENTRY; i++){
        if( fat32_driver_state.dir_table_buf.table[i].user_attribute == UATTR_NOT_EMPTY
        && cmp_string_with_fixed_length(fat32_driver_state.dir_table_buf.table[i].name, request.name, 8)
        && (isFolder || cmp_string_with_fixed_length(fat32_driver_state.dir_table_buf.table[i].ext, request.ext, 3))
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
        if(idx_empty_entry == -1 && fat32_driver_state.dir_table_buf.table[i].user_attribute != UATTR_NOT_EMPTY){
            idx_empty_entry = i;
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
    if(isFolder){filesize = CLUSTER_SIZE;}
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
    dir_entry->filesize = filesize; 
    //bagian ini masukin atribut folder atau file
    if(isFolder){dir_entry->attribute = 0;}
    else dir_entry->attribute = ATTR_SUBDIRECTORY;
    //karena udah keisi otomatis gak mungkin kosong dong
    dir_entry->user_attribute = UATTR_NOT_EMPTY;
    /**
     * untuk bagian cluster kita ambil dari empty_clusters yang udah kita isi 
     * masing-masing clusternya
    */
    dir_entry->cluster_low = (uint8_t) empty_clusters[0]; //ekstrak bit-bit yang ada
    dir_entry->cluster_high = (uint8_t) (empty_clusters[0] >> 16); //ekstrak bit-bit yang ada
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
    struct FAT32DirectoryTable new_table = fat32_driver_state.dir_table_buf;
    if(isFolder){
        //membuat folder tabel baru
        init_directory_table(&new_table, request.name, request.parent_cluster_number);
        write_clusters(&new_table, empty_clusters[0], alloc_cluster);
    }
     
    // menggunakan linked list 
    struct ClusterBuffer buffer;
    for (int i = 0; i < alloc_cluster; i++) {
        uint32_t next_cluster = empty_clusters[i + 1];
        if (i + 1 == alloc_cluster) {
            next_cluster = FAT32_FAT_END_OF_FILE;
            int current_size = filesize % CLUSTER_SIZE;
            if (current_size != 0) {
                memcpy(&buffer, request.buf, current_size);
                for (int j = current_size; j < CLUSTER_SIZE; ++j)
                    buffer.buf[j] = 0x0;
                request.buf = (void *)&buffer;
            }
        }
        uint32_t cluster = empty_clusters[i];
        write_clusters(request.buf, cluster, 1);
        request.buf += CLUSTER_SIZE;
        fat32_driver_state.fat_table.cluster_map[cluster] = next_cluster;
    }
    write_clusters(fat32_driver_state.fat_table.cluster_map, FAT_CLUSTER_NUMBER, 1);
    return 0; //kasus berhasil 
}

int8_t delete(struct FAT32DriverRequest request){
    bool isFolder = request.buffer_size == 0;
    // bool isParentValid = get_dir_table_from_cluster(request.parent_cluster_number, &fat32_driver_state.dir_table_buf);
    // if (!isParentValid){
    //     return -1;
    // }

    bool found = false;
    int i;

    for(i=0; i<TOTAL_DIRECTORY_ENTRY; i++){
        if(fat32_driver_state.dir_table_buf.table[i].user_attribute == UATTR_NOT_EMPTY
        && cmp_string_with_fixed_length(fat32_driver_state.dir_table_buf.table[i].name, request.name, 8)
        && (isFolder || cmp_string_with_fixed_length(fat32_driver_state.dir_table_buf.table[i].ext, request.ext, 3))
        ){
            found = true;
            break;
        }
    }

    if (!found) return 1;
    else{
        int prev;
        //Linked list allocationTable pada indeks i jika ketemu file dalam folder
        while (fat32_driver_state.fat_table.cluster_map[i] != FAT32_FAT_END_OF_FILE){
            prev = i;
            i = fat32_driver_state.fat_table.cluster_map[i];
            fat32_driver_state.fat_table.cluster_map[prev] = 0;
        }
        fat32_driver_state.fat_table.cluster_map[i] = 0;
    }
    return 0;
}
