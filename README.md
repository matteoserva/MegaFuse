MegaFuse
========

Fuse module for the MEGA cloud storage provider (mega.co.nz) in c++
All files are downloaded only when required, and data can be read as soon as it is available.
The dowloader will assign a higher priority to the requested chunk and then prefetch the remaining part of the file.
This allows fast streaming of any kind of video file without prior encoding.
A local cache is maintained to speedup processing.


please edit your config file "megafuse.conf" before running.

to run,just

	make
	./MegaFuse

to compile on debian 7 you need these additional packages:
	
	apt-get install libcrypto++ libcurl4-openssl-dev libdb5.1++-dev libfreeimage-dev 

you can pass additional options to the fuse module via the command line option -f. example:
	
	MegaFuse -f -o allow_other -o uid=1000

after an abnormal termination you might need to clear the mountpoint:
	
	$ fusermount -u $MOUNTPOINT
	or # umount $MOUNTPOINT

I'm currently accepting donations via paypal at the address of my main project

	http://ygopro.it/web/modules.php?name=Donations&op=make
