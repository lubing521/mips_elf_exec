#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <endian.h>

#include "converter.h"
#include "elf.h"

#define ELF_FILE_BUFLEN (1<<20)  /* temporary buffer for loading ELF executable file */

static char sym_stub_object[] = "sym_stub.o";
static char sym_stub_temp_object[] = "sym_stub.o.temp";

int is_image_valid(Elf32_Ehdr *hdr)
{
    return 1;
}

static int convert_object_do(Elf32_Ehdr  *hdr,
                             Elf32_Shdr  *shdr_sym,
                             Elf32_Sym   *syms,
                             char        *strings)
{
    int i;
    Elf32_Addr addr;
    Elf32_Shdr *shdr_tab;

    /* Not check argument validation now. */

    shdr_tab = (Elf32_Shdr *)((unsigned char *)hdr + be32toh(hdr->e_shoff));

    for (i = 0; i < be32toh(shdr_sym->sh_size) / sizeof(Elf32_Sym); i++) {
        if ((ELF32_ST_TYPE(syms[i].st_info) == STT_OBJECT)
                && (ELF32_ST_BIND(syms[i].st_info) == STB_GLOBAL)
                && (be32toh(syms[i].st_value) == 0)) {
            addr = *(Elf32_Addr *)((unsigned char *)hdr + be32toh(shdr_tab[be16toh(syms[i].st_shndx)].sh_offset));
            DBG("Convert %-32.31s to address 0x%08x.", (strings + be32toh(syms[i].st_name)), be32toh(addr));
            syms[i].st_value = addr;
            syms[i].st_shndx = htobe16(SHN_ABS);
            syms[i].st_size = htobe32(0);
        }
    }

    return 0;
}

static int convert_object(unsigned char *elf_start)
{
    int i;
    Elf32_Ehdr  *hdr = NULL;
    Elf32_Shdr  *shdr = NULL;
    Elf32_Sym   *syms = NULL;
    char        *strings = NULL;

    hdr = (Elf32_Ehdr *)elf_start;
    if (!is_image_valid(hdr)) {
        return -1;
    }

    shdr = (Elf32_Shdr *)(elf_start + be32toh(hdr->e_shoff));
    for (i = 0; i < be16toh(hdr->e_shnum); i++) {
        if (be32toh(shdr[i].sh_type) == SHT_SYMTAB) {
            syms = (Elf32_Sym *)(elf_start + be32toh(shdr[i].sh_offset));
            strings = (char *)((char *)elf_start + be32toh(shdr[be32toh(shdr[i].sh_link)].sh_offset));
            break;
        }
    }

    if (syms == NULL || strings == NULL) {
        return -1;
    }

    return convert_object_do(hdr, shdr + i, syms, strings);
}

int main()
{
    int ret = 0;
    FILE *felf, *felf_temp;
    unsigned char *elf_buf;
    size_t readlen, writelen;

    elf_buf = malloc(ELF_FILE_BUFLEN);
    if (elf_buf == NULL) {
        ERR("Alloc buffer for ELF file failed %d.", ELF_FILE_BUFLEN);
        ret = -1;
        goto err_out0;
    }

    felf = fopen(sym_stub_object, "rb");
    if (felf == NULL) {
        ERR("Open sym_stub object file failed: %s.", sym_stub_object);
        ret = -1;
        goto err_out1;
    }

    memset(elf_buf, 0, ELF_FILE_BUFLEN);
    readlen = fread(elf_buf, 1, ELF_FILE_BUFLEN, felf);
    if (readlen == ELF_FILE_BUFLEN) {
        ERR("ELF file buffer(%d) is used up.", ELF_FILE_BUFLEN);
        ret = -1;
        goto err_out2;
    }

    ret = convert_object(elf_buf);
    if (ret != 0) {
        ERR("Convert failed.");
        goto err_out2;
    }

    felf_temp = fopen(sym_stub_temp_object, "wb");
    if (felf_temp == NULL) {
        ERR("Create temporary sym_stub object file failed: %s.", sym_stub_temp_object);
        ret = -1;
        goto err_out2;
    }

    writelen = fwrite(elf_buf, 1, readlen, felf_temp);
    if (writelen != readlen) {
        ERR("Write converted ELF file failed: wanna %d, write %d.", readlen, writelen);
        ret = -1;
        goto err_out3;
    }

    if (fclose(felf_temp) != 0) {
        BUG("Close %s failed.", sym_stub_temp_object);
    }
    if (fclose(felf) != 0) {
        BUG("Close %s failed.", sym_stub_object);
    }
    free(elf_buf);
    if (remove(sym_stub_object) != 0) {
        BUG("Remove %s failed.", sym_stub_object);
    }
    if (rename(sym_stub_temp_object, sym_stub_object) != 0) {
        BUG("Rename %s to %s failed.", sym_stub_temp_object, sym_stub_object);
    }

    return 0;

err_out3:
    fclose(felf_temp);

err_out2:
    fclose(felf);

err_out1:
    free(elf_buf);

err_out0:
    return ret;
}

