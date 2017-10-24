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

#ifndef _BINDING_UTILS_H_
#define _BINDING_UTILS_H_

#include <string.h>
#include <json-c/json.h>

#ifndef AFB_BINDING_VERSION
#define AFB_BINDING_VERSION 2
#endif
#include <afb/afb-binding.h>

#define REGISTER_VERB(n, a, i, s) { .verb = #n, .callback = verb_##n, .auth = a, .info = i, .session = s }

/**
 * @brief Get a string from a json object.
 * @param[in] obj Json object from wich the string is queried.
 * @param[in] name Name of the string to get.
 * @return The string value.
 */
static inline const char* get_json_string(struct json_object* obj, const char* name)
{
	struct json_object* item = NULL;
	if (!obj || !name || !strlen(name)) return NULL;
	if (!json_object_object_get_ex(obj, name, &item) || !item) return NULL;
	return json_object_get_string(item);
}

/**
 * @brief Add a string key/value to a json object.
 * @param[in] obj The json object to which the key/value is added.
 * @param[in] key The key to add.
 * @param[in] value The value to add.
 */
static inline void json_object_add_string(struct json_object* obj, const char* key, const char* value)
{
	json_object_object_add(obj, key, json_object_new_string(value));
}

/**
 * @brief Send an @c event with the specified @c message then fail with the same @c message.
 * @param[in] req The query to fail.
 * @param[in] event The event to push.
 * @param[in] message The message to push with the event and use as a fail message.
 */
static inline void afb_fail_ex(struct afb_req req, struct afb_event event, const char* message)
{
	struct json_object* result = json_object_new_object();
	json_object_add_string(result, "message", message);
	afb_event_push(event, result);
	
	afb_req_fail(req, message, NULL);
}

#endif // _BINDING_UTILS_H_
