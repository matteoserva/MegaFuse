MegaFuse
========

Fuse module for the MEGA cloud storage provider (mega.co.nz) in c++

please edit your config file "megafuse.conf" before running.

to run,just

	make
	./Debug/MegaFuse

to compile on debian 7 you need these additional packages:
	
	apt-get install libcrypto++ libcurl4-openssl-dev libdb5.1++-dev libfreeimage-dev 

you can pass additional options to the fuse module via the command line option -f. example:
	
	MegaFuse -f -o allow_other -o uid=1000

this software saves its cache in ~/.megaclient/,
after an abnormal termination you might need to clear the cache and the mountpoint:

	rm -rf ~/.megaclient
	umount $MOUNTPOINT

I'm currently accepting donations via paypal at the address of my main project

	http://ygopro.it/web/modules.php?name=Donations&op=make
