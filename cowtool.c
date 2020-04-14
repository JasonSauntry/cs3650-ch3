#include <ctype.h>
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

#define VERSION_KEPT 16

slist* build_ls_tree(slist* list, const char* dir) {
	slist* contents = storage_list(dir);
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
			printf("Version for %s: \n", img);
			printf(" + root_inode: %d\n", get_root_inum());
			super_block* block = get_super();
			int index = block->most_recent_version;
			int end = (index + 1) % VERSIONS_KEPT;

			for (int i = index; i != end && i >= 0; i--){
				version v = block->versions[i % VERSIONS_KEPT];
				printf("%d %s %s\n", v.version_number, v.trigger, v.path);
			}	
			return 0;
		}

		if (streq(cmd, "rollback") && argc == 4 && isdigit(atoi(argv[3]))) {
			int v_req = atoi(argv[3]);
			printf("Rollback %s to version %d\n", img, v_req);
			super_rollback(v_req);
			return 0;
		}

    print_usage(argv[0]);
}
