/**
 * @file test_jaln_channel_info.c This file contains tests for jaln_channel_info.c functions.
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

#include <test-dept.h>
#include "jaln_channel_info.h"

void test_jaln_channel_info_create()
{
	struct jaln_channel_info *chan_info = NULL;
	assert_equals((void*) NULL, chan_info);
	chan_info = jaln_channel_info_create();
	assert_not_equals((void*) NULL, chan_info);
	jaln_channel_info_destroy(&chan_info);
}

void test_jaln_channel_info_destroy()
{
	struct jaln_channel_info *chan_info = NULL;
	chan_info = jaln_channel_info_create();
	jaln_channel_info_destroy(&chan_info);
	assert_equals((void*) NULL, chan_info);
}

void test_jaln_channel_info_destroy_does_not_crash()
{
	struct jaln_channel_info *chan_info = NULL;
	jaln_channel_info_destroy(NULL);
	jaln_channel_info_destroy(&chan_info);
}
