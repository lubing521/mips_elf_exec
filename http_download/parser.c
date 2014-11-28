#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>

typedef uint32_t addr_t; /* 目标设备的地址指针类型 */

#define LINE_BUF_LEN    256
#define SYMBOL_LEN      64

#define BUG(fmt, arg...) \
    do { \
        fprintf(stderr, "*BUG* %s[%d]: " fmt "\n", __func__, __LINE__, ##arg); \
        exit(-1); \
    } while (0)

#define ERR(fmt, arg...) \
    do { \
        fprintf(stderr, "*ERR* %s[%d]: " fmt "\n", __func__, __LINE__, ##arg); \
    } while (0)

static char sym_stub_file[] = "sym_stub.c";
static char sym_stub_temp_file[] = "sym_stub.c.temp";

static int is_neglected_stub_line(char *line)
{
    if (line == NULL) {
        return 1;
    }

    if (strncmp(line, "data_", 5) == 0) {
        return 0;
    }
    if (strncmp(line, "func_", 5) == 0) {
        return 0;
    }

    return 1;
}

static int is_valid_stub_line(char *line)
{
    if (line == NULL) {
        return 0;
    }

    /* TODO */
    return 1;
}

static char *get_sym_from_stub_line(char *symbuf, char *line)
{
    unsigned int len;
    char *cp, *cq;

    if (symbuf == NULL || line == NULL) {
        return NULL;
    }

    cp = strchr(line, ' ');
    if (cp == NULL) {
        return NULL;
    }
    for (cp = cp + 1; *cp == ' '; cp++) {
        (void)0;
    }
    cq = strchr(cp, ' ');
    if (cq == NULL) {
        return NULL;
    }

    len = (unsigned int)cq - (unsigned int)cp;
    if (len >= SYMBOL_LEN) {
        BUG();
    }

    memset(symbuf, 0, SYMBOL_LEN);
    memcpy(symbuf, cp, len);

    return symbuf;
}

static addr_t get_symaddr_from_symtab(char *sym, FILE *symtab)
{
    char line_buf[LINE_BUF_LEN];
    int llen;
    addr_t ret = 0;
    char *cp;

    if (sym == NULL || symtab == NULL) {
        BUG();
    }

    if (fseek(symtab, 0, SEEK_SET) != 0) {
        BUG("fseek to symbol table file start position failed.");
    }

    while (fgets(line_buf, LINE_BUF_LEN, symtab) != NULL) {
        llen = strlen(line_buf);
        if (line_buf[llen - 1] != '\n' && line_buf[llen - 1] != '\r') {
            BUG();
        }

        if ((cp = strstr(line_buf, sym)) == NULL) {
            continue;
        }
        cp--;
        if (*cp != ' ') {
            continue;
        }
        cp++;
        cp += strlen(sym);
        if (!(*cp == '\r' || *cp == '\n')) {
            continue;
        }

        ret = strtoul(line_buf, NULL, 16);
        break;
    }

    return ret;
}

static void regen_stub_line(addr_t symaddr, char *line)
{
    char *cp;

    if (symaddr == 0 || line == NULL) {
        BUG();
    }

    cp = strstr(line, "0x");
    if (cp == NULL) {
        BUG("Line missing \"0x\": %s", line);
    }

    if ((cp - line) > (LINE_BUF_LEN - 16)) {
        BUG("Line too long: %s", line);
    }

    sprintf(cp, "0x%08x;\n", symaddr);
}

static void usage(char *cmd)
{
    printf("Usage: %s symbol_table.txt\n", cmd);
}

int main(int argc, char *argv[])
{
    FILE *fsymtab, *fstub, *fstub_temp;
    char line[LINE_BUF_LEN];
    int line_len;
    char symbol[SYMBOL_LEN];
    addr_t symaddr;
    char *cp;

    if (argc != 2) {
        usage(argv[0]);
        return 0;
    }

    fsymtab = fopen(argv[1], "r");
    if (fsymtab == NULL) {
        ERR("Open symbol table failed %s.", argv[1]);
        return -1;
    }

    fstub = fopen(sym_stub_file, "r");
    if (fstub == NULL) {
        ERR("Open symbol stub failed %s.", sym_stub_file);
        return -1;
    }

    fstub_temp = fopen(sym_stub_temp_file, "w");
    if (fstub_temp == NULL) {
        ERR("Open symbol stub temporary failed %s.", sym_stub_temp_file);
        return -1;
    }

    while (fgets(line, LINE_BUF_LEN, fstub) != NULL) {
        line_len = strlen(line);
        if (line[line_len - 1] != '\n' && line[line_len - 1] != '\r') {
            BUG("stub file line too long: %s", line);
        }

        if (is_neglected_stub_line(line)) {
            cp = strstr(line, "\r\n");
            if (cp != NULL) {
                *cp = '\n';     /* 去掉windows模式下面的回车模式 */
            }
            fprintf(fstub_temp, "%s", line);
            continue;
        }

        if (!is_valid_stub_line(line)) {
            BUG("invalid stub line: %s", line);
        }

        if (get_sym_from_stub_line(symbol, line) == NULL) {
            BUG("get symbol failed: %s", line);
        }

        symaddr = get_symaddr_from_symtab(symbol, fsymtab);
        if (symaddr == 0) {
            ERR("Not find symbol %s address.", symbol);
            return -1;
        }

        regen_stub_line(symaddr, line);
        fprintf(fstub_temp, "%s", line);
    }

    fclose(fsymtab);
    fclose(fstub);
    if (fclose(fstub_temp) != 0) {
        BUG("Close stub temporary file failed: %s.", sym_stub_temp_file);
    }

    if (remove(sym_stub_file) != 0) {
        BUG("Remove old stub file failed.");
    }

    if (rename(sym_stub_temp_file, sym_stub_file) != 0) {
        BUG("Rename new stub file failed.");
    }

    return 0;
}

