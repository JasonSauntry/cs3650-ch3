
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>

#include "storage.h"
#include "slist.h"
#include "util.h"
#include "directory.h"
#include "inode.h"
#include "super.h"

slist* build_ls_tree(slist* list, const char* dir) {
	slist* contents = storage_list(dir, 0);
	for (slist* xs = contents; xs; xs = xs->next) {
		const size_t size = 256;
		char* text = alloca(size);
		strcpy(text, dir);
		if (!streq("/", dir)) {
			strncat(text, "/", size);
		}
		strncat(text, xs->data, size);
		strcpy(xs->data, text + strlen(dir) + 1);
		list = s_cons(text, list);

		int datanode = tree_lookup(text);
		inode* node = get_inode(datanode);
		if (node->directory) {
			list = build_ls_tree(list, text);
		}
	}
	return list;
}

slist*
image_ls_tree(const char* base)
{
	return build_ls_tree(0, base);
}

void
print_usage(const char* name)
{
	fprintf(stderr, "Usage: %s cmd ...\n", name);
	exit(1);
}

int versions() {
	super_block* super = get_super();
	version* versions = super->versions;
	for (int i = super->most_recent_version; i >= 0; i--) {
		int index = i % VERSIONS_KEPT;
		int named_version = versions[index].version_number;
		if (named_version != i) {
			break;
		}

		printf("%d %s\n", i, versions[index].trigger);
	}

	return 0;
}
		
void rollback(int vnum) {
	super_block* super = get_super();
	super->most_recent_version = vnum;
}

int
main(int argc, char* argv[])
{
	if (argc < 3) {
		print_usage(argv[0]);
	}

	const char* cmd = argv[1];
	const char* img = argv[2];

	if (streq(cmd, "new")) {
		assert(argc == 3);

		storage_init(img, 1);
		printf("Created disk image: %s\n", img);
		return 0;
	}

	if (access(img, R_OK) == -1) {
		fprintf(stderr, "No such image: %s\n", img);
		return 1;
	}

	storage_init(img, 0);

	if (streq(cmd, "ls")) {
		slist* xs = image_ls_tree("/");
		for (slist* it = xs; it != 0; it = it->next) {
			//printf("%s\n", it->data);
		puts(it->data);
		}
		s_free(xs);
		return 0;
	}

	if (streq(cmd, "versions")) {
		printf("Versions for %s:\n", img);
		return versions();
	}

	if (streq(cmd, "rollback")) {
		if (argc < 4) {
			print_usage(argv[0]);
			return 0;
		}

		int vnum = atoi(argv[3]);
		printf("Rollback %s to version %d\n", img, vnum);
		rollback(vnum);
		return 0;
	}

	print_usage(argv[0]);
}
