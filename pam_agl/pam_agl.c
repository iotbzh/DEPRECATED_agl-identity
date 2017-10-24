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

#define DATABASE_FILE "/etc/agl/keys.json"

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

char* get_file_content(const char* path)
{
	char* buffer;
	long length;
	FILE* f;
	
	buffer = NULL;
	length = 0;
	
	f = fopen(path, "rb");
	if (f)
	{
		fseek(f, 0, SEEK_END);
		length = ftell(f);
		fseek(f, 0, SEEK_SET);
		buffer = malloc(length + 1);
		if (buffer)
		{
			if (fread (buffer, 1, length, f) != length)
			{
				free(buffer);
				buffer = NULL;
			}
			else buffer[length] = 0;
		}
		fclose (f);
	}
	
	return buffer;
}

int read_device(const char* device, char** idkey)
{
	int fd;
	ssize_t sz;
	header h;
		
	printf("[PAM DEBUG] check_device %s...\n", device);
	fd = open(device, O_RDONLY);
	if (fd == -1)
	{
		printf("[PAM DEBUG] Failed to open the device %s!\n", device);
		return PAM_SERVICE_ERR;
	}

	sz = read(fd, &h, sizeof(header));
	if (sz != sizeof(header) || !is_valid_mn(h.mn) || h.size < 1) { close(fd); printf("[PAM DEBUG]: bad header!\n"); return PAM_SERVICE_ERR; }
	printf("[PAM DEBUG]: data size=%lu\n", h.size);

	*idkey = (char*)malloc(h.size + 1);
	if (!*idkey) { close(fd); printf("[PAM DEBUG] Bad alloc!\n"); return PAM_SERVICE_ERR; }

	memset(*idkey, 0, h.size + 1);
	sz = read(fd, *idkey, h.size);
	close(fd);
	if (sz != h.size) { free(idkey); printf("[PAM DEBUG] Bad data read!\n"); return PAM_SERVICE_ERR; }
	return PAM_SUCCESS;
}

int authenticate(pam_handle_t* pamh, const char* uuid)
{
	char* file_content;
	struct json_object* database;
	struct json_object* key;

	file_content = get_file_content(DATABASE_FILE);
	if (!file_content)
	{
		printf("[PAM DEBUG] Failed to read database file (%s)\n", DATABASE_FILE);
		return PAM_SERVICE_ERR;
	}
	
	database = json_tokener_parse(file_content);
	if (!database)
	{
		printf("[PAM DEBUG] Failed to parse the database\n");
		return PAM_SERVICE_ERR;
	}
	
	if (json_object_object_get_ex(database, uuid, &key))
	{
		printf("[PAM] Key found!\n");
		printf("[PAM DEBUG] pam_set_item(\"%s\")\n", uuid);
		pam_set_item(pamh, PAM_USER, uuid);
			
		const char* pam_authtok;
		if (pam_get_item(pamh, PAM_AUTHTOK, (const void**)&pam_authtok) == PAM_SUCCESS && !pam_authtok)
			pam_set_item(pamh, PAM_AUTHTOK, uuid);
		
		json_object_put(database);
		free(file_content);
		return PAM_SUCCESS;
	}
	
	printf("[PAM] Key not found!\n");
	free(file_content);
	if (database) json_object_put(database);
	return PAM_AUTH_ERR;
}

int check_device(pam_handle_t* pamh, const char* device)
{
	char* idkey;
	int ret;
	
	ret = read_device(device, &idkey);
	if (ret != PAM_SUCCESS) return ret;
	
	printf("[PAM DEBUG] Data read:\n%s\n", idkey);
	
	json_object* idkey_json = json_tokener_parse(idkey);
	if (!idkey_json)
	{
		free(idkey);
		printf("[PAM DEBUG] Failed to parse json data!\n");
		return PAM_SERVICE_ERR;
	}

	json_object* uuid_json;
	if(!json_object_object_get_ex(idkey_json, "uuid", &uuid_json))
	{
		free(idkey);
		printf("[PAM DEBUG] The json does not contains a valid uuid\n");
		return PAM_SERVICE_ERR;
	}

	const char* uuid = json_object_get_string(uuid_json);
	printf("[PAM DEBUG] uuid: %s\n", uuid);
	
	ret = authenticate(pamh, uuid);
	free(idkey);
	json_object_put(idkey_json);
	return ret;
}

void log_pam(const char* fname, int flags, int argc, const char** argv, const char* device)
{
	printf("[PAM DEBUG] ---------- %s ----------\n", fname);
	printf("[PAM DEBUG] flags: %d\n", flags);
	for(int i = 0; i < argc; ++i)
	{
		printf("[PAM DEBUG] argv[%d]: %s\n", i, argv[i]);
	}
	printf("[PAM DEBUG] device: %s\n", device);
	printf("[PAM DEBUG] ----------------------------------------\n");
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
