#include <stdio.h>
#include<limits.h>
#include <string.h>
#include <fcntl.h>

typedef struct adv_item_s{
    struct adv_item_s * next;
    //struct list_head list; /* 广告所在链表 */
    char ad_url[64];    /* 广告url */
    char ad_type[8];   /* 广告资源的类型(flv/mp4等) */
    char ad_len;    /* 广告时长 */
}adv_item_t;

typedef struct adv_lists_s
{
#if 0
    struct list_head list_5s;
    struct list_head list_15s;
    struct list_head list_30s;
#endif

#if 1
    adv_item_t* list_5s[10];
    int list_5s_index;
    adv_item_t* list_15s[10];
    int list_15s_index;
    adv_item_t* list_30s[10];
    int list_30s_index;
#endif

}adv_lists_t;

adv_lists_t g_adv_lists;
char youku_andord_adv_head[] = "{\"P\":7,\"VAL\":[";
char youku_andord_adv_val_al[] = "{\"AL\":";
char youku_andord_adv_val_rs[] = ",\"AT\":70,\"BRS\":\"\",\"CUF\":1,\"RS\":\"";
char youku_andord_adv_val_vqt[] = "\",\"RST\":\"video\",\"SDKID\":0,\"SU\":[],\"VID\":\"174510121\",\"VQT\":\"";
char youku_andord_adv_val_end[] = "\"}";
char youku_andord_adv_end[] = "]}";

void make_youku_andord_adv_al(char * youku_andord_adv, int *offset,adv_item_t *padv)
{
    int copy_len;
    copy_len  = sizeof(youku_andord_adv_val_al) - 1;
    memcpy(youku_andord_adv + *offset, youku_andord_adv_val_al, copy_len);
    *offset += copy_len;
    *offset += sprintf(youku_andord_adv + *offset, "%d", padv->ad_len);
    
    copy_len  = sizeof(youku_andord_adv_val_rs) - 1;
    memcpy(youku_andord_adv + *offset, youku_andord_adv_val_rs, copy_len);
    *offset += copy_len;
    *offset += sprintf(youku_andord_adv + *offset, "%s", padv->ad_url);
    
    copy_len  = sizeof(youku_andord_adv_val_vqt) - 1;
    memcpy(youku_andord_adv + *offset, youku_andord_adv_val_vqt, copy_len);
    *offset += copy_len;
    *offset += sprintf(youku_andord_adv + *offset, "%s", padv->ad_type);
    
    copy_len  = sizeof(youku_andord_adv_val_end) - 1;
    memcpy(youku_andord_adv + *offset, youku_andord_adv_val_end, copy_len);
    *offset += copy_len;  
}


/* 根据播放视频的时长以及分类，决定投放哪些广告 */
adv_item_t * get_adv_items(int video_time, int video_type)
{
    adv_item_t * adv_head, *padv;
    /* 具体算法待完善 */

    adv_head = g_adv_lists.list_15s[0];
    padv = adv_head;
    padv->next = g_adv_lists.list_5s[0];
    padv = padv->next;
    padv->next = g_adv_lists.list_30s[0];
    padv = padv->next;
    padv->next = NULL;
    
    return adv_head;
}
int  make_youku_andord_adv(char * youku_andord_adv)
{
    int offset, copy_len;
    adv_item_t *padv;
    offset = 0;
    copy_len =  sizeof(youku_andord_adv_head) - 1;
    memcpy(youku_andord_adv + offset , youku_andord_adv_head, copy_len);
    offset += copy_len;
    padv = get_adv_items(30,0);
    if (padv) {
       make_youku_andord_adv_al(youku_andord_adv, &offset, padv);
       padv = padv->next;
    }

    while(padv) {
        offset += sprintf(youku_andord_adv + offset, "%s",",");
        make_youku_andord_adv_al(youku_andord_adv, &offset, padv);
        padv = padv->next;        
    }
    
    copy_len = sizeof(youku_andord_adv_end) - 1;
    memcpy(youku_andord_adv + offset , youku_andord_adv_end, copy_len);
    offset += copy_len;

    return offset;
}

int adv_f_get_c(fd)
{
    char ch[1];
    int read_len;
    read_len = read(fd, ch, 1);
    if (read_len != 1) {
        return 0;
    }
    return ch[0];
}

void add_adv_item_to_list(adv_item_t *adv_item)
{
    if (adv_item->ad_len == 5) {
        g_adv_lists.list_5s[g_adv_lists.list_5s_index++] = adv_item;
    }
    if (adv_item->ad_len == 15) {
        g_adv_lists.list_15s[g_adv_lists.list_15s_index++] = adv_item;
    }
    if (adv_item->ad_len == 30) {
        g_adv_lists.list_30s[g_adv_lists.list_30s_index++] = adv_item;
    }                
}

void show_adv_list()
{
    int i;
    for (i = 0; i < g_adv_lists.list_5s_index; i++) {
        printf("%s,%s,%d\n",g_adv_lists.list_5s[i]->ad_url, 
            g_adv_lists.list_5s[i]->ad_type,
            g_adv_lists.list_5s[i]->ad_len);
    }

    for (i = 0; i < g_adv_lists.list_15s_index; i++) {
        printf("%s,%s,%d\n",g_adv_lists.list_15s[i]->ad_url, 
            g_adv_lists.list_15s[i]->ad_type,
            g_adv_lists.list_15s[i]->ad_len);
    }

    for (i = 0; i < g_adv_lists.list_30s_index; i++) {
        printf("%s,%s,%d\n",g_adv_lists.list_30s[i]->ad_url, 
            g_adv_lists.list_30s[i]->ad_type,
            g_adv_lists.list_30s[i]->ad_len);
    }    
}
void create_adv_list(char *list_file)
{
    int fd;
    int c;
    int offset;
    int state;
    adv_item_t *adv_item;
    
    enum {
        sw_start = 0,
        sw_url,
        sw_type,
        sw_len,
        sw_done
    }stbl_state;

    fd = open(list_file, O_RDONLY);
    printf("fd is :%d\n", fd);
    if (fd == -1) {
        printf("open file failed!!!\n");
        return;
    }
    state = sw_start;
    adv_item = NULL;
    while(c = adv_f_get_c(fd)){
        switch (state) {
        case sw_start:
            if (adv_item) {/* 存储上一条广告资源 */
                add_adv_item_to_list(adv_item);
            }
            
            adv_item = (adv_item_t *) malloc(sizeof(adv_item_t));
            if (adv_item == NULL) {
                /* error */
                return;
            }
            memset(adv_item, 0 , sizeof(adv_item_t));
            offset = 0;
            adv_item->ad_url[offset++] = c;
            state = sw_url;
            break;
        case sw_url:
            if (c ==32) {
                state = sw_type;
                offset = 0;
            } else {
                adv_item->ad_url[offset++] = c;
            }
            break;
        case sw_type:
            if (c == 32) {
                state = sw_len;
            } else {
                adv_item->ad_type[offset++] = c;
            }            
            break;
        case sw_len:
            if (c == '\n') {
                state = sw_start;
            }else if (c == '\r') {
                /* do nothing*/
                ;
            } else {
                adv_item->ad_len = adv_item->ad_len * 10 + (c - '0');
            }            
            break;
           
        default:
            break;            
        }
        
    }

    if (adv_item) {/* 存储最后一条广告资源 */
        add_adv_item_to_list(adv_item);
    }
    show_adv_list();
    close(fd);
}

int main()
{
    char * youku_andord_adv;
    int len;
    int i;
    char * str;

    create_adv_list("adv_list.txt");
    
    youku_andord_adv = (char *) malloc(1024);
    if (youku_andord_adv == NULL) {
        return 0;
    }

    memset(youku_andord_adv, 0 , 1024);

    len = make_youku_andord_adv(youku_andord_adv);

#if 1
    for (i = 0; i < len ; i++) {
        printf("%c", youku_andord_adv[i]);
    } 
#endif

    free(youku_andord_adv);
    return 0;
}

