Welcom to inotify_file.

This progarm is watch-dog for a file access.

./inotify_file file-path

example
./inotify_file /var/log/vsftpd.log

It starts watch dog both /var/log/vsftpd.log and /var/log/ dir.
Although /var/log/vsftpd.log will be removed,it keeps watching /var/log dir.
and if /var/log/vsftpd.log will be created again,it keeps on watching /var/log/vsftpd.log again.


first release
Version 1.0 update 2014.7.10

How to make
1] Download , copy and spread tar.gz file.
 1) Download inotify_file.yyyy.m.d.tar.gz file.
 2) spread the tar.gz. 
  $tar zxvf inotify_file.yyyy.m.d.tar.gz
 3) cd inotify_file
  $./make

2] start command
./inotify_file  file-path

or excute as a deamon.
change the inotify_file_d shell script as you like.
exp.
WATCH_F=/var/log/vsftpd.log <- watch file path


start,stop deamon
./inotify_file_d start|stop
