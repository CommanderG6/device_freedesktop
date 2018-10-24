/*
 * Copyright (C) 2012 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include <errno.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <unistd.h>
#include <wayland-client.h>
#include <linux/input.h>
#include <sys/socket.h>
#include <sys/un.h>

#include <log/log.h>
#include <hardware/hwcomposer.h>
#include <sync/sync.h>
#include <drm_fourcc.h>
#include <linux-dmabuf-unstable-v1-client-protocol.h>
#include <gralloc_handle.h>

#include "simple-dmabuf-drm.h"

struct spurv_hwc_composer_device_1 {
    hwc_composer_device_1_t base; // constant after init
    const hwc_procs_t *procs;     // constant after init
    pthread_t wayland_thread;     // constant after init
    int32_t vsync_period_ns;      // constant after init
    struct display *display;      // constant after init
    struct window *window;        // constant after init

    pthread_mutex_t vsync_lock;
    bool vsync_callback_enabled; // protected by this->vsync_lock

    int input_fd;
};

#define USE_SUBSURFACES 0

static int hwc_prepare(hwc_composer_device_1_t* dev __unused,
                       size_t numDisplays, hwc_display_contents_1_t** displays) {
    //ALOGE("*** %s: %d", __PRETTY_FUNCTION__, 1);

    if (!numDisplays || !displays) return 0;

    hwc_display_contents_1_t* contents = displays[HWC_DISPLAY_PRIMARY];

    if (!contents) return 0;

    for (size_t i = 0; i < contents->numHwLayers; i++) {
        //ALOGE("*** %s: contents->hwLayers[i].compositionType %d", __PRETTY_FUNCTION__, contents->hwLayers[i].compositionType);
#if USE_SUBSURFACES
        if (contents->hwLayers[i].compositionType == HWC_FRAMEBUFFER)
            contents->hwLayers[i].compositionType = HWC_OVERLAY;
        else if (contents->hwLayers[i].compositionType == HWC_BACKGROUND)
            contents->hwLayers[i].compositionType = HWC_FRAMEBUFFER;
#else
        if (contents->hwLayers[i].compositionType == HWC_BACKGROUND)
            contents->hwLayers[i].compositionType = HWC_FRAMEBUFFER;
#endif
    }

    return 0;
}

static struct buffer *get_dmabuf_buffer(struct spurv_hwc_composer_device_1* pdev, struct gralloc_handle_t *drm_handle)
{
    struct buffer *buf = NULL;
    static unsigned created_buffers = 0;
    int ret = 0;

    for (unsigned i = 0; i < created_buffers; i++) {
        if (pdev->window->buffers[i].dmabuf_fd == drm_handle->prime_fd) {
            buf = &pdev->window->buffers[i];
            break;
        }
    }

    //ALOGE("*** %s: %d prime_fd %d format %d stride %d width %d height %d usage %d modifier %" PRIu64, __PRETTY_FUNCTION__, 6, drm_handle->prime_fd, drm_handle->format, drm_handle->stride, drm_handle->width, drm_handle->height, drm_handle->usage, drm_handle->modifier);

    //gralloc_gbm_bo_t *gbm_bo = drm_handle->data;

    if (!buf) {
        int drm_format;

        assert(created_buffers < NUM_BUFFERS);
        switch(drm_handle->format) {
        case HAL_PIXEL_FORMAT_RGBA_8888:
            drm_format = DRM_FORMAT_ABGR8888;
            break;
        case HAL_PIXEL_FORMAT_RGBX_8888:
            drm_format = DRM_FORMAT_XRGB8888;
            break;
        default:
            ALOGE("failed to convert Android format %d", drm_handle->format);
            return NULL;
        }
        ret = create_dmabuf_buffer(pdev->display, &pdev->window->buffers[created_buffers], drm_handle->width, drm_handle->height, drm_format, 0, drm_handle->prime_fd, drm_handle->stride);
        if (ret) {
            ALOGE("failed to create a wayland dmabuf buffer");
            return NULL;
        }
        buf = &pdev->window->buffers[created_buffers];
        created_buffers++;
    }

    return buf;
}

static struct wl_surface *get_surface(struct spurv_hwc_composer_device_1* pdev, hwc_layer_1_t* layer, int pos)
{
    if (layer->compositionType == HWC_FRAMEBUFFER_TARGET)
        return pdev->window->surface;

    struct wl_surface *surface = NULL;
    struct wl_subsurface *subsurface = NULL;
    static unsigned created_surfaces = 0;
    int ret = 0;

    if (pos < created_surfaces) {
        surface = pdev->window->surfaces[pos];
        subsurface = pdev->window->subsurfaces[pos];
    } else {
        surface = wl_compositor_create_surface(pdev->display->compositor);
        subsurface = wl_subcompositor_get_subsurface(pdev->display->subcompositor,
							            surface,
                                        pdev->window->surface);
        pdev->window->surfaces[created_surfaces] = surface;
        pdev->window->subsurfaces[created_surfaces] = subsurface;
        created_surfaces++;
    }

    wl_subsurface_set_position(subsurface, layer->displayFrame.left, layer->displayFrame.top);
    wl_subsurface_set_desync(subsurface);

    return surface;
}

static int hwc_set(struct hwc_composer_device_1* dev,size_t numDisplays,
                   hwc_display_contents_1_t** displays) {

    int ret;
    struct spurv_hwc_composer_device_1* pdev = (struct spurv_hwc_composer_device_1*)dev;

    //ALOGE("*** %s: %d", __PRETTY_FUNCTION__, 1);
    if (!numDisplays || !displays) {
        return 0;
    }

    //ALOGE("*** %s: %d", __PRETTY_FUNCTION__, 2);
    hwc_display_contents_1_t* contents = displays[HWC_DISPLAY_PRIMARY];

    int retireFenceFd = -1;
    int err = 0;
    for (size_t layer = 0; layer < contents->numHwLayers; layer++) {
        hwc_layer_1_t* fb_layer = &contents->hwLayers[layer];
        //ALOGE("*** %s: fb_layer->compositionType %d", __PRETTY_FUNCTION__, fb_layer->compositionType);

        int releaseFenceFd = -1;
        if (fb_layer->compositionType == HWC_FRAMEBUFFER_TARGET) {
            if (fb_layer->acquireFenceFd > 0) {
                const int kAcquireWarningMS= 3000;
                err = sync_wait(fb_layer->acquireFenceFd, kAcquireWarningMS);
                if (err < 0 && errno == ETIME) {
                    ALOGE("hwcomposer waited on fence %d for %d ms",
                          fb_layer->acquireFenceFd, kAcquireWarningMS);
                }
                close(fb_layer->acquireFenceFd);

                if (fb_layer->compositionType != HWC_FRAMEBUFFER_TARGET) {
                    ALOGE("hwcomposer found acquire fence on layer %d which is not an"
                          "HWC_FRAMEBUFFER_TARGET layer", layer);
                }

                releaseFenceFd = dup(fb_layer->acquireFenceFd);
                fb_layer->acquireFenceFd = -1;
            }
        }

        //ALOGE("*** %s: %d", __PRETTY_FUNCTION__, 4);

#if USE_SUBSURFACES
        if (fb_layer->compositionType != HWC_OVERLAY &&
            fb_layer->compositionType != HWC_FRAMEBUFFER_TARGET) {
             ALOGE("Unexpected layer with compositionType %d", fb_layer->compositionType);
             continue;
        }
#else
        if (fb_layer->compositionType != HWC_FRAMEBUFFER_TARGET) {
             continue;
        }
#endif

        if (!fb_layer->handle) {
            continue;
        }

        //ALOGE("*** %s: %d handle %d", __PRETTY_FUNCTION__, 5, fb_layer->handle->data[0]);
        struct gralloc_handle_t *drm_handle = (struct gralloc_handle_t *)fb_layer->handle;
        struct buffer *buf = get_dmabuf_buffer(pdev, drm_handle);
        if (!buf) {
             ALOGE("Failed to get wl_dmabuf");
             continue;
        }

        struct wl_surface *surface = get_surface(pdev, fb_layer, layer);
        if (!surface) {
             ALOGE("Failed to get surface");
             continue;
        }

        wl_surface_attach(surface, buf->buffer, 0, 0);
        wl_surface_damage(surface, 0, 0, drm_handle->width, drm_handle->height);
        wl_surface_commit(surface);
        wl_display_flush(pdev->display->display);

        if (fb_layer->compositionType == HWC_FRAMEBUFFER_TARGET) {
            fb_layer->releaseFenceFd = releaseFenceFd;

            if (releaseFenceFd > 0) {
                if (retireFenceFd == -1) {
                    retireFenceFd = dup(releaseFenceFd);
                } else {
                    int mergedFenceFd = sync_merge("hwc_set retireFence",
                                                   releaseFenceFd, retireFenceFd);
                    close(retireFenceFd);
                    retireFenceFd = mergedFenceFd;
                }
            }
        }
    }

    contents->retireFenceFd = retireFenceFd;

    return err;
}

static int hwc_query(struct hwc_composer_device_1* dev, int what, int* value) {
    struct spurv_hwc_composer_device_1* pdev =
            (struct spurv_hwc_composer_device_1*)dev;

    ALOGE("*** %s: %d", __PRETTY_FUNCTION__, 1);
    switch (what) {
        case HWC_BACKGROUND_LAYER_SUPPORTED:
            // we do not support the background layer
            value[0] = 0;
            break;
        case HWC_VSYNC_PERIOD:
            value[0] = pdev->vsync_period_ns;
            break;
        default:
            // unsupported query
            ALOGE("%s badness unsupported query what=%d", __FUNCTION__, what);
            return -EINVAL;
    }
    return 0;
}

static int hwc_event_control(struct hwc_composer_device_1* dev, int dpy __unused,
                             int event, int enabled) {
    ALOGE("*** %s: %d", __PRETTY_FUNCTION__, 1);
#if 0
    struct spurv_hwc_composer_device_1* pdev =
            (struct spurv_hwc_composer_device_1*)dev;
    int ret = -EINVAL;

    // enabled can only be 0 or 1
    if (!(enabled & ~1)) {
        if (event == HWC_EVENT_VSYNC) {
            pthread_mutex_lock(&pdev->vsync_lock);
            pdev->vsync_callback_enabled = enabled;
            pthread_mutex_unlock(&pdev->vsync_lock);
            ret = 0;
        }
    }
    return ret;
#endif
    return 0;
}

static int hwc_blank(struct hwc_composer_device_1* dev __unused, int disp,
                     int blank __unused) {
    ALOGE("*** %s: %d", __PRETTY_FUNCTION__, 1);
#if 0
    if (disp != HWC_DISPLAY_PRIMARY) {
        return -EINVAL;
    }
#endif
    return 0;
}

static void hwc_dump(hwc_composer_device_1* dev __unused, char* buff __unused,
                     int buff_len __unused) {
    // This is run when running dumpsys.
    // No-op for now.
}


static int hwc_get_display_configs(struct hwc_composer_device_1* dev __unused,
                                   int disp, uint32_t* configs, size_t* numConfigs) {
    ALOGE("*** %s: %d", __PRETTY_FUNCTION__, 1);
    if (*numConfigs == 0) {
        return 0;
    }

    if (disp == HWC_DISPLAY_PRIMARY) {
        configs[0] = 0;
        *numConfigs = 1;
        return 0;
    }

    return -EINVAL;
}


static int32_t hwc_attribute(struct spurv_hwc_composer_device_1* pdev,
                             const uint32_t attribute) {
    ALOGE("*** %s: %d", __PRETTY_FUNCTION__, 1);
    switch(attribute) {
        case HWC_DISPLAY_VSYNC_PERIOD:
            return pdev->vsync_period_ns;
        case HWC_DISPLAY_WIDTH:
            return 1024;
        case HWC_DISPLAY_HEIGHT:
            return 768;
        case HWC_DISPLAY_DPI_X:
            return 96*1000;
        case HWC_DISPLAY_DPI_Y:
            return 96*1000;
        case HWC_DISPLAY_COLOR_TRANSFORM:
            return HAL_COLOR_TRANSFORM_IDENTITY;
        default:
            ALOGE("unknown display attribute %u", attribute);
            return -EINVAL;
    }
}

static int hwc_get_display_attributes(struct hwc_composer_device_1* dev __unused,
                                      int disp, uint32_t config __unused,
                                      const uint32_t* attributes, int32_t* values) {
    struct spurv_hwc_composer_device_1* pdev = (struct spurv_hwc_composer_device_1*)dev;
    ALOGE("*** %s: %d", __PRETTY_FUNCTION__, 1);
    for (int i = 0; attributes[i] != HWC_DISPLAY_NO_ATTRIBUTE; i++) {
        if (disp == HWC_DISPLAY_PRIMARY) {
            values[i] = hwc_attribute(pdev, attributes[i]);
            if (values[i] == -EINVAL) {
                return -EINVAL;
            }
        } else {
            ALOGE("unknown display type %u", disp);
            return -EINVAL;
        }
    }

    return 0;
}

static int hwc_close(hw_device_t* dev) {
    ALOGE("*** %s: %d", __PRETTY_FUNCTION__, 1);

    struct spurv_hwc_composer_device_1* pdev = (struct spurv_hwc_composer_device_1*)dev;
    pthread_kill(pdev->wayland_thread, SIGTERM);
    pthread_join(pdev->wayland_thread, NULL);

    free(dev);
    return 0;
}

#define INPUT_PIPE_NAME "/dev/input/wayland_events"

static int
ensure_pipe(struct spurv_hwc_composer_device_1* pdev)
{
    if (pdev->input_fd == -1) {
        pdev->input_fd = open(INPUT_PIPE_NAME, O_WRONLY | O_NONBLOCK);
        if (pdev->input_fd == -1) {
            ALOGE("Failed to open pipe to InputFlinger: %s", strerror(errno));
            return -1;
        }
    }
    return 0;
}

#define ADD_EVENT(time_, type_, code_, value_)     \
    event[n].time.tv_sec = time_ / 1000;           \
    event[n].time.tv_usec = (time_ % 1000) * 1000; \
    event[n].type = type_;                         \
    event[n].code = code_;                         \
    event[n].value = value_;                       \
    n++;

static void
touch_handle_down(void *data, struct wl_touch *wl_touch,
		  uint32_t serial, uint32_t time, struct wl_surface *surface,
		  int32_t id, wl_fixed_t x_w, wl_fixed_t y_w)
{
    struct spurv_hwc_composer_device_1* pdev = (struct spurv_hwc_composer_device_1*)data;
    struct input_event event[6];
    int res, n = 0;

    if (ensure_pipe(pdev))
        return;

    ADD_EVENT(time, EV_ABS, ABS_MT_SLOT, id);
    ADD_EVENT(time, EV_ABS, ABS_MT_TRACKING_ID, id);
    ADD_EVENT(time, EV_ABS, ABS_MT_POSITION_X, wl_fixed_to_int(x_w));
    ADD_EVENT(time, EV_ABS, ABS_MT_POSITION_Y, wl_fixed_to_int(y_w));
    ADD_EVENT(time, EV_ABS, ABS_MT_PRESSURE, 50);
    ADD_EVENT(time, EV_SYN, SYN_REPORT, 0);

    res = write(pdev->input_fd, &event, sizeof(event));
    if (res < sizeof(event))
        ALOGE("Failed to write event for InputFlinger: %s", strerror(errno));
}

static void
touch_handle_up(void *data, struct wl_touch *wl_touch,
		uint32_t serial, uint32_t time, int32_t id)
{
    struct spurv_hwc_composer_device_1* pdev = (struct spurv_hwc_composer_device_1*)data;
    struct input_event event[4];
    int res, n = 0;

    if (ensure_pipe(pdev))
        return;

    ADD_EVENT(time, EV_ABS, ABS_MT_SLOT, id);
    ADD_EVENT(time, EV_ABS, ABS_MT_TRACKING_ID, -1);
    ADD_EVENT(time, EV_SYN, SYN_REPORT, 0);

    res = write(pdev->input_fd, &event, sizeof(event));
    if (res < sizeof(event))
        ALOGE("Failed to write event for InputFlinger: %s", strerror(errno));
}

static void
touch_handle_motion(void *data, struct wl_touch *wl_touch,
		    uint32_t time, int32_t id, wl_fixed_t x_w, wl_fixed_t y_w)
{
    struct spurv_hwc_composer_device_1* pdev = (struct spurv_hwc_composer_device_1*)data;
    struct input_event event[6];
    int res, n = 0;

    if (ensure_pipe(pdev))
        return;

    ADD_EVENT(time, EV_ABS, ABS_MT_SLOT, id);
    ADD_EVENT(time, EV_ABS, ABS_MT_TRACKING_ID, id);
    ADD_EVENT(time, EV_ABS, ABS_MT_POSITION_X, wl_fixed_to_int(x_w));
    ADD_EVENT(time, EV_ABS, ABS_MT_POSITION_Y, wl_fixed_to_int(y_w));
    ADD_EVENT(time, EV_ABS, ABS_MT_PRESSURE, 50);
    ADD_EVENT(time, EV_SYN, SYN_REPORT, 0);

    res = write(pdev->input_fd, &event, sizeof(event));
    if (res < sizeof(event))
        ALOGE("Failed to write event for InputFlinger: %s", strerror(errno));
}

static void
touch_handle_frame(void *data, struct wl_touch *wl_touch)
{
}

static void
touch_handle_cancel(void *data, struct wl_touch *wl_touch)
{
}

static void
touch_handle_shape(void *data, struct wl_touch *wl_touch, int32_t id, wl_fixed_t major, wl_fixed_t minor)
{
    struct spurv_hwc_composer_device_1* pdev = (struct spurv_hwc_composer_device_1*)data;
    struct input_event event[6];
    int res, n = 0;

    if (ensure_pipe(pdev))
        return;

    ADD_EVENT(0, EV_ABS, ABS_MT_SLOT, id);
    ADD_EVENT(0, EV_ABS, ABS_MT_TRACKING_ID, id);
    ADD_EVENT(0, EV_ABS, ABS_MT_TOUCH_MAJOR, wl_fixed_to_int(major));
    ADD_EVENT(0, EV_ABS, ABS_MT_TOUCH_MINOR, wl_fixed_to_int(minor));
    ADD_EVENT(0, EV_SYN, SYN_REPORT, 0);

    res = write(pdev->input_fd, &event, sizeof(event));
    if (res < sizeof(event))
        ALOGE("Failed to write event for InputFlinger: %s", strerror(errno));
}

static void
touch_handle_orientation(void *data, struct wl_touch *wl_touch, int32_t id, wl_fixed_t orientation)
{
}

static const struct wl_touch_listener touch_listener = {
	touch_handle_down,
	touch_handle_up,
	touch_handle_motion,
	touch_handle_frame,
	touch_handle_cancel,
	touch_handle_shape,
	touch_handle_orientation,
};

static void* hwc_wayland_thread(void* data) {
    struct spurv_hwc_composer_device_1* pdev = (struct spurv_hwc_composer_device_1*)data;
    int ret = 0;

    ALOGE("*** %s: %d", __PRETTY_FUNCTION__, 1);

    setpriority(PRIO_PROCESS, 0, HAL_PRIORITY_URGENT_DISPLAY);

    while (ret != -1)
        ret = wl_display_dispatch(pdev->display->display);

    ALOGE("*** %s: Wayland client was disconnected: %s", __PRETTY_FUNCTION__, strerror(ret));

    return NULL;
}

#if 0
    struct timespec rt;
    if (clock_gettime(CLOCK_MONOTONIC, &rt) == -1) {
        ALOGE("%s:%d error in vsync thread clock_gettime: %s",
              __FILE__, __LINE__, strerror(errno));
    }
    const int log_interval = 60;
    int64_t last_logged = rt.tv_sec;
    int sent = 0;
    int last_sent = 0;
    bool vsync_enabled = false;
    struct timespec wait_time;
    wait_time.tv_sec = 0;
    wait_time.tv_nsec = pdev->vsync_period_ns;

    while (true) {
        int err = nanosleep(&wait_time, NULL);
        if (err == -1) {
            if (errno == EINTR) {
                break;
            }
            ALOGE("error in vsync thread: %s", strerror(errno));
        }

        pthread_mutex_lock(&pdev->vsync_lock);
        vsync_enabled = pdev->vsync_callback_enabled;
        pthread_mutex_unlock(&pdev->vsync_lock);

        if (!vsync_enabled) {
            continue;
        }

        if (clock_gettime(CLOCK_MONOTONIC, &rt) == -1) {
            ALOGE("%s:%d error in vsync thread clock_gettime: %s",
                  __FILE__, __LINE__, strerror(errno));
        }

        int64_t timestamp = int64_t(rt.tv_sec) * 1e9 + rt.tv_nsec;
        pdev->procs->vsync(pdev->procs, 0, timestamp);
        if (rt.tv_sec - last_logged >= log_interval) {
            ALOGD("hw_composer sent %d syncs in %ds", sent - last_sent, rt.tv_sec - last_logged);
            last_logged = rt.tv_sec;
            last_sent = sent;
        }
        ++sent;
    }
#endif
static void hwc_register_procs(struct hwc_composer_device_1* dev,
                               hwc_procs_t const* procs) {
    struct spurv_hwc_composer_device_1* pdev = (struct spurv_hwc_composer_device_1*)dev;
    pdev->procs = procs;
}
#if 0
#define INPUT_SOCKET_NAME "/dev/input/wayland_events"

static int create_input_socket(spurv_hwc_composer_device_1 *pdev)
{
    int conn, fd, ret;
    struct sockaddr_un addr = {0,};

    unlink(INPUT_SOCKET_NAME);

    conn = socket(AF_UNIX, SOCK_SEQPACKET, 0);
    if (conn == -1) {
        return conn;
    }

    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, INPUT_SOCKET_NAME, sizeof(addr.sun_path) - 1);

    ret = bind(conn, (const struct sockaddr *) &addr, sizeof(struct sockaddr_un));
    if (ret == -1) {
        return ret;
    }

    ret = listen(connection_socket, 1);
    if (ret == -1) {
        return ret;
    }

    fd = accept(conn, NULL, NULL);
    if (fd == -1) {
        return fd;
    }

    return 0;
}
#endif
static int hwc_open(const struct hw_module_t* module, const char* name,
                    struct hw_device_t** device) {
    int ret = 0;

    if (strcmp(name, HWC_HARDWARE_COMPOSER)) {
        ALOGE("%s called with bad name %s", __FUNCTION__, name);
        return -EINVAL;
    }

    spurv_hwc_composer_device_1 *pdev = new spurv_hwc_composer_device_1();
    if (!pdev) {
        ALOGE("%s failed to allocate dev", __FUNCTION__);
        return -ENOMEM;
    }

    pdev->base.common.tag = HARDWARE_DEVICE_TAG;
    pdev->base.common.version = HWC_DEVICE_API_VERSION_1_1;
    pdev->base.common.module = const_cast<hw_module_t *>(module);
    pdev->base.common.close = hwc_close;

    pdev->base.prepare = hwc_prepare;
    pdev->base.set = hwc_set;
    pdev->base.eventControl = hwc_event_control;
    pdev->base.blank = hwc_blank;
    pdev->base.query = hwc_query;
    pdev->base.registerProcs = hwc_register_procs;
    pdev->base.dump = hwc_dump;
    pdev->base.getDisplayConfigs = hwc_get_display_configs;
    pdev->base.getDisplayAttributes = hwc_get_display_attributes;

    pdev->vsync_period_ns = 1000*1000*1000/60; // vsync is 60 hz
    pdev->input_fd = -1;

    setenv("XDG_RUNTIME_DIR", "/run/user/1000", 1);
    pdev->display = create_display(&touch_listener, pdev);
    pdev->display->req_dmabuf_immediate = true;
    if (!pdev->display) {
        ALOGE("failed to open wayland connection");
        return -ENODEV;
    }
    ALOGE("wayland display %p", pdev->display);

    pdev->window = create_window(pdev->display, 1024, 768);
    if (!pdev->display) {
        ALOGE("failed to create the wayland window");
        return -ENODEV;
    }

	/* Here we retrieve the linux-dmabuf objects if executed without immed,
	 * or error */
	wl_display_roundtrip(pdev->display->display);
#if 0
	if (!pdev->window->wait_for_configure) {
	    pdev->window->callback = wl_surface_frame(pdev->window->surface);
	    wl_callback_add_listener(pdev->window->callback, &frame_listener, pdev->window);
	    wl_surface_commit(pdev->window->surface);
    }
#endif
#if 0
    /* We'll block here until InputFlinger connects */
    ret = create_input_socket(pdev);
    if (ret != 0) {
        ALOGE("Failed to create socket for input events");
        return ret;
    }
#endif
/*
    hw_module_t const* hw_module;
    ret = hw_get_module(GRALLOC_HARDWARE_MODULE_ID, &hw_module);
    if (ret != 0) {
        ALOGE("spurv_hw_composer hwc_open %s module not found", GRALLOC_HARDWARE_MODULE_ID);
        return ret;
    }
    ret = framebuffer_open(hw_module, &pdev->fbdev);
    if (ret != 0) {
        ALOGE("spurv_hw_composer hwc_open could not open framebuffer");
    }
*/

    pthread_mutex_init(&pdev->vsync_lock, NULL);
    pdev->vsync_callback_enabled = false;

    ret = pthread_create (&pdev->wayland_thread, NULL, hwc_wayland_thread, pdev);
    if (ret) {
        ALOGE("spurv_hw_composer could not start wayland_thread\n");
    }

    *device = &pdev->base.common;

    return ret;
}


static struct hw_module_methods_t hwc_module_methods = {
    open: hwc_open,
};

hwc_module_t HAL_MODULE_INFO_SYM = {
    common: {
        tag: HARDWARE_MODULE_TAG,
        module_api_version: HWC_MODULE_API_VERSION_0_1,
        hal_api_version: HARDWARE_HAL_API_VERSION,
        id: HWC_HARDWARE_MODULE_ID,
        name: "Android Emulator hwcomposer module",
        author: "The Android Open Source Project",
        methods: &hwc_module_methods,
    }
};
