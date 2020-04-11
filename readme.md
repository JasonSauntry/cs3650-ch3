
File System Layout:

*	1MB = 256 pages (4k blocks)
*	4096 byte pages
*	Page0: super block, including bitmaps
*	Page1 - n: inodes
*	Rest of pages: Data
*	256 inodes

#### COW:
*	Add a thing in every inode that is a list of incoming hard links.
*	Every `inode` has  version number. On write, copy iff version number is
	older than global version number. Inc global version only on callback from
	Fuse.

## Credit

Used throughout:
*	Insights from [Geoff Kuenning](https://www.cs.hmc.edu/~geoff/classes/hmc.cs135.201001/homework/fuse/fuse_doc.html 3rd-party FUSE documentation)
*	Linux Programmer's Manual
