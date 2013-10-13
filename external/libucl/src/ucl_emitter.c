/* Copyright (c) 2013, Vsevolod Stakhov
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *       * Redistributions of source code must retain the above copyright
 *         notice, this list of conditions and the following disclaimer.
 *       * Redistributions in binary form must reproduce the above copyright
 *         notice, this list of conditions and the following disclaimer in the
 *         documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED ''AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL AUTHOR BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "ucl.h"
#include "ucl_internal.h"

/**
 * @file rcl_emitter.c
 * Serialise RCL object to the RCL format
 */


static void ucl_elt_write_json (ucl_object_t *obj, UT_string *buf, unsigned int tabs, bool start_tabs, bool compact);
static void ucl_obj_write_json (ucl_object_t *obj, UT_string *buf, unsigned int tabs, bool start_tabs, bool compact);
static void ucl_elt_write_rcl (ucl_object_t *obj, UT_string *buf, unsigned int tabs, bool start_tabs, bool is_top);
static void ucl_elt_write_yaml (ucl_object_t *obj, UT_string *buf, unsigned int tabs, bool start_tabs, bool compact);

/**
 * Add tabulation to the output buffer
 * @param buf target buffer
 * @param tabs number of tabs to add
 */
static inline void
ucl_add_tabs (UT_string *buf, unsigned int tabs, bool compact)
{
	while (!compact && tabs--) {
		utstring_append_len (buf, "    ", 4);
	}
}

/**
 * Serialise string
 * @param str string to emit
 * @param buf target buffer
 */
static void
ucl_elt_string_write_json (const char *str, UT_string *buf)
{
	const char *p = str;

	utstring_append_c (buf, '"');
	while (*p != '\0') {
		switch (*p) {
		case '\n':
			utstring_append_len (buf, "\\n", 2);
			break;
		case '\r':
			utstring_append_len (buf, "\\r", 2);
			break;
		case '\b':
			utstring_append_len (buf, "\\b", 2);
			break;
		case '\t':
			utstring_append_len (buf, "\\t", 2);
			break;
		case '\f':
			utstring_append_len (buf, "\\f", 2);
			break;
		case '\\':
			utstring_append_len (buf, "\\\\", 2);
			break;
		case '"':
			utstring_append_len (buf, "\\\"", 2);
			break;
		default:
			utstring_append_c (buf, *p);
			break;
		}
		p ++;
	}
	utstring_append_c (buf, '"');
}

/**
 * Write a single object to the buffer
 * @param obj object to write
 * @param buf target buffer
 */
static void
ucl_elt_obj_write_json (ucl_object_t *obj, UT_string *buf, unsigned int tabs, bool start_tabs, bool compact)
{
	ucl_object_t *cur, *tmp;

	if (start_tabs) {
		ucl_add_tabs (buf, tabs, compact);
	}
	if (compact) {
		utstring_append_c (buf, '{');
	}
	else {
		utstring_append_len (buf, "{\n", 2);
	}
	HASH_ITER (hh, obj, cur, tmp) {
		ucl_add_tabs (buf, tabs + 1, compact);
		ucl_elt_string_write_json (cur->key, buf);
		if (compact) {
			utstring_append_c (buf, ':');
		}
		else {
			utstring_append_len (buf, ": ", 2);
		}
		ucl_obj_write_json (cur, buf, tabs + 1, false, compact);
		if (cur->hh.next != NULL) {
			if (compact) {
				utstring_append_c (buf, ',');
			}
			else {
				utstring_append_len (buf, ",\n", 2);
			}
		}
		else if (!compact) {
			utstring_append_c (buf, '\n');
		}
	}
	ucl_add_tabs (buf, tabs, compact);
	utstring_append_c (buf, '}');
}

/**
 * Write a single array to the buffer
 * @param obj array to write
 * @param buf target buffer
 */
static void
ucl_elt_array_write_json (ucl_object_t *obj, UT_string *buf, unsigned int tabs, bool start_tabs, bool compact)
{
	ucl_object_t *cur = obj;

	if (start_tabs) {
		ucl_add_tabs (buf, tabs, compact);
	}
	if (compact) {
		utstring_append_c (buf, '[');
	}
	else {
		utstring_append_len (buf, "[\n", 2);
	}
	while (cur) {
		ucl_elt_write_json (cur, buf, tabs + 1, true, compact);
		if (cur->next != NULL) {
			if (compact) {
				utstring_append_c (buf, ',');
			}
			else {
				utstring_append_len (buf, ",\n", 2);
			}
		}
		else if (!compact) {
			utstring_append_c (buf, '\n');
		}
		cur = cur->next;
	}
	ucl_add_tabs (buf, tabs, compact);
	utstring_append_c (buf, ']');
}

/**
 * Emit a single element
 * @param obj object
 * @param buf buffer
 */
static void
ucl_elt_write_json (ucl_object_t *obj, UT_string *buf, unsigned int tabs, bool start_tabs, bool compact)
{
	switch (obj->type) {
	case UCL_INT:
		if (start_tabs) {
			ucl_add_tabs (buf, tabs, compact);
		}
		utstring_printf (buf, "%ld", (long int)ucl_obj_toint (obj));
		break;
	case UCL_FLOAT:
		if (start_tabs) {
			ucl_add_tabs (buf, tabs, compact);
		}
		utstring_printf (buf, "%lf", ucl_obj_todouble (obj));
		break;
	case UCL_TIME:
		if (start_tabs) {
			ucl_add_tabs (buf, tabs, compact);
		}
		utstring_printf (buf, "%lf", ucl_obj_todouble (obj));
		break;
	case UCL_BOOLEAN:
		if (start_tabs) {
			ucl_add_tabs (buf, tabs, compact);
		}
		utstring_printf (buf, "%s", ucl_obj_toboolean (obj) ? "true" : "false");
		break;
	case UCL_STRING:
		if (start_tabs) {
			ucl_add_tabs (buf, tabs, compact);
		}
		ucl_elt_string_write_json (ucl_obj_tostring (obj), buf);
		break;
	case UCL_OBJECT:
		ucl_elt_obj_write_json (obj->value.ov, buf, tabs, start_tabs, compact);
		break;
	case UCL_ARRAY:
		ucl_elt_array_write_json (obj->value.ov, buf, tabs, start_tabs, compact);
		break;
	case UCL_USERDATA:
		break;
	}
}

/**
 * Write a single object to the buffer
 * @param obj object
 * @param buf target buffer
 */
static void
ucl_obj_write_json (ucl_object_t *obj, UT_string *buf, unsigned int tabs, bool start_tabs, bool compact)
{
	ucl_object_t *cur;
	bool is_array = (obj->next != NULL);

	if (is_array) {
		/* This is an array actually */
		if (start_tabs) {
			ucl_add_tabs (buf, tabs, compact);
		}

		if (compact) {
			utstring_append_c (buf, '[');
		}
		else {
			utstring_append_len (buf, "[\n", 2);
		}
		cur = obj;
		while (cur != NULL) {
			ucl_elt_write_json (cur, buf, tabs + 1, true, compact);
			if (cur->next) {
				utstring_append_c (buf, ',');
			}
			if (!compact) {
				utstring_append_c (buf, '\n');
			}
			cur = cur->next;
		}
		ucl_add_tabs (buf, tabs, compact);
		utstring_append_c (buf, ']');
	}
	else {
		ucl_elt_write_json (obj, buf, tabs, start_tabs, compact);
	}

}

/**
 * Emit an object to json
 * @param obj object
 * @return json output (should be freed after using)
 */
static unsigned char *
ucl_object_emit_json (ucl_object_t *obj, bool compact)
{
	UT_string *buf;
	unsigned char *res;

	/* Allocate large enough buffer */
	utstring_new (buf);

	ucl_obj_write_json (obj, buf, 0, false, compact);

	res = utstring_body (buf);
	utstring_free (buf);

	return res;
}

/**
 * Write a single object to the buffer
 * @param obj object to write
 * @param buf target buffer
 */
static void
ucl_elt_obj_write_rcl (ucl_object_t *obj, UT_string *buf, unsigned int tabs, bool start_tabs, bool is_top)
{
	ucl_object_t *cur, *tmp;

	if (start_tabs) {
		ucl_add_tabs (buf, tabs, is_top);
	}
	if (!is_top) {
		utstring_append_len (buf, "{\n", 2);
	}

	while (obj) {
		HASH_ITER (hh, obj, cur, tmp) {
			ucl_add_tabs (buf, tabs + 1, is_top);
			utstring_append_len (buf, cur->key, strlen (cur->key));
			if (cur->type != UCL_OBJECT && cur->type != UCL_ARRAY) {
				utstring_append_len (buf, " = ", 3);
			}
			else {
				utstring_append_c (buf, ' ');
			}
			ucl_elt_write_rcl (cur, buf, is_top ? tabs : tabs + 1, false, false);
			if (cur->type != UCL_OBJECT && cur->type != UCL_ARRAY) {
				utstring_append_len (buf, ";\n", 2);
			}
			else {
				utstring_append_c (buf, '\n');
			}
		}
		obj = obj->next;
	}
	ucl_add_tabs (buf, tabs, is_top);
	if (!is_top) {
		utstring_append_c (buf, '}');
	}
}

/**
 * Write a single array to the buffer
 * @param obj array to write
 * @param buf target buffer
 */
static void
ucl_elt_array_write_rcl (ucl_object_t *obj, UT_string *buf, unsigned int tabs, bool start_tabs, bool is_top)
{
	ucl_object_t *cur = obj;

	if (start_tabs) {
		ucl_add_tabs (buf, tabs, false);
	}

	utstring_append_len (buf, "[\n", 2);
	while (cur) {
		ucl_elt_write_rcl (cur, buf, tabs + 1, true, false);
		utstring_append_len (buf, ",\n", 2);
		cur = cur->next;
	}
	ucl_add_tabs (buf, tabs, false);
	utstring_append_c (buf, ']');
}

/**
 * Emit a single element
 * @param obj object
 * @param buf buffer
 */
static void
ucl_elt_write_rcl (ucl_object_t *obj, UT_string *buf, unsigned int tabs, bool start_tabs, bool is_top)
{
	switch (obj->type) {
	case UCL_INT:
		if (start_tabs) {
			ucl_add_tabs (buf, tabs, false);
		}
		utstring_printf (buf, "%ld", (long int)ucl_obj_toint (obj));
		break;
	case UCL_FLOAT:
		if (start_tabs) {
			ucl_add_tabs (buf, tabs, false);
		}
		utstring_printf (buf, "%.4lf", ucl_obj_todouble (obj));
		break;
	case UCL_TIME:
		if (start_tabs) {
			ucl_add_tabs (buf, tabs, false);
		}
		utstring_printf (buf, "%.4lf", ucl_obj_todouble (obj));
		break;
	case UCL_BOOLEAN:
		if (start_tabs) {
			ucl_add_tabs (buf, tabs, false);
		}
		utstring_printf (buf, "%s", ucl_obj_toboolean (obj) ? "true" : "false");
		break;
	case UCL_STRING:
		if (start_tabs) {
			ucl_add_tabs (buf, tabs, false);
		}
		ucl_elt_string_write_json (ucl_obj_tostring (obj), buf);
		break;
	case UCL_OBJECT:
		ucl_elt_obj_write_rcl (obj->value.ov, buf, tabs, start_tabs, is_top);
		break;
	case UCL_ARRAY:
		ucl_elt_array_write_rcl (obj->value.ov, buf, tabs, start_tabs, is_top);
		break;
	case UCL_USERDATA:
		break;
	}
}

/**
 * Emit an object to rcl
 * @param obj object
 * @return rcl output (should be freed after using)
 */
static unsigned char *
ucl_object_emit_rcl (ucl_object_t *obj)
{
	UT_string *buf;
	unsigned char *res;

	/* Allocate large enough buffer */
	utstring_new (buf);

	ucl_elt_write_rcl (obj, buf, 0, false, true);

	res = utstring_body (buf);
	utstring_free (buf);

	return res;
}


/**
 * Write a single object to the buffer
 * @param obj object to write
 * @param buf target buffer
 */
static void
ucl_elt_obj_write_yaml (ucl_object_t *obj, UT_string *buf, unsigned int tabs, bool start_tabs, bool is_top)
{
	ucl_object_t *cur, *tmp;

	if (start_tabs) {
		ucl_add_tabs (buf, tabs, is_top);
	}
	if (!is_top) {
		utstring_append_len (buf, ": {\n", 4);
	}

	while (obj) {
		HASH_ITER (hh, obj, cur, tmp) {
			ucl_add_tabs (buf, tabs + 1, is_top);
			utstring_append_len (buf, cur->key, strlen (cur->key));
			if (cur->type != UCL_OBJECT && cur->type != UCL_ARRAY) {
				utstring_append_len (buf, " : ", 3);
			}
			else {
				utstring_append_c (buf, ' ');
			}
			ucl_elt_write_yaml (cur, buf, is_top ? tabs : tabs + 1, false, false);
			if (cur->type != UCL_OBJECT && cur->type != UCL_ARRAY) {
				utstring_append_len (buf, ",\n", 2);
			}
			else {
				utstring_append_c (buf, '\n');
			}
		}
		obj = obj->next;
	}
	ucl_add_tabs (buf, tabs, is_top);
	if (!is_top) {
		utstring_append_c (buf, '}');
	}
}

/**
 * Write a single array to the buffer
 * @param obj array to write
 * @param buf target buffer
 */
static void
ucl_elt_array_write_yaml (ucl_object_t *obj, UT_string *buf, unsigned int tabs, bool start_tabs, bool is_top)
{
	ucl_object_t *cur = obj;

	if (start_tabs) {
		ucl_add_tabs (buf, tabs, false);
	}

	utstring_append_len (buf, ": [\n", 4);
	while (cur) {
		ucl_elt_write_yaml (cur, buf, tabs + 1, true, false);
		utstring_append_len (buf, ",\n", 2);
		cur = cur->next;
	}
	ucl_add_tabs (buf, tabs, false);
	utstring_append_c (buf, ']');
}

/**
 * Emit a single element
 * @param obj object
 * @param buf buffer
 */
static void
ucl_elt_write_yaml (ucl_object_t *obj, UT_string *buf, unsigned int tabs, bool start_tabs, bool is_top)
{
	switch (obj->type) {
	case UCL_INT:
		if (start_tabs) {
			ucl_add_tabs (buf, tabs, false);
		}
		utstring_printf (buf, "%ld", (long int)ucl_obj_toint (obj));
		break;
	case UCL_FLOAT:
		if (start_tabs) {
			ucl_add_tabs (buf, tabs, false);
		}
		utstring_printf (buf, "%.4lf", ucl_obj_todouble (obj));
		break;
	case UCL_TIME:
		if (start_tabs) {
			ucl_add_tabs (buf, tabs, false);
		}
		utstring_printf (buf, "%.4lf", ucl_obj_todouble (obj));
		break;
	case UCL_BOOLEAN:
		if (start_tabs) {
			ucl_add_tabs (buf, tabs, false);
		}
		utstring_printf (buf, "%s", ucl_obj_toboolean (obj) ? "true" : "false");
		break;
	case UCL_STRING:
		if (start_tabs) {
			ucl_add_tabs (buf, tabs, false);
		}
		ucl_elt_string_write_json (ucl_obj_tostring (obj), buf);
		break;
	case UCL_OBJECT:
		ucl_elt_obj_write_yaml (obj->value.ov, buf, tabs, start_tabs, is_top);
		break;
	case UCL_ARRAY:
		ucl_elt_array_write_yaml (obj->value.ov, buf, tabs, start_tabs, is_top);
		break;
	case UCL_USERDATA:
		break;
	}
}

/**
 * Emit an object to rcl
 * @param obj object
 * @return rcl output (should be freed after using)
 */
static unsigned char *
ucl_object_emit_yaml (ucl_object_t *obj)
{
	UT_string *buf;
	unsigned char *res;

	/* Allocate large enough buffer */
	utstring_new (buf);

	ucl_elt_write_yaml (obj, buf, 0, false, true);

	res = utstring_body (buf);
	utstring_free (buf);

	return res;
}

unsigned char *
ucl_object_emit (ucl_object_t *obj, enum ucl_emitter emit_type)
{
	if (emit_type == UCL_EMIT_JSON) {
		return ucl_object_emit_json (obj, false);
	}
	else if (emit_type == UCL_EMIT_JSON_COMPACT) {
		return ucl_object_emit_json (obj, true);
	}
	else if (emit_type == UCL_EMIT_YAML) {
		return ucl_object_emit_yaml (obj);
	}
	else {
		return ucl_object_emit_rcl (obj);
	}

	return NULL;
}
