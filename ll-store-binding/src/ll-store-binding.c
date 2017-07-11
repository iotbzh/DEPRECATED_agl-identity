#define _GNU_SOURCE
#include <stdio.h>
#include <string.h>
#include <json-c/json.h>
#include <lmdb.h>

#define AFB_BINDING_VERSION 2
#include <afb/afb-binding.h>

#define DB_FILE "/home/agl/projects/ll-store-binding/build/src/ll-store-binding.lmdb"

MDB_env* dbenv;

/// @brief Get a string from a json object.
/// @param[in] obj Json object from wich the string is queried.
/// @param[in] name Name of the string to get.
/// @return 
const char* get_json_string(struct json_object* obj, const char* name)
{
	if (!obj || !name || !strlen(name)) return NULL;
	
	struct json_object* item = NULL;
	if (!json_object_object_get_ex(obj, name, &item) || !item) return NULL;

	return json_object_get_string(item);
}

char* make_key(const char* username, const char* appname, const char* tagname)
{
	size_t sz_username = username ? strlen(username) : 0;
	size_t sz_appname = appname ? strlen(appname) : 0;
	size_t sz_tagname = tagname ? strlen(tagname) : 0;
	size_t sz_total = sz_username + sz_appname + sz_tagname + 3;

	char* result = (char*)malloc(sz_total);
	memset(result, sz_total, 0);

	strcpy(result, username);
	result[sz_username] = '.';
	
	strcpy(result + sz_username + 1, appname);
	result[sz_username + 1 + sz_appname] = '.';

	strcpy(result + sz_username + 1 + sz_appname + 1, tagname);
	
	return result;
}

static void verb_get(struct afb_req req)
{
	int r;
	struct json_object* args = afb_req_json(req);

	AFB_INFO("args:\n%s\n", json_object_to_json_string_ext(args, JSON_C_TO_STRING_PRETTY));

	const char* username = get_json_string(args, "username");
	const char* appname = get_json_string(args, "appname");
	const char* tagname = get_json_string(args, "tagname");

	if (!username || !appname || !tagname)
	{
		AFB_ERROR("[store] username, appname and tagname must be provided!");
		afb_req_fail(req, "username, appname and tagname must be provided!", NULL);
		return;
	}

	char* keyname = make_key(username, appname, tagname);

	MDB_txn* txn;
	r = mdb_txn_begin(dbenv, NULL, 0, &txn);
	if (r)
	{
		free(keyname);
		AFB_ERROR("Failed to begin a transaction!");
		afb_req_fail(req, "Failed to begin a transaction!", NULL);
		return;
	}

	MDB_dbi dbi;
	r = mdb_dbi_open(txn, NULL, 0, &dbi);
	if (r)
	{
		free(keyname);
		mdb_txn_abort(txn);
		AFB_ERROR("Failed to open the database!");
		afb_req_fail(req, "Failed to open the database!", NULL);
		return;
	}

	MDB_val k;
	MDB_val v;
	k.mv_size = strlen(keyname) + 1;
	k.mv_data = keyname;

	if(mdb_get(txn, dbi, &k, &v))
	{
		free(keyname);
		mdb_txn_abort(txn);
		mdb_dbi_close(dbenv, dbi);
		AFB_ERROR("Failed to get the data!");
		afb_req_fail(req, "Failed to get the data!", NULL);
		return;
	}

	char* value = strndup(v.mv_data, v.mv_size + 1);
	if(mdb_txn_commit(txn))
	{
		free(keyname);
		free(value);
		mdb_dbi_close(dbenv, dbi);
		AFB_ERROR("Failed to commit the transaction!");
		return;
	}
		
	json_object* result = json_object_new_object();
	json_object_object_add(result, "key", json_object_new_string(keyname));
	json_object_object_add(result, "value", json_object_new_string(value));
	afb_req_success(req, result, NULL);

	free(value);
	free(keyname);
	return;
}

static void verb_set(struct afb_req req)
{
	int r;
	struct json_object* args = afb_req_json(req);

	AFB_INFO("args:\n%s\n", json_object_to_json_string_ext(args, JSON_C_TO_STRING_PRETTY));

	const char* username = get_json_string(args, "username");
	const char* appname = get_json_string(args, "appname");
	const char* tagname = get_json_string(args, "tagname");
	const char* value = get_json_string(args, "value");

	if (!username || !appname || !tagname)
	{
		AFB_ERROR("[store] username, appname and tagname must be provided!");
		afb_req_fail(req, "username, appname and tagname must be provided!", NULL);
		return;
	}

	char* keyname = make_key(username, appname, tagname);

	MDB_txn* txn;
	r = mdb_txn_begin(dbenv, NULL, 0, &txn);
	if (r)
	{
		free(keyname);
		AFB_ERROR("Failed to begin a transaction!");
		afb_req_fail(req, "Failed to begin a transaction!", NULL);
		return;
	}

	MDB_dbi dbi;
	r = mdb_dbi_open(txn, NULL, 0, &dbi);
	if (r)
	{
		free(keyname);
		mdb_txn_abort(txn);
		AFB_ERROR("Failed to open the database!");
		afb_req_fail(req, "Failed to open the database!", NULL);
		return;
	}

	MDB_val k;
	MDB_val v;
	k.mv_size = strlen(keyname) + 1;
	k.mv_data = keyname;
	v.mv_size = value ? strlen(value) + 1 : 0;
	v.mv_data = value;

	if(mdb_put(txn, dbi, &k, &v, 0))
	{
		free(keyname);
		mdb_txn_abort(txn);
		mdb_dbi_close(dbenv, dbi);
		AFB_ERROR("Failed to get the data!");
		afb_req_fail(req, "Failed to get the data!", NULL);
		return;
	}

	if(mdb_txn_commit(txn))
	{
		free(keyname);
		mdb_dbi_close(dbenv, dbi);
		AFB_ERROR("Failed to commit the transaction!");
		return;
	}
		
	json_object* result = json_object_new_object();
	json_object_object_add(result, "key", json_object_new_string(keyname));
	json_object_object_add(result, "value", json_object_new_string(value));
	afb_req_success(req, result, NULL);

	free(keyname);
	return;
}

int ll_store_preinit()
{
	int r = mdb_env_create(&dbenv);
	if (r)
	{
		AFB_INFO("Failed to create MDB environment!");
		dbenv = NULL;
		return r;
	}

	r = mdb_env_open(dbenv, "/home/agl/ll-store-binding.lmdb", MDB_NOSUBDIR, 0644);
	if (r)
	{
		mdb_env_close(dbenv);
		dbenv = NULL;
		AFB_INFO("Failed to open MDB environment!");
		return r;
	}

/*
	MDB_txn* txn;
	r = mdb_txn_begin(dbenv, NULL, 0, &txn);
	if (r)
	{
		mdb_env_close(dbenv);
		dbenv = NULL;
		AFB_INFO("Failed to begin a transaction!");
		return r;
	}

	MDB_dbi dbi;
	r = mdb_dbi_open(txn, NULL, 0, &dbi);
	if (r)
	{
		mdb_env_close(dbenv);
		AFB_INFO("Failed to open the database!");
		dbenv = NULL;
	}
*/
	return 0;
}

static const afb_verb_v2 _ll_store_binding_verbs[]= {
	{
		.verb = "get",
		.callback = verb_get,
		.auth = NULL,
		.info = NULL,
		.session = AFB_SESSION_NONE_V2
	},
	{
		.verb = "set",
		.callback = verb_set,
		.auth = NULL,
		.info = NULL,
		.session = AFB_SESSION_NONE_V2
	},
	{ .verb=NULL}
};

const struct afb_binding_v2 afbBindingV2 = {
		.api = "ll-store",
		.specification = NULL,
		.verbs = _ll_store_binding_verbs,
		.preinit = ll_store_preinit,
		.init = NULL,
		.onevent = NULL,
		.noconcurrency = 0
};
