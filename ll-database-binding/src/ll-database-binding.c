/*
 * Copyright 2017 IoT.bzh
 *
 * author: Loïc Collignon <loic.collignon@iot.bzh>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#define _GNU_SOURCE
#include <unistd.h>
#include <sys/types.h>
#include <pwd.h>
#include <stdio.h>
#include <string.h>
#include <linux/limits.h>
#include <json-c/json.h>
#include <db.h>

#define AFB_BINDING_VERSION 2
#include <afb/afb-binding.h>

#include "utils.h"

#ifndef MAX_PATH
#define MAX_PATH 1024
#endif

#define DBFILE		"/ll-database-binding.db"
#define USERNAME	"agl"
#define APPNAME		"firefox"

// ----- Globals -----
static DB*		database;
static char	database_file[MAX_PATH];

// ----- Binding's declarations -----
static int ll_database_binding_init();
static void verb_insert(struct afb_req req);
static void verb_update(struct afb_req req);
static void verb_delete(struct afb_req req);
static void verb_read(struct afb_req req);

// ----- Binding's implementations -----

/**
 * @brief Initialize the binding.
 * @return Exit code, zero if success.
 */
static int ll_database_binding_init()
{
	struct passwd pwd;
	struct passwd* result;
	char buf[MAX_PATH];
	size_t bufsize;
	int ret;

	bufsize = sysconf(_SC_GETPW_R_SIZE_MAX);
	if (bufsize == -1 || bufsize > MAX_PATH) bufsize = MAX_PATH;

	ret = getpwuid_r(getuid(), &pwd, buf, bufsize, &result);
	if (result == NULL)
	{
		if (ret == 0) AFB_ERROR("User not found");
		else AFB_ERROR("getpwuid_r failed with %d code", ret);
		return -1;
	}

	memset(database_file, 0, MAX_PATH);
	strcat(database_file, result->pw_dir);
	strcat(database_file, DBFILE);

	AFB_INFO("The database file is '%s'", database_file);

	if ((ret = db_create(&database, NULL, 0)) != 0)
	{
		AFB_ERROR("Failed to create database: %s.", db_strerror(ret));
		return -1;
	}

	if ((ret = database->open(database, NULL, database_file, NULL, DB_BTREE, DB_CREATE, 0664)) != 0)
	{
		AFB_ERROR("Failed to open the '%s' database: %s.", database_file, db_strerror(ret));
		database->close(database, 0);
		return -1;
	}

	return 0;
}

/**
 * @brief Handle the @c read verb.
 * @param[in] req The query.
 */
static void verb_insert(struct afb_req req)
{
	DBT key;
	DBT data;
	int ret;

	char* rkey;
	const char* tag;
	const char* value;

	struct json_object* args;
	struct json_object* item;

	args = afb_req_json(req);

	if (!args)
	{
		afb_req_fail(req, "No argument provided.", NULL);
		return;
	}

	if (!json_object_object_get_ex(args, "key", &item) || !item) tag = NULL;
	else tag = json_object_get_string(item);

	if (!tag || !strlen(tag))
	{
		afb_req_fail(req, "No tag provided.", NULL);
		return;
	}

	if (!json_object_object_get_ex(args, "value", &item) || !item) value = NULL;
	else value = json_object_get_string(item);

	if (!value || !strlen(value))
	{
		afb_req_fail(req, "No value provided.", NULL);
		return;
	}

	rkey = malloc(strlen(USERNAME) + strlen(APPNAME) + strlen(tag) + 3);
	strcpy(rkey, USERNAME);
	strcat(rkey, ":");
	strcat(rkey, APPNAME);
	strcat(rkey, ":");
	strcat(rkey, tag);

	AFB_INFO("insert: key=%s, value=%s", rkey, value);

	memset(&key, 0, sizeof(key));
	memset(&data, 0, sizeof(data));

	key.data = rkey;
	key.size = (uint32_t)strlen(rkey);

	data.data = (void*)value;
	data.size = (uint32_t)strlen(value);

	if ((ret = database->put(database, NULL, &key, &data, DB_NOOVERWRITE)) == 0)
		afb_req_success_f(req, NULL, "db success: insertion %s=%s.", (char*)key.data, (char*)data.data);
	else
		afb_req_fail_f(req, "Failed to insert datas.", "db fail: insertion : %s=%s - %s", (char*)key.data, (char*)data.data, db_strerror(ret));
	free(rkey);
}

static void verb_update(struct afb_req req)
{
	DBT key;
	DBT data;
	int ret;

	char* rkey;
	const char* tag;
	const char* value;

	struct json_object* args;
	struct json_object* item;

	args = afb_req_json(req);
	// username should be get from identity binding
	// application should be get from smack
	// tag should be get using get_json_string(args, "tag");

	if (!args)
	{
		afb_req_fail(req, "No argument provided.", NULL);
		return;
	}

	if (!json_object_object_get_ex(args, "tag", &item) || !item) tag = NULL;
	else tag = json_object_get_string(item);

	if (!tag || !strlen(tag))
	{
		afb_req_fail(req, "No tag provided.", NULL);
		return;
	}

	if (!json_object_object_get_ex(args, "value", &item) || !item) value = NULL;
	else value = json_object_get_string(item);

	if (!value || !strlen(value))
	{
		afb_req_fail(req, "No value provided.", NULL);
		return;
	}

	rkey = malloc(strlen(USERNAME) + strlen(APPNAME) + strlen(tag) + 3);
	strcpy(rkey, USERNAME);
	strcat(rkey, ":");
	strcat(rkey, APPNAME);
	strcat(rkey, ":");
	strcat(rkey, tag);

	AFB_INFO("update: key=%s, value=%s", rkey, value);

	memset(&key, 0, sizeof(key));
	memset(&data, 0, sizeof(data));

	key.data = rkey;
	key.size = (uint32_t)strlen(rkey);

	data.data = (void*)value;
	data.size = (uint32_t)strlen(value);

	if ((ret = database->put(database, NULL, &key, &data, 0)) == 0)
		afb_req_success_f(req, NULL, "db success: update %s=%s.", (char*)key.data, (char*)data.data);
	else
		afb_req_fail_f(req, "Failed to update datas.", "db fail: update %s=%s - %s", (char*)key.data, (char*)data.data, db_strerror(ret));
	free(rkey);
}

static void verb_delete(struct afb_req req)
{
	DBT key;
	int ret;

	char* rkey;
	const char* tag;

	struct json_object* args;
	struct json_object* item;

	args = afb_req_json(req);

	if (!args)
	{
		afb_req_fail(req, "No argument provided.", NULL);
		return;
	}

	if (!json_object_object_get_ex(args, "tag", &item) || !item) tag = NULL;
	else tag = json_object_get_string(item);

	if (!tag || !strlen(tag))
	{
		afb_req_fail(req, "No tag provided.", NULL);
		return;
	}

	rkey = malloc(strlen(USERNAME) + strlen(APPNAME) + strlen(tag) + 3);
	strcpy(rkey, USERNAME);
	strcat(rkey, ":");
	strcat(rkey, APPNAME);
	strcat(rkey, ":");
	strcat(rkey, tag);

	AFB_INFO("delete: key=%s", rkey);

	memset(&key, 0, sizeof(key));

	key.data = rkey;
	key.size = (uint32_t)strlen(rkey);

	if ((ret = database->del(database, NULL, &key, 0)) == 0)
		afb_req_success_f(req, NULL, "db success: delete %s.", (char *)key.data);
	else
		afb_req_fail_f(req, "Failed to delete datas.", "db fail: delete %s - %s", (char*)key.data, db_strerror(ret));
	free(rkey);
}

static void verb_read(struct afb_req req)
{
	DBT key;
	DBT data;
	int ret;

	char* rkey;
	const char* tag;
	char value[4096];

	struct json_object* args;
	struct json_object* item;
	struct json_object* result;
	struct json_object* val;

	args = afb_req_json(req);

	if (!args)
	{
		afb_req_fail(req, "No argument provided.", NULL);
		return;
	}

	if (!json_object_object_get_ex(args, "tag", &item) || !item) tag = NULL;
	else tag = json_object_get_string(item);

	if (!tag || !strlen(tag))
	{
		afb_req_fail(req, "No tag provided.", NULL);
		return;
	}

	rkey = malloc(strlen(USERNAME) + strlen(APPNAME) + strlen(tag) + 3);
	strcpy(rkey, USERNAME);
	strcat(rkey, ":");
	strcat(rkey, APPNAME);
	strcat(rkey, ":");
	strcat(rkey, tag);

	AFB_INFO("update: key=%s, value=%s", rkey, value);

	memset(&key, 0, sizeof(key));
	memset(&data, 0, sizeof(data));
	memset(&value, 0, 4096);

	key.data = rkey;
	key.size = (uint32_t)strlen(rkey);

	data.data = value;
	data.ulen = 4096;
	data.flags = DB_DBT_USERMEM;

	if ((ret = database->get(database, NULL, &key, &data, 0)) == 0)
	{
		result = json_object_new_object();
		val = json_tokener_parse((char*)data.data);
		json_object_object_add(result, "value", val ? val : json_object_new_string((char*)data.data));

		afb_req_success_f(req, result, "db success: read %s=%s.", (char*)key.data, (char*)data.data);
	}
	else
		afb_req_fail_f(req, "Failed to read datas.", "db fail: read %s - %s", (char*)key.data, db_strerror(ret));
	free(rkey);
}

// ----- Binding's configuration -----
/*
static const struct afb_auth ll_database_binding_auths[] = {
};
*/

static const afb_verb_v2 ll_database_binding_verbs[]= {
		REGISTER_VERB(insert,	NULL, NULL, AFB_SESSION_NONE_V2),
		REGISTER_VERB(update,	NULL, NULL, AFB_SESSION_NONE_V2),
		REGISTER_VERB(delete,	NULL, NULL, AFB_SESSION_NONE_V2),
		REGISTER_VERB(read,		NULL, NULL, AFB_SESSION_NONE_V2),
        { .verb = NULL}
};

const struct afb_binding_v2 afbBindingV2 = {
                .api = "ll-database",
                .specification = NULL,
                .verbs = ll_database_binding_verbs,
                .preinit = NULL,
                .init = ll_database_binding_init,
                .onevent = NULL,
                .noconcurrency = 0
};
