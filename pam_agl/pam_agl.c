#include <fcntl.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <uuid/uuid.h>
#include <json-c/json.h>

#define PAM_SM_AUTH
#define PAM_SM_ACCOUNT
#define PAM_SM_SESSION
#define PAM_SM_PASSWORD
#include <security/pam_modules.h>
#include <security/pam_appl.h>

#define BLOCK_SIZE 4096
typedef struct header_
{
	char mn[4];
	size_t size;
} header;

int is_valid_mn(const char* v)
{
	return v && v[0] == 'I' && v[1] == 'D' && v[2] == 'K' && v[3] == 'Y';
}

void pam_putenv_ex(pam_handle_t* pamh, const char* name, const char* value)
{
	char result[4096];
	strcpy(result, name);

	int offset = strlen(name);
	result[offset] = '=';

	strcpy(result + offset + 1, value);

	pam_putenv(pamh, result);
}

int check_device(pam_handle_t* pamh, const char* device)
{
	printf("[PAM DEBUG]: check_device %s...\n", device);
	int fd = open(device, O_RDONLY);
	if (fd == -1)
	{
		printf("[PAM DEBUG]: Failed to open the device %s!\n", device);
		return PAM_SERVICE_ERR;
	}

	header h;
	ssize_t sz = read(fd, &h, sizeof(header));
	if (sz != sizeof(header) || !is_valid_mn(h.mn) || h.size < 1) { close(fd); printf("[PAM DEBUG]: bad header!\n"); return PAM_SERVICE_ERR; }
	printf("[PAM DEBUG]: data size=%d\n", h.size);

	char* idkey = (char*)malloc(h.size + 1);
	if (!idkey) { close(fd); printf("[PAM DEBUG] Bad alloc!\n"); return PAM_SERVICE_ERR; }

	memset(idkey, 0, h.size + 1);
	size_t count = read(fd, idkey, h.size);
	close(fd);

	if (count != h.size) { free(idkey); printf("[PAM DEBUG] Bad data read!\n"); return PAM_SERVICE_ERR; }
	printf("[PAM DEBUG] Data read:\n%s\n", idkey);

	json_object* idkey_json = json_tokener_parse(idkey);
	if (!idkey_json) { free(idkey); printf("[PAM DEBUG] Failed to parse json data!\n"); return PAM_SERVICE_ERR; }

	json_object* uuid_json;
	if(!json_object_object_get_ex(idkey_json, "uuid", &uuid_json)) { free(idkey); printf("[PAM DEBUG]: The json does not contains a valid uuid\n"); return PAM_SERVICE_ERR; }

	const char* uuid = json_object_get_string(uuid_json);
	printf("[PAM DEBUG] uuid: %s\n", uuid);

	// TODO: Check if the uuid is accepted
	const char* const ids[] = {
		"4f29e9ea-600a-11e7-8331-c70192ecfa55",
		"13126524-6256-11e7-be33-3f4e4481a8c9",
		NULL
	};

	int i = 0;
	while(ids[i])
	{
		if (!strcmp(ids[i], uuid))
		{
			printf("[PAM DEBUG] pam_set_item(\"%s\")\n", uuid);
			pam_set_item(pamh, PAM_USER, uuid);
			
			const char* pam_authtok;
			if (pam_get_item(pamh, PAM_AUTHTOK, (const void**)&pam_authtok) == PAM_SUCCESS && !pam_authtok)
				pam_set_item(pamh, PAM_AUTHTOK, uuid);

			return PAM_SUCCESS;
		}
		++i;
	}
	
	return PAM_AUTH_ERR;
}

void log_pam(const char* fname, int flags, int argc, const char** argv, const char* device)
{
	printf("[PAM DEBUG]: ---------- %s ----------\n", fname);
	printf("[PAM DEBUG]: flags: %d\n", flags);
	for(int i = 0; i < argc; ++i)
	{
		printf("[PAM DEBUG]: argv[%d]: %s\n", i, argv[i]);
	}
	printf("[PAM DEBUG]: device: %s\n", device);
	printf("[PAM DEBUG]: ----------------------------------------\n");
}

/*!
	@brief The pam_sm_authenticate function is the service module's implementation
	of the pam_authenticate(3) interface.
	This function performs the task of authenticating the user.

	@param[in] pamh Unknown.
	@param[in] flags PAM_SILENT and/or PAM_DISALLOW_NULL_AUTHTOK.
	@return PAM_SUCCESS if ok.
*/
PAM_EXTERN int pam_sm_authenticate(pam_handle_t* pamh, int flags, int argc, const char** argv)
{
	const char* device = pam_getenv(pamh, "DEVICE");
	log_pam("pam_sm_authenticate", flags, argc, argv, device);
	return check_device(pamh, device);
}

PAM_EXTERN int pam_sm_setcred(pam_handle_t* pamh, int flags, int argc, const char** argv)
{
	log_pam("pam_sm_setcred", flags, argc, argv, pam_getenv(pamh, "DEVICE"));
	return PAM_SUCCESS;
}

PAM_EXTERN int pam_sm_acct_mgmt(pam_handle_t* pamh, int flags, int argc, const char** argv)
{
	log_pam("pam_sm_acct_mgmt", flags, argc, argv, pam_getenv(pamh, "DEVICE"));
	return PAM_SUCCESS;
}

PAM_EXTERN int pam_sm_open_session(pam_handle_t* pamh, int flags, int argc, const char** argv)
{
	log_pam("pam_sm_open_session", flags, argc, argv, pam_getenv(pamh, "DEVICE"));
	return PAM_SUCCESS;
}

PAM_EXTERN int pam_sm_close_session(pam_handle_t* pamh, int flags, int argc, const char** argv)
{
	log_pam("pam_sm_close_session", flags, argc, argv, pam_getenv(pamh, "DEVICE"));
	return PAM_SUCCESS;
}

PAM_EXTERN int pam_sm_chauthtok(pam_handle_t* pamh, int flags, int argc, const char** argv)
{
	log_pam("pam_sm_chauthtok", flags, argc, argv, pam_getenv(pamh, "DEVICE"));
	return PAM_SUCCESS;
}
