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

#include <security/pam_client.h>
#include <security/pam_ext.h>
#include <security/pam_filter.h>
#include <security/pam_misc.h>
#include <security/pam_modutil.h>

#define DATABASE_FILE "/etc/agl/keys.json"

int authenticate(pam_handle_t* pamh, const char* uid)
{
	struct json_object* database;
	struct json_object* nfc;
	struct json_object* key;

	database = json_object_from_file(DATABASE_FILE);
	if (!database)
	{
		printf("[PAM DEBUG] Failed to parse the database\n");
		return PAM_SERVICE_ERR;
	}
	
	if (json_object_object_get_ex(database, "nfc", &nfc))
	{	
		if (json_object_object_get_ex(nfc, uid, &key))
		{
			printf("[PAM] Key found!\n");
			printf("[PAM DEBUG] pam_set_item(\"%s\")\n", uid);
			pam_set_item(pamh, PAM_USER, uid);
				
			const char* pam_authtok;
			if (pam_get_item(pamh, PAM_AUTHTOK, (const void**)&pam_authtok) == PAM_SUCCESS && !pam_authtok)
				pam_set_item(pamh, PAM_AUTHTOK, uid);
			
			json_object_put(database);
			return PAM_SUCCESS;
		}
	}
	
	printf("[PAM] Key not found!\n");
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
	const char* uid = pam_getenv(pamh, "UID");
	log_pam("pam_sm_authenticate", flags, argc, argv, uid);
	return authenticate(pamh, uid);
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
