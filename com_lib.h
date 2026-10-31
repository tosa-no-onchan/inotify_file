/*
 * com_lib.h
 *
 *  Created on: 2014/03/16
 *      Author: nishi
 */

#ifndef COM_LIB_H_
#define COM_LIB_H_

#define MSPLIT_MAX_LIST 20

typedef struct _msplit_list Msplit_list;
struct _msplit_list{
	int cur_n;	// カンレンと POP数
	int a_cnt;	// アロケート件数
	char *mlist[MSPLIT_MAX_LIST];
};

char *get_strcpy(char *str);
char *get_strncpy(char *str,int lng);
char *get_strcat(char *str1,char *str2);
int str_endsWith(char *str1,char *str2);
int str_startsWith(char *str1,char *str2);
char *strstr_last(char *str1,char *str2);

char* rtrim(char* string, char junk);
char* ltrim(char *string, char junk);
void memdump(char *dt,int lng);


Msplit_list *msplit(char *str, char *sep);
void msplit_free(Msplit_list *pmsplit_list);
char *msplit_pop(Msplit_list *pmsplit_list);


#define SPLIT_MAX_STR 256
#define SPLIT_MAX_WORD 128

char **split_not_use(char *str, char *sep);
void split_free_not_use(char **words);

#endif /* COM_LIB_H_ */
