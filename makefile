# Compiler & linker
ASM           = nasm
LIN           = ld
CC            = gcc
ISO           = genisoimage
DISK_NAME     = storage

# Directory
SOURCE_FOLDER = src
SOURCE_FOLDER_CODE = src/code
SOURCE_FOLDER_STDLIB = src/stdlib
SOURCE_FOLDER_KERNEL = src/kernel
SOURCE_FOLDER_ASM = src/asm
OUTPUT_FOLDER = bin
ISO_NAME      = OS2024

# Flags
WARNING_CFLAG = -Wall -Wextra -Werror
DEBUG_CFLAG   = -fshort-wchar -g
STRIP_CFLAG   = -nostdlib -fno-stack-protector -nostartfiles -nodefaultlibs -ffreestanding
CFLAGS        = $(DEBUG_CFLAG) $(WARNING_CFLAG) $(STRIP_CFLAG) -c -I$(SOURCE_FOLDER)
AFLAGS        = -f elf32 -g -F dwarf
LFLAGS        = -T $(SOURCE_FOLDER)/linker.ld -melf_i386
ISOFLAGS      = -R -b boot/grub/grub1 -no-emul-boot -boot-load-size 4 -A os -input-charset utf8 -quiet -boot-info-table -o bin/OS2024.iso bin/iso

#otomisasi 
SRCS_CODE := $(wildcard $(SOURCE_FOLDER)/code/*)
SRCS_STDLIB := $(wildcard $(SOURCE_FOLDER)/stdlib/*.c)
SRCS_KERNEL := $(wildcard $(SOURCE_FOLDER)/kernel/*.c)
ASM_SRCS := $(wildcard $(SOURCE_FOLDER_ASM)/*.s)

OBJS_CODE := $(foreach dir,$(SRCS_CODE), $(wildcard $(dir)/*.c))

OBJS_STDLIB := $(patsubst $(SOURCE_FOLDER)/stdlib/%.c, $(OUTPUT_FOLDER)/%.o, $(SRCS_STDLIB))
OBJS_KERNEL := $(patsubst $(SOURCE_FOLDER)/kernel/%.c, $(OUTPUT_FOLDER)/%.o, $(SRCS_KERNEL))
ASM_OBJS := $(patsubst $(SOURCE_FOLDER_ASM)/%.s, $(OUTPUT_FOLDER)/%.o, $(ASM_SRCS))
OBJS := $(patsubst $(SOURCE_FOLDER)/code/%.c, $(OUTPUT_FOLDER)/%.o, $(OBJS_CODE))

$(shell mkdir -p $(dir $(OBJS)))

$(OUTPUT_FOLDER)/%.o: $(SOURCE_FOLDER)/code/%.c
	$(CC) $(CFLAGS) -c $< -o $@

$(OUTPUT_FOLDER)/%.o: $(SOURCE_FOLDER_STDLIB)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

$(OUTPUT_FOLDER)/%.o: $(SOURCE_FOLDER_KERNEL)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

$(OUTPUT_FOLDER)/%.o: $(SOURCE_FOLDER_ASM)/%.s
	$(ASM) $(AFLAGS) $< -o $@

#prerequisites
prereq = $(OBJS) $(OBJS_STDLIB) $(OBJS_KERNEL) $(ASM_OBJS)  
#main
run: all
	@qemu-system-i386 -s -S -drive file=$(OUTPUT_FOLDER)/storage.bin,format=raw,if=ide,index=0,media=disk -cdrom $(OUTPUT_FOLDER)/$(ISO_NAME).iso 
all: build
build: iso
clean:
	$(shell rm -rf  $(dir $(OBJS)))
	rm -rf $(OUTPUT_FOLDER)/*.o
	rm -rf *.o *.iso $(OUTPUT_FOLDER)/kernel
	rm -rf $(OUTPUT_FOLDER)/*.iso

t: $(prereq)
	echo $^

kernel: $(prereq)
	@$(LIN) $(LFLAGS) $^ -o $(OUTPUT_FOLDER)/kernel
	@echo Linking object files and generate elf32...
	@rm -f *.o

disk:
	@qemu-img create -f raw $(OUTPUT_FOLDER)/$(DISK_NAME).bin 4M

iso: kernel
	mkdir -p $(OUTPUT_FOLDER)/iso/boot/grub
	cp $(OUTPUT_FOLDER)/kernel     $(OUTPUT_FOLDER)/iso/boot/
	cp other/grub1                 $(OUTPUT_FOLDER)/iso/boot/grub/
	cp $(SOURCE_FOLDER)/menu.lst   $(OUTPUT_FOLDER)/iso/boot/grub/
	$(ISO) $(ISOFLAGS)
	rm -r $(OUTPUT_FOLDER)/iso/
	@echo succesfully linked files

inserter:
	@$(CC) -Wno-builtin-declaration-mismatch -g -I$(SOURCE_FOLDER) \
		$(SOURCE_FOLDER)/stdlib/string.c \
		$(SOURCE_FOLDER_CODE)/filesystem/fat32.c \
		$(SOURCE_FOLDER_CODE)/external/external-inserter.c \
		-o $(OUTPUT_FOLDER)/inserter

user-shell:
	@$(ASM) $(AFLAGS) $(SOURCE_FOLDER_CODE)/crt0.s -o crt0.o
	@$(CC)  $(CFLAGS) -fno-pie $(SOURCE_FOLDER_CODE)/user-shell.c -o user-shell.o
	@$(CC)  $(CFLAGS) -fno-pie $(SOURCE_FOLDER)/stdlib/string.c -o string.o
	@$(LIN) -T $(SOURCE_FOLDER)/user-linker.ld -melf_i386 --oformat=binary \
		$(SOURCE_FOLDER_ASM)/crt0.o user-shell.o -o $(OUTPUT_FOLDER)/shell
	@echo Linking object shell object files and generate flat binary...
	@$(LIN) -T $(SOURCE_FOLDER)/user-linker.ld -melf_i386 --oformat=elf32-i386 \
		$(SOURCE_FOLDER_ASM)/crt0.o user-shell.o string.o -o $(OUTPUT_FOLDER)/shell_elf
	@echo Linking object shell object files and generate ELF32 for debugging...
	@size --target=binary $(OUTPUT_FOLDER)/shell
	@rm -f *.o

insert-shell: inserter user-shell
	@echo Inserting shell into root directory...
		@cd $(OUTPUT_FOLDER); ./inserter shell 2 $(DISK_NAME).bin

.PHONY: all