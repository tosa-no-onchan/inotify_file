all: inotify_file.c com_lib.c com_lib.h
	gcc -I. -g -o inotify_file inotify_file.c com_lib.c