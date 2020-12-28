/*
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */

#ifndef _NIMBLE_NPL_OS_H_
#define _NIMBLE_NPL_OS_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <ms_kern.h>
#include <assert.h>
#include <stdint.h>
#include <string.h>

#define BLE_NPL_OS_ALIGNMENT    4
#define BLE_NPL_TIME_FOREVER    MS_TIMEOUT_FOREVER

typedef uint32_t ble_npl_time_t;
typedef int32_t  ble_npl_stime_t;

struct ble_npl_event {
    ble_npl_event_fn *fn;
    void *arg;
    bool queued;
};

struct ble_npl_eventq {
    ms_handle_t evtqid;
    struct ble_npl_event *buf[32];
};

struct ble_npl_callout {
    ms_handle_t timerid;
    struct ble_npl_eventq *evq;
    struct ble_npl_event ev;
};

struct ble_npl_mutex {
    ms_handle_t mutexid;
};

struct ble_npl_sem {
    ms_handle_t semid;
};

#ifdef __cplusplus
}
#endif

#endif  /* _NPL_H_ */
