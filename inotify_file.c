/*
 * inotify_file.c
 *  Created on: 2014/07/08
 *      Author: nishi
 *
 *  ./inotify_filse file-path
 *
 *  file-path で指定されたファイルのアクセスをinotify を用いて監視します。
 *  ファイルが削除されても監視を継続できます。
 *
 */
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <errno.h>
#include <limits.h>
#include <sys/inotify.h>

#include <sys/stat.h>
#include <time.h>
#include "com_lib.h"


typedef struct _parm Parm;
struct _parm{
	char *file_path;
	char *dir_s;
	char *file_s;
};

typedef struct _noti_tbl Noti_tbl;
struct _noti_tbl{
	bool isAct;
	bool isMk;
	bool isDel;
	int wd;
	char *name_s;
};

static void display_inotify_event(struct inotify_event *inotify_p);
void check_agrv(int argc, char **argv);

// N_ALL_EVENTS のカスタマイズ
#define IN_ALL_EVENTS_FILE	 (IN_MODIFY | IN_ATTRIB | IN_CLOSE_WRITE  \
			  | IN_CLOSE_NOWRITE | IN_OPEN | IN_MOVED_FROM	      \
			  | IN_MOVED_TO | IN_CREATE | IN_DELETE		      \
			  | IN_DELETE_SELF | IN_MOVE_SELF)
// ディレクトリーの監視マスク
//static uint32_t mask_dir=IN_ALL_EVENTS; /* 全てのイベントを監視 */
static uint32_t mask_dir=IN_CREATE | IN_DELETE;

// ファイルの監視マスク
static uint32_t mask_file=IN_ALL_EVENTS_FILE; /* 全てのイベントを監視 */
//static uint32_t mask_file=IN_MODIFY | IN_DELETE_SELF;

// lsof コマンド
static char lsof_c[]="/usr/sbin/lsof";

static char version[]="1.0";
static char update[]="2014.7.10";

Parm parm;
Noti_tbl noti_tbl[2];

int seq_no;

int main(int argc, char **argv){
    int i;

    printf("inodify_file ver=%s update %s\n",version,update);
	fflush(stdout);

    check_agrv(argc,argv);

    //exit(0);

    printf("start watch dir and file.\n");
    printf("seq date > wd mask cookie len name [event]\n");
    fflush(stdout);

    int fd  = inotify_init();

    if (fd == -1) {
        perror("main() :#1 inotify_init() error");
        exit(EXIT_FAILURE);
    }

    //printf("main() :#1 fd=%d\n",fd);

    /*
     *  監視対象とするディレクトリとファイルを inotify_add_watch に設定します。
     *  ディレクトリは、監視対象ファイルが削除＆作成された時の判定の為に監視します。
     */
    for (i=0 ; i<2 ; i++) {
    	int wd;
    	noti_tbl[i].isMk=false;
    	noti_tbl[i].isDel=false;
    	if(noti_tbl[i].isAct==true){
        	// デイレクトリーの監視
        	if(i==0){
            	wd = inotify_add_watch(fd, parm.dir_s,mask_dir);
        	}
        	// ファイルの監視
        	else{
            	wd = inotify_add_watch(fd, parm.file_path,mask_file);
        	}
        	if (wd < 0) {
        		perror("inotify_add_watch");
        		exit(EXIT_FAILURE);
            }
        	noti_tbl[i].wd=wd;
    	}
    	else{
        	noti_tbl[i].wd=-1;
    	}
    }

    int lng;	// 読み込みサイズ
    int offs=0;	//読み込みオフセット
    char buffer[65536];
    for (seq_no=1;;seq_no++) {
        /*
        *  - fd からは複数のイベントを同時に収集できる。
         *  - 監視対象のディレクトリが複数ある場合はいったんバッファに
         *    受信してから、ユーザ側で区別する。
         *  - バッファサイズはある程度大きくないとエラーとなる。
        *    sizeof(struct inotify_event) を指定して1イベントづつ採る
         *    のは無理。
         *  - バッファサイズが大きくても受信する inotify イベントがなく
         *    なれば返ってくる。
         *  - 最後のイベントは中途半端な状態まで読み込まれることがある。
         *    その場合はイベントの後半部分を読み直す必要がある。
         *    プログラム中では aux を使って調整している。
         */
        /*  buffer の先頭から offs まではすでに読み込んでいる。 */
    	lng = read(fd, buffer + offs, sizeof(buffer) - offs);
		if (lng == -EINTR)
			continue;;
    	if (lng == -1) {
    		perror("main() :#2 read error:");
    		exit(EXIT_FAILURE);
    	}
    	lng += offs;

        /*
         *  バッファサイズが大きいのに最初のイベントが中途半端にしか
         *  読めないならエラーとする。
         */
    	if (lng < sizeof(struct inotify_event)) {
    		fprintf(stderr, "main() :#3 short of red bytes\n");
    		exit(EXIT_FAILURE);
    	}

    	int cur = 0;	//処理中のイベントレコード位置
    	bool isBreak=false;	//イベントレコードの途中切れ=無し
    	while (cur < lng) {
    		struct inotify_event *inotify_p;
    		inotify_p = (struct inotify_event *)(buffer + cur);

    		if (lng < cur + offsetof(struct inotify_event, name)) {
    			/* inotify イベントの固定部分が読み込めていない。 */
    			offs = lng - cur;
    			memmove(buffer, buffer + cur, offs);
    			isBreak=true;	//イベントレコードの途中切れ=有り
    			break;
    		}

    		int size = sizeof(struct inotify_event) + inotify_p->len;
    		if (lng < cur + size) {
    			/* inotify イベントの可変部分が読み込めていない。 */
    			offs = lng - cur;
    			memmove(buffer, buffer + cur, offs);
    			isBreak=true;	//イベントレコードの途中切れ=有り
    			break;
    		}
            /* イベントの解析 and 表示 */
    		display_inotify_event(inotify_p);
    		cur += size;

    		//対象ファイルが削除されました
        	if(noti_tbl[1].isDel == true){
        		//printf("main() :#4 inotify_rm_watch execute fd=%d , noti_tbl[1].wd=%d\n",fd,noti_tbl[1].wd);
        		//削除されたファイルの watch-dog を削除しようとするとエラーになるみたい。
        		if(inotify_rm_watch(fd, noti_tbl[1].wd) == -1){
        			//perror("main() :#5 inotify_rm_watch execute error");
        			//exit(EXIT_FAILURE);
        		}
            	noti_tbl[1].wd = -1;
        		noti_tbl[1].isDel=false;
        	}
        	//対象ファイルが作成されました
        	if(noti_tbl[1].isMk == true){
        		printf("main() :#7 inotify_add_watch execute\n");
            	fflush(stdout);
            	int wd = inotify_add_watch(fd, parm.file_path,mask_file);
            	if (wd < 0) {
            		perror("main() :#8 inotify_add_watch execute error");
            		exit(EXIT_FAILURE);
                }
            	noti_tbl[1].wd=wd;
            	noti_tbl[1].isMk=false;
            	noti_tbl[1].isAct=true;
        	}
    	}
    	//イベントレコードの途中切れ==無し
    	if(isBreak==false){
        	//読み込みオフセットをクリアー
    		offs=0;
    	}
    }
    close(fd);
    return 0;
}

/*
 * static void display_inotify_event(struct inotify_event *inotify_p)
 * イベントの解析 and 表示
 */
static void display_inotify_event(struct inotify_event *inotify_p){
	int ret = 0;
	char buffer[256];
	char wk[128];
	bool isMk=false;
	bool isDel=false;
	bool isModf=false;
    uint32_t mask = inotify_p->mask;

    // File was accessed (read) (*).
    if (mask & IN_ACCESS)
        ret += sprintf(buffer + ret, "ACCESS ");

    // Metadata   changed
    if (mask & IN_ATTRIB)
        ret += sprintf(buffer + ret, "ATTRIB ");

    // File opened for writing was closed (*).
    if (mask & IN_CLOSE_WRITE)
        ret += sprintf(buffer + ret, "CLOSE_WRITE ");

    // File not opened for writing was closed (*).
    if (mask &  IN_CLOSE_NOWRITE)
        ret += sprintf(buffer + ret, "CLOSE_NOWRITE ");

    // File/directory created in watched directory (*).
    if (mask & IN_CREATE){
        ret += sprintf(buffer + ret, "CREATE ");
        isMk=true;
    }

    // File/directory deleted from watched directory (*).
    if (mask & IN_DELETE){
        ret += sprintf(buffer + ret, "DELETE ");
    }

    // Watched file/directory was itself deleted.
    if (mask & IN_DELETE_SELF){
        ret += sprintf(buffer + ret, "DELETE_SELF ");
        isDel=true;
    }
    // File was modified (*).
    if (mask & IN_MODIFY){
        ret += sprintf(buffer + ret, "MODIFY ");
        isModf=true;
    }

    // Watched file/directory was itself moved.
    if (mask & IN_MOVE_SELF)
        ret += sprintf(buffer + ret, "MODIFY_SELF ");

    // File moved out of watched directory (*).
    if (mask & IN_MOVED_FROM)
        ret += sprintf(buffer + ret, "MOVE_FROM ");

    // File moved into watched directory (*).
    if (mask & IN_MOVED_TO)
        ret += sprintf(buffer + ret, "MOVE_TO ");

    // File was opened (*).
    if (mask & IN_OPEN){
        ret += sprintf(buffer + ret, "OPEN ");
        //isModf=true;
    }

    // Watch  was removed explicitly (inotify_rm_watch(2)) or
    // automatically (file was deleted, or  file  system  was
    // unmounted).
    if (mask & IN_IGNORED)
        ret += sprintf(buffer + ret, "IGNORED ");

    // Subject of this event is a directory.
    if (mask & IN_ISDIR)
        ret += sprintf(buffer + ret, "ISDIR ");

    // Event queue overflowed (wd is -1 for this event).
    if (mask & IN_Q_OVERFLOW)
        ret += sprintf(buffer + ret, "Q_OVERFLOW ");

    // File system containing watched object was unmounted.
    if (mask & IN_UNMOUNT)
        ret += sprintf(buffer + ret, "Q_UNMOUNT ");

	// 現在に時刻を取得
	time_t ctm;
	ctm=time(NULL);
	//ctime_r(&ctm,wk);
	struct tm *tm_ptr;
	tm_ptr=localtime(&ctm);
	strftime(wk,sizeof(wk)-1,"%H:%M:%S %d %b",tm_ptr);
	rtrim(wk,0x0a);

    if(inotify_p->len > 0){
        printf("%d %s > %d %x %u %u \"%s\" [%s]\n",
        		seq_no,
        		wk,
        		inotify_p->wd,
        		inotify_p->mask,
        		inotify_p->cookie,
        		inotify_p->len,
               &inotify_p->name,
               buffer);

    }
    else{
        printf("%d %s > %d %x %u %u - [%s]\n",
        		seq_no,
        		wk,
               inotify_p->wd,
               inotify_p->mask,
               inotify_p->cookie,
               inotify_p->len,
               buffer);

    }
	fflush(stdout);

    //デイレクトリーイベントです。
    if(inotify_p->wd == noti_tbl[0].wd){
    	int rc;
    	//ファイルの作成です
    	if(isMk==true){
			if(strcmp(parm.file_s,&(inotify_p->name[0]))==0){
				noti_tbl[1].isMk=true;
			}
    	}
    	//ファイルの削除です
    	//if(isDel==true){
		//	if(strcmp(parm.file_s,&(inotify_p->name[0]))==0){
		//		noti_tbl[1].isDel=true;
		//	}
    	//}
    }
    //ファイルイベントです。
    if(inotify_p->wd == noti_tbl[1].wd){
    	//ファイルの削除です
    	if(isDel==true){
    		noti_tbl[1].isDel=true;
    	}
    	//ファイルの変更です
    	if(isModf==true){
    		// call "lsof file-path"
    		sprintf(wk,"%s %s",lsof_c,parm.file_path);
    		system(wk);
    	}
    }
}
/*
 * void check_agrv(int argc, char **argv)
 */
void check_agrv(int argc, char **argv){
	int i;
    if (argc != 2) {
    	printf("usage: inotify_file file-path\n");
    	fflush(stdout);
    	exit(EXIT_SUCCESS);
    }
    //対象ファイルパスを取得
    parm.file_path=get_strcpy(argv[1]);
    char *slash=strstr_last(parm.file_path,"/");
    if(slash == NULL){
    	parm.dir_s="./";
    	parm.file_s=parm.file_path;
    }
    else{
    	parm.dir_s=get_strncpy(parm.file_path,slash-parm.file_path);
    	parm.file_s=get_strcpy(slash+1);
    }
    printf("-- parm --\nfile_path=%s\nwatch dir(wd=1)=%s\nwatch file(wd>=2)=%s\n",parm.file_path,parm.dir_s,parm.file_s);
	fflush(stdout);

    noti_tbl[0].name_s=parm.dir_s;
    noti_tbl[1].name_s=parm.file_path;

    struct stat sts;
    for(i=0;i<2;i++){
        if ((stat (noti_tbl[i].name_s, &sts)) == -1){
        	printf ("The file %s doesn't exist...\n", noti_tbl[i].name_s);
        	fflush(stdout);
        	noti_tbl[i].isAct=false;
        }
        else{
        	noti_tbl[i].isAct=true;
        }
    }
}
