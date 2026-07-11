#include <string.h>
#include <stdio.h>

enum wb_mod_type {
	WB_MOD_INT,
	WB_MOD_LL,
	WB_MOD_BOOL,
	WB_MOD_FLOAT,
	WB_MOD_STRING,
	WB_MOD_CUSTOM
};

int wb_mod_sub_text(const char * format, const char * label, char * result,
				const void * target, int type, int length);

int
main()
{
	const char * format = "Bat {bat}% Brightness {bkl}%";
	char buffer[128];
	int target = 2;

	int res = wb_mod_sub_text(format, "bat", buffer, &target, WB_MOD_INT, sizeof(buffer));
	if (res < 0) {
		printf("gagal wak\n");
		return -1;
	}

	printf("%s\n", buffer);

	return 0;
}

int
wb_mod_sub_text(const char * format, const char * label, char * result,
				const void * target, int type, int length)
{
	int label_length = strlen(label);
	if (label_length > 64) {
		return -1;
	}

	char buffer[64];
	snprintf(buffer, sizeof(buffer), "%c%s%c", '{', label, '}');

	const char * found = strstr(format, buffer);
	if (found == NULL) {
		return -1;
	}

	int offset = (long)found - (long)format;
	int resume = offset + label_length + 2;

	switch(type) {
		case WB_MOD_INT:
			snprintf(result, length, "%.*s%d%s", offset, format,
							*(int *)target, format + resume);
			break;
		case WB_MOD_LL:
			snprintf(result, length, "%.*s%lld%s", offset, format,
							*(long long int *)target, format + resume);
			break;
		case WB_MOD_FLOAT:
			snprintf(result, length, "%.*s%f%s", offset, format,
							*(double *)target, format + resume);
			break;
		case WB_MOD_STRING:
			snprintf(result, length, "%.*s%s%s", offset, format,
							(char *) target, format + resume);
			break;
		default:
	}
	
	printf("%s\n", result);
	return 0;
}


