/*
 * com_lib.c
 *  Created on: 2014/03/16
 *      Author: nishi
 */
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stddef.h>

#include "com_lib.h"

/*
 * char *get_strcpy(char *str)
 * ストリングのメモリーを確保してコピー
 */
char *get_strcpy(char *str){
	char *p=NULL;
	size_t len;
	if(str != NULL){
		len = strlen(str)+1;
		p=malloc(len);
		if(p == NULL){
			printf("get_strcpy():99 malloc() error\n");
		}
		else{
			strcpy(p, str);
		}
	}
	return p;
}
/*
 * char *get_strncpy(char *str,int lng)
 * ストリングのメモリーを確保してコピー
 */
char *get_strncpy(char *str,int lng){
	char *p;
	size_t len;
	len = lng+1;
	//printf("get_strncpy() :#1 lng=%d\n",lng);

	p=malloc(len);
	if(p == NULL){
		printf("get_strncpy():99 malloc() error\n");
	}
	else{
		strncpy(p,str,lng);
		*(p+lng)=0x00;
	}
	return p;
}
/*
 * *get_strcat(char *str1,char *str2)
 * ストリングを結合して、新たなメモリーを確保
 *   str1+str2
 */
char *get_strcat(char *str1,char *str2){
	char *p;
	size_t len;
	len = strlen(str1)+strlen(str2)+1;
	p=malloc(len);
	if(p == NULL){
		printf("get_strcat():99 malloc() error\n");
	}
	else{
		strcpy(p, str1);
		strcat(p, str2);
	}
	return p;
}
/*
 * str_endsWith(char *str1,char *str2)
 * str1 の最後が str2 かチェック
 */
int str_endsWith(char *str1,char *str2){
	int rc=-1;
	char *str1_p,p;
	int str1_l;
	int str2_l;
	if(str1 != NULL && str2 != NULL){
		str1_l=strlen(str1);
		str2_l=strlen(str2);
		if(str1_l >= str2_l){
			str1_p=str1+(str1_l-str2_l);
			rc=strcmp(str1_p,str2);
		}
	}
	return rc;
}
/*
 * str_startsWith(char *str1,char *str2)
 * str1 の先頭が、str2 と同じかチェック
 */
int str_startsWith(char *str1,char *str2){
	int rc=-1;
	int str1_l;
	int str2_l;
	if(str1 != NULL && str2 != NULL){
		str1_l=strlen(str1);
		str2_l=strlen(str2);
		if(str1_l >= str2_l){
			rc=strncmp(str1,str2,str2_l);
		}
	}
	return rc;
}

/*
 * *strstr_last(char *str1,char *str2)
 * str1 の中の最後の str2 の位置を検索
 * みつから無いときは、 NULL
 */
char *strstr_last(char *str1,char *str2){
	int lng;
	char *p1,*p2;
	p2=p1=str1;
	while(1){
		p1 = strstr(p1, str2);
		if(p1 == NULL){
			break;
		}
		p2=p1;
		p1=p1+strlen(str2);
	}
	if(str1 == p2){
		p2=NULL;
	}
	return p2;
}

/*
 * char* rtrim(char* string, char junk)
 */
char* rtrim(char* string, char junk){
	char* original = string + strlen(string);
	while(*--original == junk);
	*(original + 1) = '\0';
	return string;
}
/*
 * char* ltrim(char *string, char junk)
 */
char* ltrim(char *string, char junk){
	char* original = string;
	char *p = original;
	int trimmed = 0;
	do{
		if (*original != junk || trimmed){
			trimmed = 1;
			*p++ = *original;
		}
	}
	while (*original++ != '\0');
	return string;
}
/*
 * memdump(char *dt,int lng)
 */
void memdump(char *dt,int lng){
	int data;
	int i1;
	unsigned long addr;
	char buff[100];
	if(lng >0){
		for(addr = 0; ; addr += 16){
			printf("%08lX  ", addr);
			for(i1 = 0; i1 < 16; i1++){
				if(lng <=0 ){
					buff[i1] = '\0';
					for(;i1 < 16; i1++) printf("   ");
					printf("  %s\n", buff);
					return;
				}
				data=*dt;
				lng--;
				dt++;
				if(data < 0x20 || data >= 0x7F)
					buff[i1] = '.';
				else
					buff[i1] = data;
				printf("%02X ", data);
			}
			buff[i1] = '\0';
			printf("  %s\n", buff);
		}
	}
}
/*
 * Msplit_list *msplit(char *str, char *sep)
 * strtokと同じ引数をとる
 */
Msplit_list *msplit(char *str, char *sep){
	char *p,*cu_p;
	int i;
	Msplit_list *mp;
	mp=malloc(sizeof(Msplit_list));
	if(mp == NULL){
		printf("msplit():99 malloc() error\n");
	}
	else{
		mp->a_cnt=0;
		mp->cur_n=0;
		cu_p=str;
		while(1){
			p = strstr(cu_p, sep);
			if(p != NULL){
				i=p - cu_p;
				if(i > 0){
					mp->mlist[mp->a_cnt]=get_strncpy(cu_p,i);
					cu_p = cu_p+i+strlen(sep);
					mp->a_cnt++;
				}
			}
			else{
				if(strlen(cu_p)>0){
					mp->mlist[mp->a_cnt]=get_strcpy(cu_p);
					mp->a_cnt++;
				}
				break;
			}
			if(mp->a_cnt >= MSPLIT_MAX_LIST){
				break;
			}
		}
	}
	return mp;
}
/*
 * char *split_pop(Split_list *psplit_list)
 *  psplit_list->cur_n で示される string を返す。
 */
char *msplit_pop(Msplit_list *pmsplit_list){
	char *p=NULL;
	if(pmsplit_list->cur_n < pmsplit_list->a_cnt){
		p=pmsplit_list->mlist[pmsplit_list->cur_n];
		pmsplit_list->cur_n++;
	}
	return p;
}
/*
 * void split_free(Split_list *psplit_list)
 * splitで作成した配列のメモリを解放する
 */
void msplit_free(Msplit_list *pmsplit_list){
	int i;
	for(i=0;i < pmsplit_list->a_cnt;i++){
		free(pmsplit_list->mlist[i]);
	}
	free(pmsplit_list);
}
// strtokと同じ引数をとる
// 文字列さすポインタを要素とする配列を返す
char **split_not_use(char *str, char *sep){
    char *word = NULL;
    char *str_work = NULL;
    int last = 0;

    //うけとった文字列のコピーを作成
    str_work = (char *)malloc(sizeof(char) * (strlen(str) + 1));
    strcpy(str_work, str);

    //文字列のポインタを保持する配列
    char **results = (char **)malloc(sizeof(char *) * SPLIT_MAX_WORD);

    //strtokで得たトークンのポインタを配列に代入
    last = 0;
    for(word = strtok(str_work,sep); word; word = strtok(NULL,sep),last++) {
        results[last] = word;
    }
    results[last] = NULL;

    return results;
}

// splitで作成した配列のメモリを解放する
void split_free_not_use(char **words){
    //words[0]をfreeすればすべてのトークンのメモリは解放される?
    free(words[0]);
    free(words);
}
