#include <gst/gst.h>
#include <gst/gstdevice.h>
#include <gst/gstdevicemonitor.h>

#include <string>
#include <unordered_map>
#include <vector>

#include "audio_device.hpp"
#include "audio_device_manager.hpp"
struct AccumDevice
{
    AudioDevice dev;
};

static std::string toStdString(const gchar *s)
{
    return s ? std::string{s} : std::string{};
}

static std::string extractUid(GstDevice *device)
{
    // Try a set of common keys across platforms; fall back to name+class.
    static const char *keys[] = {
        // GStreamer common key (osxaudio, wasapi, etc.)
        "unique-id",
        // ALSA / Pulse / PipeWire
        "alsa.id", "device.bus_path", "device.id", "device.path"};

    std::string uid;
    if (GstStructure *props = gst_device_get_properties(device))
    {
        for (const char *k : keys)
        {
            if (gst_structure_has_field(props, k))
            {
                if (const gchar *v = gst_structure_get_string(props, k))
                {
                    uid = toStdString(v);
                    break;
                }
            }
        }
        gst_structure_free(props);
    }

    if (uid.empty())
    {
        const gchar *classes = gst_device_get_device_class(device);
        gchar *name = gst_device_get_display_name(device);
        uid = toStdString(name) + "|" + toStdString(classes);
        if (name)
            g_free(name);
    }
    return uid;
}

// Enumerate audio devices (sources and sinks) via GStreamer
std::vector<AudioDevice> AudioDeviceManager::findAllAudioDevices()
{
    // Ensure GStreamer initialized (idempotent)
    int argc = 0;
    char **argv = nullptr;
    (void)argc;
    (void)argv; // silence unused warning
    gst_init(nullptr, nullptr);

    std::vector<AudioDevice> out;

    GstDeviceMonitor *mon = gst_device_monitor_new();
    if (!mon)
    {
        return out;
    }

    // Monitor both input and output audio devices
    gst_device_monitor_add_filter(mon, "Audio/Source", nullptr);
    gst_device_monitor_add_filter(mon, "Audio/Sink", nullptr);

    if (!gst_device_monitor_start(mon))
    {
        g_object_unref(mon);
        return out;
    }

    // Accumulate by a best-effort UID to merge source/sink flags for the same device
    std::unordered_map<std::string, AccumDevice> map;

    GList *devices = gst_device_monitor_get_devices(mon);
    for (GList *l = devices; l != nullptr; l = l->next)
    {
        auto *dev = GST_DEVICE(l->data);
        if (!dev)
            continue;

        std::string uid = extractUid(dev);
        const gchar *classes = gst_device_get_device_class(dev);
        gchar *name = gst_device_get_display_name(dev);

        bool isSource = classes && g_strrstr(classes, "Audio/Source") != nullptr;
        bool isSink = classes && g_strrstr(classes, "Audio/Sink") != nullptr;

        auto &slot = map[uid];
        if (slot.dev.uid.empty())
        {
            slot.dev.uid = uid;
            slot.dev.name = toStdString(name);
            slot.dev.hasInput = false;
            slot.dev.hasOutput = false;
        }
        slot.dev.hasInput = slot.dev.hasInput || isSource;
        slot.dev.hasOutput = slot.dev.hasOutput || isSink;

        if (name)
            g_free(name);
    }

    g_list_free_full(devices, (GDestroyNotify)g_object_unref);
    gst_device_monitor_stop(mon);
    g_object_unref(mon);

    out.reserve(map.size());
    for (auto &kv : map)
    {
        out.push_back(kv.second.dev);
    }
    return out;
}

static GstElement *findAndCreateElement(const std::string &uid, const char *deviceClass)
{
    gst_init(nullptr, nullptr);

    GstDeviceMonitor *mon = gst_device_monitor_new();
    gst_device_monitor_add_filter(mon, deviceClass, nullptr);

    if (!gst_device_monitor_start(mon))
    {
        g_object_unref(mon);
        return nullptr;
    }

    GstElement *element = nullptr;
    GList *devices = gst_device_monitor_get_devices(mon);
    for (GList *l = devices; l; l = l->next)
    {
        GstDevice *dev = GST_DEVICE(l->data);
        if (extractUid(dev) == uid)
        {
            element = gst_device_create_element(dev, nullptr);
            break;
        }
    }

    g_list_free_full(devices, (GDestroyNotify)g_object_unref);
    gst_device_monitor_stop(mon);
    g_object_unref(mon);

    return element;
}

GstElement *AudioDeviceManager::createSourceElement(const std::string &uid)
{
    return findAndCreateElement(uid, "Audio/Source");
}

GstElement *AudioDeviceManager::createSinkElement(const std::string &uid)
{
    return findAndCreateElement(uid, "Audio/Sink");
}
