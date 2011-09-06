/**
 * @file test_jaldb_utils.c This file contains functions to test jaldb_utils.c.
 *
 * @section LICENSE
 *
 * Source code in 3rd-party is licensed and owned by their respective
 * copyright holders.
 *
 * All other source code is copyright Tresys Technology and licensed as below.
 *
 * Copyright (c) 2011 Tresys Technology LLC, Columbia, Maryland, USA
 *
 * This software was developed by Tresys Technology LLC
 * with U.S. Government sponsorship.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <db.h>
#include <dirent.h>
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <test-dept.h>
#include <time.h>
#include <unistd.h>

#include "jal_alloc.h"
#include "jaldb_strings.h"
#include "jaldb_utils.h"
#include "test_utils.h"

#define OTHER_DB_ROOT "./testdb/"

static DB_ENV *env = NULL;
static DB *dbase = NULL;

int mkdir_fails_with_eperm(__attribute__((unused)) const char *path,
		__attribute__((unused))mode_t mode)
{
	errno = EPERM;
	return -1;
}

time_t time_always_fails(__attribute__((unused)) time_t *timer)
{
	return -1;
}

int mkdir_fails_with_eexist(__attribute__((unused)) const char *path,
		__attribute__((unused))mode_t mode)
{
	errno = EEXIST;
	return -1;
}

void setup()
{
	struct stat st;
	if (stat(OTHER_DB_ROOT, &st) != 0) {
		int status;
		status = mkdir(OTHER_DB_ROOT, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
	}
	else {
		struct dirent *d;
		DIR *dir;
		char buf[256];
		dir = opendir(OTHER_DB_ROOT);
		while ((d = readdir(dir)) != NULL) {
			sprintf(buf, "%s/%s", OTHER_DB_ROOT, d->d_name);
			remove(buf);
		}
		int ret_val;
		ret_val = closedir(dir);
	}
	uint32_t env_flags = DB_CREATE |
		DB_INIT_LOCK |
		DB_INIT_LOG |
		DB_INIT_MPOOL |
		DB_INIT_TXN |
		DB_THREAD;
	int db_error = db_env_create(&env, 0);
	db_error = env->open(env, OTHER_DB_ROOT, env_flags, 0);
}

void teardown()
{
	if (dbase) {
		dbase->close(dbase, 0);
	}
	dbase = NULL;
	env->close(env, 0);
	env = NULL;
}

void test_store_confed_sid_returns_ok_with_valid_input()
{
	DB_TXN *transaction = NULL;
	int db_error = env->txn_begin(env, NULL, &transaction, 0);
	db_error = db_create(&dbase, env, 0);
	db_error = dbase->open(dbase, transaction, JALDB_CONF_DB, NULL, DB_BTREE, DB_CREATE, 0);
	char *rhost = jal_strdup("remote_host");
	char *ser_id = jal_strdup("1234");
	int err = 0;
	int *db_error_out = &err;
	int ret = jaldb_store_confed_sid(dbase, transaction, rhost, ser_id, db_error_out);
	transaction->commit(transaction, 0);
	DBT key;
	DBT data;
	memset(&key, 0, sizeof(key));
	memset(&data, 0, sizeof(data));
	key.data = jal_strdup(rhost);
	key.size = strlen(rhost);
	key.flags = DB_DBT_USERMEM;
	data.flags = DB_DBT_MALLOC;
	db_error = dbase->get(dbase, NULL, &key, &data, 0);
	assert_equals(0, db_error);
	int result;
	result = strncmp("1234", data.data, strlen("1234"));
	assert_equals(0, result);
	free(rhost);
	free(ser_id);
	free(data.data);
	rhost = NULL;
	ser_id = NULL;
	data.data = NULL;

	assert_equals(JALDB_OK, ret);
}

void test_store_confed_sid_returns_error_when_trying_to_insert_sid_twice()
{
	DB_TXN *transaction = NULL;
	int db_error = env->txn_begin(env, NULL, &transaction, 0);
	db_error = db_create(&dbase, env, 0);
	db_error = dbase->open(dbase, transaction, JALDB_CONF_DB, NULL, DB_BTREE, DB_CREATE, 0);
	char *rhost = jal_strdup("remote_host");
	char *ser_id = jal_strdup("1234");
	int err = 0;
	int *db_error_out = &err;
	int ret = jaldb_store_confed_sid(dbase, transaction, rhost, ser_id, db_error_out);
	assert_equals(JALDB_OK, ret);

	char *serid = jal_strdup("1234");
	ret = jaldb_store_confed_sid(dbase, transaction, rhost, serid, db_error_out);
	transaction->commit(transaction, 0);
	free(rhost);
	free(ser_id);
	free(serid);
	rhost = NULL;
	ser_id = NULL;
	serid = NULL;
	assert_equals(JALDB_E_ALREADY_CONFED, ret);
}

void test_store_confed_sid_returns_error_with_invalid_input()
{
	DB_TXN *transaction = NULL;
	int db_error = env->txn_begin(env, NULL, &transaction, 0);
	db_error = db_create(&dbase, env, 0);
	db_error = dbase->open(dbase, transaction, JALDB_CONF_DB, NULL, DB_BTREE, DB_CREATE, 0);
	char *rhost = jal_strdup("remote_host");
	char *ser_id = jal_strdup("1234");
	int err = 0;
	int *db_error_out = &err;
	int ret = jaldb_store_confed_sid(NULL, transaction, rhost, ser_id, db_error_out);
	assert_equals(JALDB_E_INVAL, ret);

	ret = jaldb_store_confed_sid(dbase, NULL, rhost, ser_id, db_error_out);
	assert_equals(JALDB_E_INVAL, ret);

	ret = jaldb_store_confed_sid(dbase, transaction, NULL, ser_id, db_error_out);
	assert_equals(JALDB_E_INVAL, ret);

	ret = jaldb_store_confed_sid(dbase, transaction, rhost, NULL, db_error_out);
	assert_equals(JALDB_E_INVAL, ret);

	ret = jaldb_store_confed_sid(dbase, transaction, rhost, ser_id, NULL);
	transaction->commit(transaction, 0);
	free(rhost);
	free(ser_id);
	rhost = NULL;
	ser_id= NULL;
	assert_equals(JALDB_E_INVAL, ret);
}

void test_sid_cmp_returns_correct_value()
{
	int db_error = db_create(&dbase, env, 0);
	db_error = dbase->open(dbase, NULL, JALDB_CONF_DB, NULL, DB_BTREE, DB_CREATE, 0);
	const char *s1 = "12345";
	size_t slen1 = strlen(s1);
	const char *s2 = "1234";
	size_t slen2 = strlen(s2);
	int ret = jaldb_sid_cmp(s1, slen1, s2, slen2);
	assert_equals(1, ret);

	const char *s3 = "2345";
	slen1 = strlen(s3);
	const char *s4 = "23456";
	slen2 = strlen(s4);
	ret = jaldb_sid_cmp(s3, slen1, s4, slen2);
	assert_equals(-1, ret);

	const char *s5 = "3456";
	slen1 = strlen(s5);
	const char *s6 = "3456";
	slen2 = strlen(s6);
	ret = jaldb_sid_cmp(s5, slen1, s6, slen2);
	assert_equals(0, ret);
}

void test_jaldb_create_dirs_creates_specified_directory_structure()
{
	dir_cleanup("./foo");
	enum jaldb_status ret = jaldb_create_dirs("./foo/bar/test");
	assert_equals(JALDB_OK, ret);

	struct stat st;
	assert_equals(0, stat("./foo/bar", &st));
	assert_equals(-1, stat("./foo/bar/test", &st));

	int rc = dir_cleanup("./foo");
	assert_equals(0, rc);
}

void test_jaldb_create_dirs_fails_cleanly_when_path_is_fwd_slash()
{
	enum jaldb_status ret = jaldb_create_dirs("/");
	assert_equals(JALDB_E_INTERNAL_ERROR, ret);
}

void test_jaldb_create_dirs_fails_cleanly_when_path_is_dbl_fwd_slash()
{
	enum jaldb_status ret = jaldb_create_dirs("//");
	assert_equals(JALDB_E_INTERNAL_ERROR, ret);
}

void test_jaldb_create_dirs_fails_cleanly_when_path_is_null()
{
	enum jaldb_status ret = jaldb_create_dirs(NULL);
	assert_equals(JALDB_E_INVAL, ret);
}

void test_jaldb_create_dirs_fails_cleanly_when_mkdir_fails_errno_eperm()
{
	replace_function(mkdir, mkdir_fails_with_eperm);
	enum jaldb_status ret = jaldb_create_dirs("./foo/bar/");
	assert_equals(JALDB_E_INTERNAL_ERROR, ret);
	restore_function(mkdir);
}

void test_jaldb_create_dirs_continues_when_mkdir_fails_errno_eexist()
{
	replace_function(mkdir, mkdir_fails_with_eexist);
	enum jaldb_status ret = jaldb_create_dirs("./foo/bar/");
	assert_equals(JALDB_OK, ret);
	restore_function(mkdir);
}

struct tm *gmtime_r_always_fails(__attribute__((unused)) const time_t *timer,
			__attribute__((unused)) struct tm *out)
{
	return NULL;
}

size_t strftime_always_fails(__attribute__((unused)) char *str,
			__attribute__((unused)) size_t maxsize,
			__attribute__((unused)) const char *format,
			__attribute__((unused)) const struct tm *timeptr)
{
	return 0;
}

int mkstemp_always_fails(__attribute__((unused)) char *template)
{
	return -1;
}

void test_jaldb_create_file_returns_cleanly_when_time_fails()
{
	replace_function(time, time_always_fails);
	char *path = NULL;
	int fd = -1;
	enum jaldb_status ret = jaldb_create_file("./", &path, &fd);
	assert_equals(JALDB_E_INTERNAL_ERROR, ret);
	assert_pointer_equals((void*) NULL, path);
	assert_equals(-1, fd);
	restore_function(time);
}

void test_jaldb_create_file_returns_cleanly_when_gmtime_r_fails()
{
	replace_function(gmtime_r, gmtime_r_always_fails);
	char *path = NULL;
	int fd = -1;
	enum jaldb_status ret = jaldb_create_file("./", &path, &fd);
	assert_equals(JALDB_E_INTERNAL_ERROR, ret);
	assert_pointer_equals((void*) NULL, path);
	assert_equals(-1, fd);
	restore_function(gmtime_r);
}

void test_jaldb_create_file_returns_cleanly_when_strftime_fails()
{
	replace_function(strftime, strftime_always_fails);
	char *path = NULL;
	int fd = -1;
	enum jaldb_status ret = jaldb_create_file("./", &path, &fd);
	assert_equals(JALDB_E_INTERNAL_ERROR, ret);
	assert_pointer_equals((void*) NULL, path);
	assert_equals(-1, fd);
	restore_function(strftime);
}

void test_jaldb_create_file_returns_cleanly_when_mkstemp_fails()
{
	replace_function(mkstemp, mkstemp_always_fails);
	char *path = NULL;
	int fd = -1;
	enum jaldb_status ret = jaldb_create_file("./", &path, &fd);
	assert_equals(JALDB_E_INTERNAL_ERROR, ret);
	assert_pointer_equals((void*) NULL, path);
	assert_equals(-1, fd);
	restore_function(mkstemp);
}

void test_jaldb_create_file_returns_cleanly_when_db_root_is_null()
{
	char *path = NULL;
	int fd = -1;
	enum jaldb_status ret = jaldb_create_file(NULL, &path, &fd);
	assert_equals(JALDB_E_INVAL, ret);
	assert_pointer_equals((void*) NULL, path);
	assert_equals(-1, fd);
}
