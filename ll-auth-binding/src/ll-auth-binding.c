#define _GNU_SOURCE
#include <stdio.h>
#include <string.h>
#include <json-c/json.h>
#include <security/pam_appl.h>
#include <security/pam_misc.h>
#include <libudev.h>
#include <pthread.h>
#include <poll.h>

#define AFB_BINDING_VERSION 2
#include <afb/afb-binding.h>

// Defines
#define PAM_RULE													"agl"
#define UDEV_MONITOR_POLLING_TIMEOUT								5000

#define UDEV_ACTION_UNSUPPORTED										0
#define UDEV_ACTION_ADD												1
#define UDEV_ACTION_REMOVE											2

#define LOGIN_SUCCESS												0
#define LOGIN_ERROR_USER_LOGGED										1
#define LOGIN_ERROR_PAM_START										2
#define LOGIN_ERROR_PAM_PUTENV										3
#define LOGIN_ERROR_PAM_AUTHENTICATE								4
#define LOGIN_ERROR_PAM_ACCT_MGMT									5
#define LOGIN_ERROR_PAM_NO_USER										6
#define LOGIN_ERROR_PAM_END											7

// Globals
static const char* error_messages[] =
{
	"",
	"The current user must be logged out first!",
	"PAM start failed!",
	"PAM putenv failed!",
	"PAM authenticate failed!",
	"PAM acct_mgmt failed!",
	"No user provided by the PAM module!",
	"PAM end failed!"
};

static char*					current_device						= NULL;
static char*					current_user						= NULL;
static struct pam_conv			conv								= { misc_conv, NULL };
static struct udev*				udev_context						= NULL;
static struct udev_monitor*		udev_mon							= NULL;
static pthread_t				udev_monitoring_thread_handle;
static struct afb_event			evt_login, evt_logout, evt_failed;

/**
 * @brief Free the memory associated to the specified string and nullify the pointer.
 * @param[in] string A pointer to the string.
 */
static inline void free_string(char** string)
{
	if (string)
	{
		if (*string) free(*string);
		*string = NULL;
	}
}

/**
 * @brief Free the memory associated to the specified UDev's context and nullify the pointer.
 * @param[in] ctx UDev's context.
 */
static inline void free_udev_context(struct udev** ctx)
{
	if (ctx)
	{
		if (*ctx) udev_unref(*ctx);
		*ctx = NULL;
	}
}

/**
 * @brief Free the memory associated to the specified UDev's monitor and nullify the pointer.
 * @param[in] mon UDev's monitor.
 */
static inline void free_udev_monitor(struct udev_monitor** mon)
{
	if (mon)
	{
		if (*mon) udev_monitor_unref(*mon);
		*mon = NULL;
	}
}

/**
 * @brief Print UDev infos for the specified device.
 * @param[in] dev The device.
 */
static inline void print_udev_device_info(struct udev_device* dev)
{
	AFB_INFO("    Action: %s", udev_device_get_action(dev));
	AFB_INFO("    Node: %s", udev_device_get_devnode(dev));
	AFB_INFO("    Subsystem: %s", udev_device_get_subsystem(dev));
	AFB_INFO("    Devtype: %s", udev_device_get_devtype(dev));
	AFB_INFO("    DevNum: %lu", udev_device_get_devnum(dev));
	AFB_INFO("    DevPath: %s", udev_device_get_devpath(dev));
	AFB_INFO("    Driver: %s", udev_device_get_driver(dev));
	AFB_INFO("    SeqNum: %llu", udev_device_get_seqnum(dev));
	AFB_INFO("    SysName: %s", udev_device_get_sysname(dev));
	AFB_INFO("    SysNum: %s", udev_device_get_sysnum(dev));
	AFB_INFO("    SysPath: %s", udev_device_get_syspath(dev));
}

/**
 * @brief Get the UDev's action as an int to allow switch condition.
 * @param[in] dev The device.
 */
static inline int udev_device_get_action_int(struct udev_device* dev)
{
	const char* action = udev_device_get_action(dev);
	return
		strcmp(action, "add")
		? (strcmp(action, "remove") ? UDEV_ACTION_UNSUPPORTED : UDEV_ACTION_REMOVE)
		: UDEV_ACTION_ADD;
}

/**
 * @brief PAM authentication process.
 * @param[in] pamh The handle to the PAM context.
 * @param[in] device The device to login.
 */
static int pam_process(pam_handle_t* pamh, const char* device)
{
	int r;
	
	if (!pamh) return LOGIN_ERROR_PAM_START;
	
	char pam_variable[4096] = "DEVICE=";
	strcat(pam_variable, device);
	
	if ((r = pam_putenv(pamh, pam_variable)) != PAM_SUCCESS)
		return LOGIN_ERROR_PAM_PUTENV;

	if ((r = pam_authenticate(pamh, 0)) != PAM_SUCCESS)
		return LOGIN_ERROR_PAM_AUTHENTICATE;

	if ((r = pam_acct_mgmt(pamh, 0)) != PAM_SUCCESS)
			return LOGIN_ERROR_PAM_ACCT_MGMT;

	const char* pam_user;
	pam_get_item(pamh, PAM_USER, (const void**)&pam_user);
	if (!pam_user)
		return LOGIN_ERROR_PAM_NO_USER;

	current_device = strdup(device);
	current_user = strdup(pam_user);
	
	return LOGIN_SUCCESS;
}

/**
 * @brief Login using PAM.
 * @param[in] device The device to use.
 * @return Exit code, @c LOGIN_SUCCESS on success.
 */
static int login_pam(const char* device)
{
	int r;
	pam_handle_t* pamh;
	
	if (current_user)
		return LOGIN_ERROR_USER_LOGGED;

	if ((r = pam_start("agl", NULL, &conv, &pamh)) != PAM_SUCCESS)
		return LOGIN_ERROR_PAM_START;

	r = pam_process(pamh, device);
	if (r != LOGIN_SUCCESS)
	{
		pam_end(pamh, r);
		return r;
	}

	if ((r = pam_end(pamh, r)) != PAM_SUCCESS)
		return LOGIN_ERROR_PAM_END;
	
	return LOGIN_SUCCESS;
}

/**
 * @brief Try to login a user using a device.
 * @param[in] device The device to use.
 * @return Exit code, @c LOGIN_SUCCESS if success.
 */
static int login(const char* device)
{
	int ret;
	struct json_object* result;
	
	result = json_object_new_object();
	
	ret = login_pam(device);
	switch(ret)
	{
		case LOGIN_SUCCESS:
			json_object_object_add(result, "device", json_object_new_string(current_device));
			json_object_object_add(result, "user", json_object_new_string(current_user));
			afb_event_broadcast(evt_login, result);
			break;
		default:
			json_object_object_add(result, "message", json_object_new_string(error_messages[ret]));
			afb_event_broadcast(evt_failed, result);
	}
	
	json_object_put(result);
	return ret;
}

/// @brief Try to logout a user using a device.
/// @param[in] device The device to logout.
static void logout(const char* device)
{
	struct json_object* result;
	
	result = json_object_new_object();
	
	if (current_device && !strcmp(device, current_device))
	{
		json_object_object_add(result, "device", json_object_new_string(current_device));
		json_object_object_add(result, "user", json_object_new_string(current_user));
		AFB_INFO("[logout] device: %s", device);
		afb_event_broadcast(evt_logout, NULL);
		
		free_string(&current_device);
		free_string(&current_user);
	}
	else
	{
		json_object_object_add(result, "message", json_object_new_string("The unplugged device wasn't the user key!"));
		AFB_INFO("The unplugged device wasn't the user key!");
		afb_event_broadcast(evt_failed, result);
	}
	json_object_put(result);
}

/**
 * @brief UDev's monitoring thread.
 */
void* udev_monitoring_thread(void* arg)
{
	struct udev_device* dev;
	struct pollfd pfd;
	int action;
	
	pfd.fd = udev_monitor_get_fd(udev_mon);
	pfd.events = POLLIN;
	
	while(1)
	{
		if (poll(&pfd, 1, UDEV_MONITOR_POLLING_TIMEOUT))
		{
			dev = udev_monitor_receive_device(udev_mon);
			if (dev)
			{
				if (!strcmp(udev_device_get_devtype(dev), "disk"))
				{
					action = udev_device_get_action_int(dev);
					switch(action)
					{
						case UDEV_ACTION_ADD:
							AFB_INFO("A device is plugged-in");
							print_udev_device_info(dev);
							login(udev_device_get_devnode(dev));
							break;
						case UDEV_ACTION_REMOVE:
							AFB_INFO("A device is plugged-out");
							print_udev_device_info(dev);
							logout(udev_device_get_devnode(dev));
							break;
						default:
							AFB_DEBUG("Unsupported udev action");
							break;
					}
				}
				udev_device_unref(dev);
			}
			else
			{
				AFB_ERROR("No Device from udev_monitor_receive_device().");
			}
		}
		else
		{
			AFB_DEBUG("Udev polling timeout");
		}
	}
	return NULL;
}

/**
 * @brief API's verb 'getuser'. Try to get user informations.
 * @param[in] req The request object.
 */
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

/**
 * @brief Do the cleanup when init fails.
 * @param[in] error Error message.
 * @param[in] retcode Error code to return.
 * @return An exit code equals to @c retcode.
 */
static inline int ll_auth_init_cleanup(const char* error, int retcode)
{
	AFB_ERROR_V2("%s", error);
	free_string(&current_user);
	free_string(&current_device);
	
	free_udev_monitor(&udev_mon);
	free_udev_context(&udev_context);
	return retcode;
}

/**
 * @brief Initialize the binding.
 */
int ll_auth_init()
{
	evt_login = afb_daemon_make_event("login");
	evt_logout = afb_daemon_make_event("logout");
	evt_failed = afb_daemon_make_event("failed");

	if (!afb_event_is_valid(evt_login) || !afb_event_is_valid(evt_logout) || !afb_event_is_valid(evt_failed))
		return ll_auth_init_cleanup("Can't create events", -1);
		
	udev_context = udev_new();
	if (!udev_context)
		return ll_auth_init_cleanup("Can't initialize udev's context", -1);
	
	udev_mon = udev_monitor_new_from_netlink(udev_context, "udev");
	if (!udev_mon)
		return ll_auth_init_cleanup("Can't initialize udev's monitor", -1);
	
	udev_monitor_filter_add_match_subsystem_devtype(udev_mon, "block", NULL);
	udev_monitor_enable_receiving(udev_mon);
	
	if (pthread_create(&udev_monitoring_thread_handle, NULL, udev_monitoring_thread, NULL))
		return ll_auth_init_cleanup("Can't start the udev's monitoring thread", -1);
	
	AFB_INFO("ll-auth-binding is ready");
	return 0;
}

static const afb_verb_v2 _ll_auth_binding_verbs[]= {
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
		.preinit = NULL,
		.init = ll_auth_init,
		.onevent = NULL,
		.noconcurrency = 0
};
