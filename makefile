# Compiler & linker
ASM           = nasm
LIN           = ld
CC            = gcc
ISO           = genisoimage
# Directory
SOURCE_FOLDER = src
SOURCE_FOLDER_CODE = src/code
SOURCE_FOLDER_STDLIB = src/stdlib
SOURCE_FOLDER_KERNEL = src/kernel
OUTPUT_FOLDER = bin
ISO_NAME      = os2024

# Flags
WARNING_CFLAG = -Wall -Wextra -Werror
DEBUG_CFLAG   = -fshort-wchar -g
STRIP_CFLAG   = -nostdlib -fno-stack-protector -nostartfiles -nodefaultlibs -ffreestanding
CFLAGS        = $(DEBUG_CFLAG) $(WARNING_CFLAG) $(STRIP_CFLAG) -m32 -c -I$(SOURCE_FOLDER)
AFLAGS        = -f elf32 -g -F dwarf
LFLAGS        = -T $(SOURCE_FOLDER)/linker.ld -melf_i386
ISOFLAGS      = -R -b boot/grub/grub1 -no-emul-boot -boot-load-size 4 -A os -input-charset utf8 -quiet -boot-info-table -o bin/OS2024.iso bin/iso

#wildcard
SRCS := $(wildcard $(SOURCE_FOLDER)/**/*.c)
OBJS_CODE := $(patsubst $(SOURCE_FOLDER_CODE)/%.c, $(OUTPUT_FOLDER)/%.o, $(SRCS))
OBJS_STDLIB := $(patsubst $(SOURCE_FOLDER_STDLIB)/%.c, $(OUTPUT_FOLDER)/%.o, $(SRCS))
OBJS_KERNEL := $(patsubst $(SOURCE_FOLDER_KERNEL)/%.c, $(OUTPUT_FOLDER)/%.o, $(SRCS))

#otomisasi 
$(OUTPUT_FOLDER)/%.o: $(SOURCE_FOLDER_CODE)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

$(OUTPUT_FOLDER)/%.o: $(SOURCE_FOLDER_STDLIB)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

$(OUTPUT_FOLDER)/%.o: $(SOURCE_FOLDER_KERNEL)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

#prerequisites
prereq: $(OBJS_CODE) $(OBJS_STDLIB) $(OBJS_KERNEL)
#main
run: all
	@qemu-system-i386 -s -S -cdrom $(OUTPUT_FOLDER)/$(ISO_NAME).iso
all: build
build: iso
clean:
	rm -rf $(OUTPUT_FOLDER)/*.o
	rm -rf *.o *.iso $(OUTPUT_FOLDER)/kernel
	rm -rf $(OUTPUT_FOLDER)/*.iso

kernel: prereq
	@$(ASM) $(AFLAGS) $(SOURCE_FOLDER)/kernel-entrypoint.s -o $(OUTPUT_FOLDER)/kernel-entrypoint.o
	@$(LIN) $(LFLAGS) $(OUTPUT_FOLDER)/*.o -o $(OUTPUT_FOLDER)/kernel
	@echo Linking object files and generate elf32...
	@rm -f *.o

iso: kernel
	mkdir -p $(OUTPUT_FOLDER)/iso/boot/grub
	cp $(OUTPUT_FOLDER)/kernel     $(OUTPUT_FOLDER)/iso/boot/
	cp other/grub1                 $(OUTPUT_FOLDER)/iso/boot/grub/
	cp $(SOURCE_FOLDER)/menu.lst   $(OUTPUT_FOLDER)/iso/boot/grub/
	$(ISO) $(ISOFLAGS)
	rm -r $(OUTPUT_FOLDER)/iso/
	@echo succesfully linked files

