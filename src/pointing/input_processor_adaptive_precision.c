/*
 * Copyright (c) 2024 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#define DT_DRV_COMPAT zmk_input_processor_adaptive_precision

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/input/input.h>
#include <drivers/input_processor.h>

#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

struct ap_config {
    int32_t speed_threshold_low;
    int32_t speed_threshold_high;
    int32_t precision_numerator;
    int32_t precision_denominator;
    int32_t min_dwell_ms;
};

struct ap_data {
    bool precision_active;
    int64_t last_transition_time;
    int16_t pending_dx;
    int16_t acc_x;
    int16_t acc_y;
};

static int ap_handle_event(const struct device *dev, struct input_event *event,
                           uint32_t param1, uint32_t param2,
                           struct zmk_input_processor_state *state) {
    const struct ap_config *cfg = dev->config;
    struct ap_data *data = dev->data;

    if (event->type != INPUT_EV_REL) {
        return ZMK_INPUT_PROC_CONTINUE;
    }

    if (event->code != INPUT_REL_X && event->code != INPUT_REL_Y) {
        return ZMK_INPUT_PROC_CONTINUE;
    }

    int16_t raw = event->value;

    if (event->code == INPUT_REL_X) {
        data->pending_dx = raw;
    } else {
        /* Y event completes the pair: compute speed and update mode */
        int32_t dx = data->pending_dx;
        int32_t dy = raw;
        int32_t speed_sq = dx * dx + dy * dy;
        int64_t now = k_uptime_get();
        int64_t elapsed = now - data->last_transition_time;

        if (data->precision_active) {
            if (speed_sq > cfg->speed_threshold_high && elapsed >= cfg->min_dwell_ms) {
                data->precision_active = false;
                data->last_transition_time = now;
                data->acc_x = 0;
                data->acc_y = 0;
                LOG_DBG("adaptive-precision: NORMAL (speed_sq=%d)", speed_sq);
            }
        } else {
            if (speed_sq < cfg->speed_threshold_low && speed_sq > 0 &&
                elapsed >= cfg->min_dwell_ms) {
                data->precision_active = true;
                data->last_transition_time = now;
                data->acc_x = 0;
                data->acc_y = 0;
                LOG_DBG("adaptive-precision: PRECISION (speed_sq=%d)", speed_sq);
            }
        }
    }

    /* Apply precision scaling when active */
    if (data->precision_active) {
        int16_t *acc = (event->code == INPUT_REL_X) ? &data->acc_x : &data->acc_y;
        int32_t accumulated = *acc + raw * cfg->precision_numerator;
        int16_t scaled = accumulated / cfg->precision_denominator;
        *acc = accumulated - (scaled * cfg->precision_denominator);
        event->value = scaled;
    }

    return ZMK_INPUT_PROC_CONTINUE;
}

static int ap_init(const struct device *dev) {
    struct ap_data *data = dev->data;
    data->precision_active = false;
    data->last_transition_time = 0;
    data->pending_dx = 0;
    data->acc_x = 0;
    data->acc_y = 0;
    return 0;
}

static struct zmk_input_processor_driver_api ap_driver_api = {
    .handle_event = ap_handle_event,
};

#define AP_INST(n)                                                                                 \
    static struct ap_data ap_data_##n = {};                                                        \
    static const struct ap_config ap_config_##n = {                                                \
        .speed_threshold_low = DT_INST_PROP(n, speed_threshold_low),                               \
        .speed_threshold_high = DT_INST_PROP(n, speed_threshold_high),                             \
        .precision_numerator = DT_INST_PROP_OR(n, precision_numerator, 1),                           \
        .precision_denominator = DT_INST_PROP(n, precision_denominator),                            \
        .min_dwell_ms = DT_INST_PROP_OR(n, min_dwell_ms, 50),                                     \
    };                                                                                             \
    DEVICE_DT_INST_DEFINE(n, &ap_init, NULL, &ap_data_##n, &ap_config_##n, POST_KERNEL,            \
                          CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, &ap_driver_api);

DT_INST_FOREACH_STATUS_OKAY(AP_INST)
