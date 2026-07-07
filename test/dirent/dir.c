#include <dirent.h>
#include <sys/types.h>
#include <stdio.h>

int
main()
{
	DIR * bkl_option = opendir("/sys/class/backlight");
	struct dirent * entry = NULL;

	while (entry = readdir(bkl_option)) {
		printf("%s\n", entry->d_name);
	}

	closedir(bkl_option);
}
