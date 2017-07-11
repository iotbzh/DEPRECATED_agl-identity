#define _GNU_SOURCE
#include <stdio.h>
#include <string.h>
#include <json-c/json.h>
#include <security/pam_appl.h>
#include <security/pam_misc.h>

#define AFB_BINDING_VERSION 2
#include <afb/afb-binding.h>

static struct pam_conv conv = { misc_conv, NULL };

static char* current_device = NULL;
static char* current_user = NULL;

afb_event evt_login, evt_logout;

/// @brief API's verb 'login'. Try to login a user using a device
/// @param[in] req The request object. Should contains a json with a "device" key.
static void verb_login(struct afb_req req)
{
	struct json_object* args = NULL;
	struct json_object* device_object = NULL;
	pam_handle_t* pamh;
	int r;

	if (current_user)
	{
		AFB_ERROR("[login] the current user must be logged out first!");
		afb_req_fail(req, "current user must be logged out first!", NULL);
		return;
	}

	args = afb_req_json(req);
	if (args == NULL || !json_object_object_get_ex(args, "device", &device_object))
	{
		AFB_ERROR("[login] device must be provided!");
		afb_req_fail(req, "device must be provided!", NULL);
		return;
	}

	const char* device = json_object_get_string(device_object);

	if ((r = pam_start("agl", NULL, &conv, &pamh)) != PAM_SUCCESS)
	{
		AFB_ERROR("PAM start failed!");
		afb_req_fail(req, "PAM start failed!", NULL);
		return;
	}

	char pam_variable[4096] = "DEVICE=";
	strcat(pam_variable, device);
	
	if ((r = pam_putenv(pamh, pam_variable)) != PAM_SUCCESS)
	{
		AFB_ERROR("PAM putenv failed!");
		afb_req_fail(req, "PAM putenv failed!", NULL);
		pam_end(pamh, r);
		return;
	}

	if ((r = pam_authenticate(pamh, 0)) != PAM_SUCCESS)
	{
		AFB_ERROR("PAM authenticate failed!");
		afb_req_fail(req, "PAM authenticate failed!", NULL);
		pam_end(pamh, r);
		return;
	}

	if ((r = pam_acct_mgmt(pamh, 0)) != PAM_SUCCESS)
	{
		AFB_ERROR("PAM acct_mgmt failed!");
		afb_req_fail(req, "PAM acct_mgmt failed!", NULL);
		pam_end(pamh, r);
		return;
	}

	const char* pam_user;
	pam_get_item(pamh, PAM_USER, (const void**)&pam_user);
	if (!pam_user)
	{
		AFB_ERROR("[login] No user provided by the PAM module!");
		afb_req_fail(req, "No user provided by the PAM module!", NULL);
		return;
	}

	current_device = strdup(device);
	current_user = strdup(pam_user);

	if ((r = pam_end(pamh, r)) != PAM_SUCCESS)
	{
		AFB_ERROR("PAM end failed!");
		afb_req_fail(req, "PAM end failed!", NULL);
		return;
	}

	AFB_INFO("[login] device: %s, user: %s", current_device, current_user);
	json_object* result = json_object_new_object();
	json_object_object_add(result, "device", json_object_new_string(current_device));
	json_object_object_add(result, "user", json_object_new_string(current_user));
	afb_req_success(req, NULL, current_device);
	afb_event_broadcast(evt_login, result);
}

/// @brief API's verb 'lgout'. Try to logout a user using a device
/// @param[in] req The request object. Should contains a json with a "device" key.
static void verb_logout(struct afb_req req)
{
	struct json_object* args = NULL;
	struct json_object* device_object = NULL;

	args = afb_req_json(req);
	if (args == NULL || !json_object_object_get_ex(args, "device", &device_object))
	{
		AFB_INFO("[logout] device must be provided!");
		afb_req_fail(req, "device must be provided!", NULL);
		return;
	}

	const char* device = json_object_get_string(device_object);
	if (current_device && !strcmp(device, current_device))
	{
		free(current_device);
		current_device = NULL;
		if (current_user)
		{
			free(current_user);
			current_user = NULL;
			AFB_INFO("[logout] device: %s", device);
			afb_req_success(req, NULL, device);
			afb_event_broadcast(evt_logout, NULL);
			return;
		}
		else
		{
			AFB_INFO("No user was linked to this device!");
			afb_req_fail(req, "No user was linked to this device!", NULL);
			return;
		}
	}

	AFB_INFO("The unplugged device wasn't the user key!");
	afb_req_fail(req, "The unplugged device wasn't the user key!", NULL);
}

static void verb_getuser(struct afb_req req)
{
	if (!current_device || !current_user)
	{
		afb_req_fail(req, "there is no logged user!", NULL);
		return;
	}

	json_object* result = json_object_new_object();
	json_object_object_add(result, "user", json_object_new_string(current_user));
	json_object_object_add(result, "device", json_object_new_string(current_device));

	afb_req_success(req, result, NULL);
}

int ll_auth_init()
{
	current_user = NULL;
	current_device = NULL;
	evt_login = afb_daemon_make_event("login");
	evt_logout = afb_daemon_make_event("logout");

	if (afb_event_is_valid(evt_login) && afb_event_is_valid(evt_logout))
		return 0;

	AFB_ERROR("Can't create events");
		return -1;
}

static const afb_verb_v2 _ll_auth_binding_verbs[]= {
	{
		.verb = "login",
		.callback = verb_login,
		.auth = NULL,
		.info = NULL,
		.session = AFB_SESSION_NONE_V2
	},
	{
		.verb = "logout",
		.callback = verb_logout,
		.auth = NULL,
		.info = NULL,
		.session = AFB_SESSION_NONE_V2
	},
	{
		.verb = "getuser",
		.callback = verb_getuser,
		.auth = NULL,
		.info = NULL,
		.session = AFB_SESSION_NONE_V2
	},
	{ .verb=NULL}
};

const struct afb_binding_v2 afbBindingV2 = {
		.api = "ll-auth",
		.specification = NULL,
		.verbs = _ll_auth_binding_verbs,
		.preinit = ll_auth_init,
		.init = NULL,
		.onevent = NULL,
		.noconcurrency = 0
};
