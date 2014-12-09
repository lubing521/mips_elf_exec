/*
 * Copyright(C) 2014 Ruijie Network. All rights reserved.
 */
/*
 * yk_player_plugin.c
 * Original Author: yaoshangping@ruijie.com.cn, 2014-03-08
 *
 * Parse and build Youku video url, codes are decompiled from video player plug-in of Youku.
 * Keep the code's original structure as far as possible.
 *
 * History:
 *  v0.9    yaoshangping@ruijie.com.cn   2014-03-08
 *          创建此文件。
 *  v1.0    zhaoyao@ruijie.com.cn        2014-04-28
 *          修订文件，去掉不需要的部分。
 *
 */

#include <sys/time.h>
#include "yk_lib.h"

/* *** getplaylist *** */

#define YK_MAX_NAME_LEN         32
#define YK_PLAYLIST_VAR_LEN     10
#define YK_URL_VAR_PASSWD_LEN   128

/* url中所需的基础数据结构 */
typedef struct yk_root_data_s {
	char video_id[YK_MAX_NAME_LEN];	
	char fid[YK_MAX_NAME_LEN];	
	char passwords[YK_MAX_NAME_LEN];	
	char partnerId[YK_MAX_NAME_LEN];
	char extstring[YK_MAX_NAME_LEN];
	char source[YK_PLAYLIST_VAR_LEN];
	char version[YK_PLAYLIST_VAR_LEN];
	char type[YK_PLAYLIST_VAR_LEN];
	char pt[YK_PLAYLIST_VAR_LEN];
	char ob[YK_PLAYLIST_VAR_LEN];
	char pf[YK_PLAYLIST_VAR_LEN];
	bool isRedirect;
	
	/* 内容可扩展 */
} yk_root_data_t;

/* url中的可选参数 */
typedef struct yk_url_var_s {
	int n;
	char ctype[YK_PLAYLIST_VAR_LEN];
	unsigned int ran;
	int ev;
	bool isRedirect;
	char passwords[YK_URL_VAR_PASSWD_LEN];
	char client_id[YK_MAX_NAME_LEN];
	char extstring[YK_MAX_NAME_LEN];

} yk_url_var_t;

typedef struct yk_player_cfg_s {
    bool isFDL;     /* true: 和k.youku.com 有关系，具体含义未知 */ 
    bool isTudouPlayer;
} yk_player_cfg_t;

typedef struct yk_player_cons_s {
    char ctype[YK_PLAYLIST_VAR_LEN]; /* "standard", "story" , 具体含义未知 */
    int ev;         /* 具体含义未知 */             
} yk_player_cons_t;

/************************************************ 
* 把字符串进行URL编码。 
* 输入： 
* str: 要编码的字符串 
* strSize: 字符串的长度。这样str中可以是二进制数据 
* result: 结果缓冲区的地址 
* resultSize:结果地址的缓冲区大小(如果str所有字符都编码，该值为strSize*3) 
* 返回值： 
* >0: result中实际有效的字符长度， 
* 0: 编码失败，原因是结果缓冲区result的长度太小 
************************************************/ 
int url_encode(const char* str, const int strSize, char* result, const int resultSize) 
{ 
    int i; 
    int j = 0; /* for result index */ 
    char ch; 

    if ((str == NULL) || (result == NULL) || (strSize <= 0) || (resultSize <= 0)) { 
        return 0; 
    } 

    for (i=0; (i<strSize) && (j<resultSize); i++) { 
        ch = str[i]; 
        if ((ch >= 'A') && (ch <= 'Z')) { 
            result[j++] = ch; 
        } else if ((ch >= 'a') && (ch <= 'z')) { 
            result[j++] = ch; 
        } else if ((ch >= '0') && (ch <= '9')) { 
            result[j++] = ch; 
        } else if(ch == ' '){ 
            result[j++] = '+'; 
        } else { 
            if (j + 3 < resultSize) { 
                sprintf(result+j, "%%%02X", (unsigned char)ch); 
                j += 3; 
            } else { 
                return 0; 
            } 
        } 
    } 

    result[j] = '\0'; 
    return j; 
} 

/************************************************ 
* 把字符串进行URL解码。 
* 输入： 
* str: 要解码的字符串 
* strSize: 字符串的长度。 
* result: 结果缓冲区的地址 
* resultSize:结果地址的缓冲区大小，可以<=strSize 
* 返回值： 
* >0: result中实际有效的字符长度， 
* 0: 解码失败，原因是结果缓冲区result的长度太小 
************************************************/ 
int url_decode(const char* str, const int strSize, char* result, const int resultSize) 
{ 
    char ch, ch1, ch2; 
    int i; 
    int j = 0; /* for result index */ 

    if ((str == NULL) || (result == NULL) || (strSize <= 0) || (resultSize <= 0)) { 
        return 0; 
    } 

    for (i=0; (i<strSize) && (j<resultSize); i++) { 
        ch = str[i]; 
        switch (ch) { 
        case '+': 
            result[j++] = ' '; 
            break; 

        case '%': 
            if (i+2 < strSize) { 
                ch1 = (str[i+1])- '0'; 
                ch2 = (str[i+2]) - '0'; 
                if ((ch1 != '0') && (ch2 != '0')) { 
                result[j++] = (char)((ch1<<4) | ch2); 
                i += 2; 
                break; 
            } 
        } 

        /* goto default */ 
        default: 
            result[j++] = ch; 
            break; 
        } 
    } 

    result[j] = '\0'; 
    return j; 
} 

/* 识别优酷视频首页的url，正确返回0，反之返回-1 */
/* 入参:url */
int identify_yk_video_url(char *purl)
{
    char *obj_url="v.youku.com/v_show";
    char *presult = NULL;
    if (purl == NULL) {
        return -1;
    }
    
    presult=strstr(purl, obj_url);
    if (presult== purl) {
        return 0;
    } else {
        return -1;
    }
}

/* 从优酷视频首页的html中获取视频的name和folderId, url一共有如下三种形式 */
/* v.youku.com/v_show/id_XNjY2OTkyNjk2.html */
/* v.youku.com/v_show/id_XNjcxNTUzNjIw.html?f=21894121&ev=4 */
/* v.youku.com/v_show/id_XNjcxNTUzNjIw.html?f=21894122 */
int get_yk_video_name(char *purl, char name[], char folderId[])
{

    char *purl_pre = "v.youku.com/v_show/id_";
    char *name_idx;
	char *folder_idx;
    int i,url_pre_len;

    if (purl == NULL) {
        return 0;
    }

    name_idx = strstr(purl, purl_pre);    
    if (name_idx == NULL) {
        return 0;
    } else {        
        i = 0;
		url_pre_len =strlen(purl_pre);
        for (name_idx = name_idx + url_pre_len;*name_idx != '.' && i < YK_MAX_NAME_LEN; name_idx++) {
            name[i++] = *name_idx;
        }

        name[i] = '\0';
		if ((url_pre_len + i + 5) == strlen(purl)) {
			/* purl里面不带?f= */
			return 1;
		} else {
			/* .html?f= */
			folder_idx = name_idx + 8;
			i = 0;
			for ( ; *folder_idx != '\0' && *folder_idx != '&'; folder_idx++) {
				folderId[i++] = *folder_idx;
			}
			folderId[i] = '\0';
			return 2;
		}
    }
}    

/* getPlayList的请求报文 */
int request_playlist(yk_root_data_t *rootdata, yk_url_var_t *url_var, 
                     yk_player_cfg_t *player_cfg, yk_player_cons_t *player_cons,
                     char *url)
{
	char *time_zone = "+08";
	char *server_domain = "v.youku.com";
	int is_url_encode_ok;
    char int2str[32];   /* 临时存储int转换的字符串 */

	if (rootdata == NULL || url_var == NULL
        || player_cfg == NULL || player_cons == NULL
        || url == NULL) {
		return -1;
	}	

	strcat(url, server_domain);//拼接字符串
	strcat(url, "/player/getPlayList");
	strcat(url, "/VideoIDS/");
	strcat(url, rootdata->video_id);
	strcat(url, "/timezone/");
	strcat(url, time_zone);
	strcat(url, "/version/");
	strcat(url, rootdata->version);
	strcat(url, "/source/");
	strcat(url, rootdata->source);
	if (rootdata->type[0]!= '\0') {
		strcat(url, "/Type/");
		strcat(url, rootdata->type);
	}

	if (strcmp(rootdata->type, "Folder") == 0) {
		strcat(url, "/Fid/");
		strcat(url, rootdata->fid);
		strcat(url, "/Pt/");
		strcat(url, rootdata->pt);
		if (rootdata->ob[0]!= '\0') {
			strcat(url, "/Ob/");
			strcat(url, rootdata->ob);
		}
	}

	if (rootdata->pf[0] != '\0') {
		strcat(url, "/Pf/");
		strcat(url, rootdata->pf);
	}

	if (player_cfg->isTudouPlayer) {
		strcat(url, "/Sc/");
		strcat(url, "2");
	}

	strcat(url, "?password=");
	if (rootdata->passwords[0]!= '\0') {
		is_url_encode_ok = url_encode(rootdata->passwords, strlen(rootdata->passwords), 
										url_var->passwords, sizeof(url_var->passwords));
		if (is_url_encode_ok) {
			strcat(url, url_var->passwords);
		}
	}

    strcat(url, "&n=");
    sprintf(int2str, "%d", url_var->n);
    strcat(url, int2str);
    strcat(url, "&ran=");
    sprintf(int2str, "%d", url_var->ran);
    strcat(url, int2str);

    if (player_cfg->isFDL) {
        strcat(url, "&ctype=");
        strcat(url, player_cons->ctype);
        strcat(url, "&ev=");
        sprintf(int2str, "%d", player_cons->ev);
        strcat(url, int2str);        
    }

	return 0;
}

/* 入参: 视频首页的url */
/* 返回生成的request playlist的url */
/*
 * XXX FIXME: 实际上playlist可以直接用下面的pattern生成(2014-05-07)
 *            v.youku.com/player/getPlayList/VideoIDS/XNjg1MDk0MDQ0
 */
int yk_url_to_playlist(char *url, char *playlist)
{
	bool video_url;
	char video_name[YK_MAX_NAME_LEN];
	char folderId[YK_MAX_NAME_LEN];
	int  ret_get_vname;

    yk_root_data_t rootdata;
	yk_url_var_t url_variables;
    yk_player_cfg_t player_cfg;
    yk_player_cons_t player_cons;    
    
	char passwords[] = "";
	char version[] = "5";
	char source[] = "video";
	char type[] = "Folder";
	char ob[] = "1";
	char pt[] = "0";
	char ctype[] = "10";   

    if (url == NULL || playlist == NULL) {
        return -1;
    }

	video_url = identify_yk_video_url(url);
	if (video_url== 0) {
		memset(video_name, 0, sizeof(video_name));
		memset(folderId, 0, sizeof(folderId));
        memset(&rootdata, 0, sizeof(rootdata));
        memset(&url_variables, 0, sizeof(url_variables));
        memset(&player_cfg, 0, sizeof(player_cfg));
        memset(&player_cons, 0, sizeof(player_cons));
        
		ret_get_vname = get_yk_video_name(url, video_name, folderId);
        if (ret_get_vname) {
            memcpy(rootdata.video_id, video_name, strlen(video_name));                
            /* 构造rootdata */
        	memcpy(rootdata.ob, ob, sizeof(ob));
        	memcpy(rootdata.pt, pt, sizeof(pt));
        	memcpy(rootdata.passwords, passwords, sizeof(passwords));	
        	memcpy(rootdata.version, version, sizeof(version));
        	memcpy(rootdata.source, source, sizeof(source));
            if (ret_get_vname == 2) {
                memcpy(rootdata.fid, folderId, strlen(folderId));
                memcpy(rootdata.type, type, sizeof(type));
            }

            /* 生成getplaylist 的请求 */
            player_cfg.isTudouPlayer = false;
            player_cfg.isFDL = true;
            player_cons.ev = 1;
			memcpy(player_cons.ctype, ctype, sizeof(ctype));
            /* 构造url的可选参数 */
        	url_variables.n = 3;
        	srandom(123);
        	url_variables.ran = random() % 10000;	
            return request_playlist(&rootdata, &url_variables, &player_cfg, &player_cons, playlist);
        	
        } else {
            return -1;
        }
	}	else {
	    return -1;
	}
}

/* *** getfileid *** */

/* 返回所在时区的本地时间中的年份 */
static int randomB() 
{
    int ret;
    time_t t;
    struct tm area;

    bzero(&area, sizeof(area));
    tzset();
    t = time32();
    localtime_r(&t, &area);

    ret = area.tm_year + 1900;

	return ret;
}

/* 根据优酷视频播放插件的反编译结果观察，该函数的返回值直接为2 */
static int randomFT()
{
    int _loc2_;

    _loc2_ = 2;

	return _loc2_;
}

/* 计算dt,具体含义不知道 */
static double zf(int *dt)
{
	double ret;

    *dt = ((*dt) * 211 + 30031) & 0xFFFF;   /* 这些值直接来自优酷视频播放插件的反编译结果 */
	ret = *dt;
	ret = ret / 0x10000;

    return ret;
}

/* @streamfileids: 服务器返回的streamfileids, "40*32*...*48*54*", 用于定位字符的位置 
 * @processed_str : 经过ranom8T加工过的字符串 */
/* @output_str: 返回提取的字符串, 注意还不是最终的fileids */
static int cg_fun(char *streamfileids, char *processed_str, char *output_str) 
{
    int _loc2_[YK_FILEID_LEN];
    int _loc4_;
    char *delim = "*";
    char *pstr;
    char sfileid[YK_STREAM_FILE_IDS_LEN];
    int i;

    if (streamfileids == NULL || 
        processed_str == NULL || 
        output_str == NULL) {
        return -1;
    }

    memset(_loc2_, 0, sizeof(_loc2_));
	memset(sfileid, 0, sizeof(sfileid));
    if (strlen(streamfileids) >= YK_STREAM_FILE_IDS_LEN) {
        return -1;
    }
    strcpy(sfileid, streamfileids);
    
    pstr = strtok(sfileid, delim);
    i = 0;
    /* 将下标转换成数字 */
    _loc2_[i++] = atoi(pstr);
    while ((pstr = strtok(NULL, delim)) && i < YK_FILEID_LEN) {
        _loc2_[i++] = atoi(pstr);
    }

    _loc4_ = 0;
    while (_loc4_ < YK_FILEID_LEN) {
        output_str[_loc4_] = *(processed_str + _loc2_[_loc4_]);
        _loc4_++;
    }

    output_str[_loc4_] = '\0';

    return 0;
}

/* 删除字符串中，指定的字符
 * @input, 原始字符串
 * @key, 要删除的字符
 */
static void del_key_in_str(char *input, char key)
{
    int i;

    if (input == NULL) {
        return;
    }
    
    i = 0;
    while (input[i] != key && input[i] != '\0') {
        i++;
    }

    while (input[i] != '\0') {
        input[i] = input[i+1];
        i++;
    }

    input[i] = '\0';    
    return;
}

/* 返回 output_str */
static int ranom8T(char *output_str, int seed)
{
    char _loc2_[YK_FILEID_LEN + 4]; /* 能存放可能多出的字符 */
    int _loc1_;
    int _loc4_;
    int _loc5_;
    int _loc6_;
    int _loc7_;
    char *alphabet = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
    char *symbol_number = "/\\:._-1234567890";

    if (output_str == NULL) {
        return -1;
    }
   
    _loc1_ = randomFT(); //返回值: 2    
    _loc1_ = _loc1_ + randomB(); //返回年: 2014
    memset(_loc2_, 0, sizeof(_loc2_));
    strcat(_loc2_, alphabet);
    strcat(_loc2_, symbol_number);         
    /* abc...xyzABC...XYZ/\:._-123...890 */
    _loc4_ = strlen(_loc2_); 
    _loc5_ = 0;
    _loc7_ = 0;
    while((_loc5_ < _loc4_) && (_loc5_ < (YK_FILEID_LEN + 2))) {
        _loc7_ = (int)(zf(&seed) * strlen(_loc2_));
        if (_loc7_ >= (YK_FILEID_LEN + 4)) {
            return -1;
        }
        output_str[_loc5_] = _loc2_[_loc7_];
        del_key_in_str(_loc2_, _loc2_[_loc7_]);		
        _loc5_++;
    }
    
    _loc6_ = randomFT(); //返回值: 2
    _loc1_ = _loc6_;
    return 0;
}

/**
 * NAME: yk_get_fileid
 *
 * DESCRIPTION:
 *      计算视频文件名。
 * 在此，提供一组测试数据:
 * "40*32*40*40*40*12*40*17*40*40*44*12*31*2*64*49*44*44*45*2*40*45*40*40*64*45*1*18*32*12*48*12*40*
 *  12*32*2*18*12*39*48*44*44*49*39*44*31*64*48*39*54*48*32*54*39*1*2*2*49*2*35*44*31*64*1*48*54*"
 * video_num = 0
 * seed = 1599
 * fileids: 030002010052F96855C90C006CBE32420239E2-4558-5F64-A43A-B998975F6B4A
 *
 * @streamfileids:  -IN  各类视频的streamfileids。
 * @video_num:      -IN  该视频的分段号。
 * @seed:           -IN  服务器给出的随机数。
 * @fileids:        -OUT 返回视频文件名,一共66个字符(不包括结束符'\0')。
 *
 * RETURN: -1表示失败，0表示成功。
 */
int yk_get_fileid(char *streamfileids, int video_num, int seed, char *fileids) 
{
	char vnum_str[3]; 
	char processed_str[YK_FILEID_LEN + 2];
	char output_str[YK_FILEID_LEN + 1];
	int ret = -1;

	if (streamfileids == NULL || fileids == NULL) {
		return -1;
	}
	
	memset(processed_str, 0, sizeof(processed_str));
	ret = ranom8T(processed_str, seed);
	if (ret != 0) {
		return -1;
	}
	
	memset(output_str, 0, sizeof(output_str));
	ret = cg_fun(streamfileids, processed_str, output_str) ;
	if (ret != 0) {
		return -1;
	} else {
		/* 将分段号转换为16进制 */
		sprintf(vnum_str, "%X", video_num);     /* MUST be upper case */
		if (strlen(vnum_str) == 1) {
			vnum_str[1] = vnum_str[0];
			vnum_str[0] = '0';
		}
		
		/* output_str的第9、10个字符替换掉 */
		output_str[8] = vnum_str[0];
		output_str[9] = vnum_str[1];
		memcpy(fileids, output_str, strlen(output_str));
		return 0;
	}
}

/* *** getflvpath *** */

/**
 * NAME: yk_get_fileurl
 *
 * DESCRIPTION:
 *      组装优酷的flvpath，即fileurl。
 *
 * @num:            -IN  号数，目前都为0。
 * @play_list       -IN  play_list信息。
 * @seg_data        -IN  分段信息。
 * @use_jumptime    -IN  是否使用jumptime。
 * @jump_time       -IN  jumptime的值。
 * @out_url         -OUT 返回组装好的url。
 *
 * RETURN: -1表示失败，0表示成功。
 */
int yk_get_fileurl(int num, yk_playlist_data_t *play_list, yk_video_seg_data_t *seg_data, 
                    bool use_jumptime, int jump_time, char *out_url)
{
	bool isFDL = false; /* 含义未知，true时，访问k.youku.com */
    char _loc7_[3];     /* 临时变量 */
    char _loc8_[4];     /* 临时变量 */
    char int2str[32];   /* 临时存储int转换的字符串 */
    int nlen = 0;
    
	char host1[] = "k.youku.com/player/getFlvPath";
	char host2[] = "f.youku.com/player/getFlvPath";

    if (play_list == NULL || seg_data == NULL || out_url == NULL) {
        return -1;
    }

	memset(_loc7_, 0, sizeof(_loc7_));
    sprintf(_loc7_, "%x", num);
    if (strlen(_loc7_) == 1) {
        _loc7_[1] = _loc7_[0];
        _loc7_[0] = '0';
    }

    nlen = strlen(host1)
            + YK_PLAYLIST_SID_LEN
            + sizeof(_loc7_)
            + sizeof(_loc8_)
            + strlen(seg_data->fileId)
            + YK_PLAYLIST_KEY_LEN * 3;
    if (nlen >= HTTP_SP_URL_LEN_MAX) {
        /* XXX: url长度为HTTP_SP_URL_LEN_MAX，长度不够 */
        return -1;
    }
    bzero(out_url, HTTP_SP_URL_LEN_MAX);
    if (isFDL) {
        strcat(out_url, host1);
    } else {
        strcat(out_url, host2);
    }

    strcat(out_url, "/sid/");
    strcat(out_url, play_list->sid);
    strcat(out_url, "_");
    strcat(out_url, _loc7_);
    memset(_loc8_, 0, sizeof(_loc8_));
    strcpy(_loc8_, play_list->fileType);
    if ((strcmp(play_list->fileType, "hd2") == 0) || 
        (strcmp(play_list->fileType, "hd3") == 0)) {
        memset(_loc8_, 0, sizeof(_loc8_));
        memcpy(_loc8_, "flv", 3);
    }

    if(play_list->drm)
    {
        memset(_loc8_, 0, sizeof(_loc8_));
        memcpy(_loc8_, "f4v", 3);   
    }

    strcat(out_url, "/st/");
    strcat(out_url, _loc8_);
    strcat(out_url, "/fileid/");
    strcat(out_url, seg_data->fileId);
    if (use_jumptime) {
        if (play_list->drm) {
            strcat(out_url, "?K=");            
        } else {
            strcat(out_url, "?start=");
            sprintf(int2str, "%d", jump_time);
            strcat(out_url, "&K=");
        }
    } else {
        strcat(out_url, "?K=");        
    }

    if (seg_data->key[0] == '\0') {
        strcat(out_url, play_list->key2);
        strcat(out_url, play_list->key1);
    } else {
        strcat(out_url, seg_data->key); 
    }

    /* 以下是可选的参数 */
    #if 0

    if ((strcmp(play_list->fileType, "flv") == 0) ||
        (strcmp(play_list->fileType, "flvhd") == 0)) {
        strcat(out_url, "&hd=0"); 
    } else {
        if (strcmp(play_list->fileType, "mp4") == 0) {
            strcat(out_url, "&hd=1");
        } else {
            if (strcmp(play_list->fileType, "hd2") == 0) {
                strcat(out_url, "&hd=2");
            } else if (strcmp(play_list->fileType, "hd3") == 0) {
                strcat(out_url, "&hd=3");
            }
        }
    }
    #endif

	return 0;
}

