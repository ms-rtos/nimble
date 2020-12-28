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


#include <stddef.h>
#include "nimble/nimble_npl.h"

bool
ble_npl_os_started(void)
{
    return true;
}

void *
ble_npl_get_current_task_id(void)
{
    return (void *)ms_thread_self();
}

void
ble_npl_eventq_init(struct ble_npl_eventq *evq)
{
    ms_mqueue_create("nimble_evtq", (ms_ptr_t)evq->buf, MS_ARRAY_SIZE(evq->buf), sizeof(evq->buf[0]),
                     MS_WAIT_TYPE_PRIO, &evq->evtqid);
}

struct ble_npl_event *
ble_npl_eventq_get(struct ble_npl_eventq *evq, ble_npl_time_t tmo)
{
    struct ble_npl_event *ev;
    ms_err_t err;

    err = ms_mqueue_wait(evq->evtqid, (ms_ptr_t)&ev, tmo);
    if (err == MS_ERR_NONE) {
        ev->queued = false;
    } else {
        ev = NULL;
    }

    return ev;
}

void
ble_npl_eventq_put(struct ble_npl_eventq *evq, struct ble_npl_event *ev)
{
    ms_err_t err;

    if (ev->queued) {
        return;
    }

    ev->queued = true;
    err = ms_mqueue_post(evq->evtqid, (ms_ptr_t)&ev, MS_TIMEOUT_FOREVER);
    if (err != MS_ERR_NONE) {
        ev->queued = false;
    }
}

void
ble_npl_eventq_remove(struct ble_npl_eventq *evq,
                      struct ble_npl_event *ev)
{
    struct ble_npl_event *tmp;
    struct ble_npl_event *first;
    ms_err_t err;

    if (!ev->queued) {
        return;
    }

    while (1) {
        err = ms_mqueue_trywait(evq->evtqid, (ms_ptr_t)&tmp);
        if (err == MS_ERR_NONE) {
            if (!first) {
                first = tmp;
            } else if (first == tmp) {
                ms_mqueue_post_front(evq->evtqid, (ms_ptr_t)&first, MS_TIMEOUT_FOREVER);
                break;
            }

            if (tmp != ev) {
                ms_mqueue_post(evq->evtqid, (ms_ptr_t)&tmp, MS_TIMEOUT_FOREVER);
            } else {
                tmp->queued = false;
                if (tmp == first) {
                    break;
                }
            }
        } else {
            break;
        }
    }
}

void
ble_npl_event_run(struct ble_npl_event *ev)
{
    ev->fn(ev->arg);
}

void
ble_npl_event_init(struct ble_npl_event *ev, ble_npl_event_fn *fn,
                   void *arg)
{
    ev->queued = false;
    ev->fn = fn;
    ev->arg = arg;
}

bool ble_npl_event_is_queued(struct ble_npl_event *ev)
{
    return ev->queued;
}

void *
ble_npl_event_get_arg(struct ble_npl_event *ev)
{
    return ev->arg;
}

void
ble_npl_event_set_arg(struct ble_npl_event *ev, void *arg)
{
    ev->arg = arg;
}

ble_npl_error_t ble_npl_mutex_init(struct ble_npl_mutex *mu)
{
    ms_err_t err;

    if (!mu) {
        return BLE_NPL_INVALID_PARAM;
    }

    err = ms_mutex_create("nimble_lock", MS_WAIT_TYPE_PRIO, &mu->mutexid);

    return err == MS_ERR_NONE ? BLE_NPL_OK : BLE_NPL_ENOMEM;
}

ble_npl_error_t
ble_npl_mutex_pend(struct ble_npl_mutex *mu, ble_npl_time_t timeout)
{
    ms_err_t err;

    if (!mu) {
        return BLE_NPL_INVALID_PARAM;
    }

    err = ms_mutex_lock(mu->mutexid, timeout);

    return err == MS_ERR_NONE ? BLE_NPL_OK : BLE_NPL_TIMEOUT;
}

ble_npl_error_t
ble_npl_mutex_release(struct ble_npl_mutex *mu)
{
    ms_err_t err;

    if (!mu) {
        return BLE_NPL_INVALID_PARAM;
    }

    err = ms_mutex_unlock(mu->mutexid);

    return err == MS_ERR_NONE ? BLE_NPL_OK : BLE_NPL_BAD_MUTEX;
}

ble_npl_error_t
ble_npl_sem_init(struct ble_npl_sem *sem, uint16_t tokens)
{
    ms_err_t err;

    if (!sem) {
        return BLE_NPL_INVALID_PARAM;
    }

    err = ms_semc_create("nimble_lock", tokens, 0xffffffff, MS_WAIT_TYPE_PRIO, &sem->semid);

    return err == MS_ERR_NONE ? BLE_NPL_OK : BLE_NPL_ENOMEM;
}

ble_npl_error_t
ble_npl_sem_pend(struct ble_npl_sem *sem, ble_npl_time_t timeout)
{
    ms_err_t err;

    if (!sem) {
        return BLE_NPL_INVALID_PARAM;
    }

    err = ms_semc_wait(sem->semid, timeout);

    return err == MS_ERR_NONE ? BLE_NPL_OK : BLE_NPL_TIMEOUT;
}

ble_npl_error_t
ble_npl_sem_release(struct ble_npl_sem *sem)
{
    ms_err_t err;

    if (!sem) {
        return BLE_NPL_INVALID_PARAM;
    }

    err = ms_semc_post(sem->semid);

    return err == MS_ERR_NONE ? BLE_NPL_OK : BLE_NPL_EINVAL;
}

uint16_t
ble_npl_sem_get_count(struct ble_npl_sem *sem)
{
    ms_err_t err;
    ms_semc_stat_t st;

    if (!sem) {
        return 0;
    }

    err = ms_semc_stat(sem->semid, &st);
    if (err != MS_ERR_NONE) {
        st.value = 0;
    }

    return st.value;
}

static void ble_npl_callout_cb(struct ble_npl_callout * co)
{
    if (co->evq) {
        ble_npl_eventq_put(co->evq, &co->ev);
    } else {
        ble_npl_event_run(&co->ev);
    }
}

void
ble_npl_callout_init(struct ble_npl_callout *co, struct ble_npl_eventq *evq,
                     ble_npl_event_fn *ev_cb, void *ev_arg)
{
    co->evq = evq;
    ble_npl_event_init(&co->ev, ev_cb, ev_arg);

    ms_timer_create("nimble_timer", (ms_timer_callback_t)ble_npl_callout_cb, co, &co->timerid);
}

ble_npl_error_t
ble_npl_callout_reset(struct ble_npl_callout *co, ble_npl_time_t ticks)
{
    return ms_timer_start(co->timerid, ticks, 0, MS_TIMER_OPT_ONE_SHOT);
}

void
ble_npl_callout_stop(struct ble_npl_callout *co)
{
    ms_timer_stop(co->timerid);
}

bool
ble_npl_callout_is_active(struct ble_npl_callout *co)
{
    ms_err_t err;
    ms_timer_stat_t st;
    bool active;

    err = ms_timer_stat(co->timerid, &st);
    if (err != MS_ERR_NONE) {
        active = false;
    } else {
        active = (st.status == MS_TIMER_STATUS_RUNNING) ? true : false;
    }

    return active;
}

ble_npl_time_t
ble_npl_callout_get_ticks(struct ble_npl_callout *co)
{
    ms_err_t err;
    ms_timer_stat_t st;

    err = ms_timer_stat(co->timerid, &st);
    if (err != MS_ERR_NONE) {
        st.remain = 0;
    }

    return ms_time_get() + st.remain;
}

uint32_t
ble_npl_time_get(void)
{
    return ms_time_get();
}

ble_npl_error_t
ble_npl_time_ms_to_ticks(uint32_t ms, ble_npl_time_t *out_ticks)
{
    *out_ticks = MS_MS_TO_TICK(ms);
    return BLE_NPL_OK;
}

ble_npl_error_t
ble_npl_time_ticks_to_ms(ble_npl_time_t ticks, uint32_t *out_ms)
{
    *out_ms = MS_TICK_TO_MS(ticks);
    return BLE_NPL_OK;
}

ble_npl_time_t
ble_npl_time_ms_to_ticks32(uint32_t ms)
{
    return MS_MS_TO_TICK(ms);
}

uint32_t
ble_npl_time_ticks_to_ms32(ble_npl_time_t ticks)
{
    return MS_TICK_TO_MS(ticks);
}

uint32_t
ble_npl_hw_enter_critical(void)
{
    return ms_arch_int_disable();
}

void
ble_npl_hw_exit_critical(uint32_t ctx)
{
    ms_arch_int_resume(ctx);
}
