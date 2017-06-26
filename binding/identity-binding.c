#define _GNU_SOURCE
#define AFB_BINDING_PRAGMA_NO_VERBOSE_MACRO

#include <string.h>
#include <json-c/json.h>
#include <afb/afb-binding-v2.h>
#include <afb/afb-req-v2.h>
#include <afb/afb-req-itf.h>

// ---------- Verb's declaration ----------------------------------------------
static void verb_login(struct afb_req req);
static void verb_logout(struct afb_req req);
static void verb_open_session(struct afb_req req);
static void verb_close_session(struct afb_req req);
static void verb_set_data(struct afb_req req);
static void verb_get_data(struct afb_req req);

// ---------- Binding's metadata ----------------------------------------------
static const struct afb_auth _afb_auth_v2_identity[] = {};

static const struct afb_verb_v2 _afb_verbs_v2_identity[] =
{
	{
		.verb = "login",
		.callback = verb_login,
		.auth = NULL,
		.session = 0,
	},
	{
		.verb = "logout",
		.callback = verb_logout,
		.auth = NULL,
		.session = 0,
	},
	{
		.verb = "open_session",
		.callback = verb_open_session,
		.auth = NULL,
		.session = 0,
	},
	{
		.verb = "close_session",
		.callback = verb_close_session,
		.auth = NULL,
		.session = 0,
	},
	{
		.verb = "get_data",
		.callback = verb_get_data,
		.auth = NULL,
		.session = 0,
	},
	{
		.verb = "set_data",
		.callback = set_data,
		.auth = NULL,
		.session = 0,
	},
	{ .verb = NULL }
};

static const struct afb_binding_v2 _afb_binding_v2_identity =
{
	.api = "identity",
	.specification = NULL,
	.verbs = _afb_verbs_v2_identity,
	.preinit = NULL,
	.init = NULL,
	.onevent = NULL
};

// ---------- Verb's implementation -------------------------------------------

static void verb_login(struct afb_req req)
{
}

static void verb_logout(struct afb_req req)
{
}

static void verb_open_session(struct afb_req req)
{
}

static void verb_close_session(struct afb_req req)
{
}

static void verb_get_data(struct afb_req req)
{
}

static void verb_set_data(struct afb_req req)
{
}
