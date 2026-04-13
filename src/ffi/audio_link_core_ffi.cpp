#include "audio_link_core_ffi.h"

#include <algorithm>
#include <cstring>
#include <memory>
#include <string>

#include <gst/gst.h>
#include <nlohmann/json.hpp>

#include "stream_manager.hpp"
#include "lookup_service.hpp"
#include "audio_device_manager.hpp"

using json = nlohmann::json;

static alc_status_t writeStringToOut(const std::string &value, char *out, size_t out_len, size_t *required_len)
{
    const size_t need = value.size() + 1;
    if (required_len)
        *required_len = need;

    if (!out || out_len == 0)
        return ALC_STATUS_OK;

    const size_t n = std::min(out_len - 1, value.size());
    if (n > 0)
        std::memcpy(out, value.data(), n);
    out[n] = '\0';

    if (out_len < need)
        return ALC_STATUS_INVALID_ARGUMENT;

    return ALC_STATUS_OK;
}

static AudioDeviceManager &getDeviceManager()
{
    static AudioDeviceManager mgr;
    return mgr;
}

static std::optional<AudioDevice> findDeviceByUid(const char *uid)
{
    if (!uid || uid[0] == '\0')
        return std::nullopt;

    auto devices = getDeviceManager().findAllAudioDevices();
    for (const auto &dev : devices)
    {
        if (dev.uid == uid)
            return dev;
    }
    return std::nullopt;
}

/* ── Lookup opaque handle ── */

struct alc_lookup
{
    std::unique_ptr<LookupService> impl;
};

/* ── StreamManager opaque handle ── */

struct alc_stream_manager
{
    std::unique_ptr<StreamManager> impl;
};

extern "C"
{

    const char *alc_version_string(void)
    {
        return "audio_link_core_ffi/0.2";
    }

    void alc_gst_init(void)
    {
        gst_init(nullptr, nullptr);
    }

    /* ── LookupService ── */

    alc_lookup_t *alc_lookup_create(int32_t port, const char *my_id, const char *my_name, const char *my_type)
    {
        if (port <= 0 || !my_id || my_id[0] == '\0' || !my_name || !my_type)
            return nullptr;

        try
        {
            auto *lk = new alc_lookup();
            lk->impl = std::make_unique<LookupService>(static_cast<int>(port), my_id, my_name, my_type);
            return lk;
        }
        catch (...)
        {
            return nullptr;
        }
    }

    void alc_lookup_destroy(alc_lookup_t *lk)
    {
        if (!lk)
            return;
        try
        {
            lk->impl.reset();
            delete lk;
        }
        catch (...)
        {
        }
    }

    alc_status_t alc_lookup_start(alc_lookup_t *lk)
    {
        if (!lk || !lk->impl)
            return ALC_STATUS_INVALID_ARGUMENT;
        try
        {
            lk->impl->start();
            return ALC_STATUS_OK;
        }
        catch (...)
        {
            return ALC_STATUS_INTERNAL_ERROR;
        }
    }

    alc_status_t alc_lookup_stop(alc_lookup_t *lk)
    {
        if (!lk || !lk->impl)
            return ALC_STATUS_INVALID_ARGUMENT;
        try
        {
            lk->impl->stop();
            return ALC_STATUS_OK;
        }
        catch (...)
        {
            return ALC_STATUS_INTERNAL_ERROR;
        }
    }

    size_t alc_lookup_get_peer_count(alc_lookup_t *lk)
    {
        if (!lk || !lk->impl)
            return 0;
        try
        {
            return lk->impl->getPeers().size();
        }
        catch (...)
        {
            return 0;
        }
    }

    alc_status_t alc_lookup_get_peer_json(alc_lookup_t *lk, size_t index, char *out, size_t out_len, size_t *required_len)
    {
        if (!lk || !lk->impl)
            return ALC_STATUS_INVALID_ARGUMENT;

        try
        {
            const auto peers = lk->impl->getPeers();
            if (index >= peers.size())
            {
                if (required_len)
                    *required_len = 0;
                return ALC_STATUS_INVALID_ARGUMENT;
            }

            const auto &peer = peers[index];
            json j;
            j["id"] = peer.id;
            j["name"] = peer.name;
            j["type"] = peer.type;
            j["address"] = peer.address;

            return writeStringToOut(j.dump(), out, out_len, required_len);
        }
        catch (...)
        {
            return ALC_STATUS_INTERNAL_ERROR;
        }
    }

    /* ── StreamManager ── */

    alc_stream_manager_t *alc_stream_manager_create(void)
    {
        try
        {
            auto *sm = new alc_stream_manager();
            sm->impl = std::make_unique<StreamManager>();
            return sm;
        }
        catch (...)
        {
            return nullptr;
        }
    }

    void alc_stream_manager_destroy(alc_stream_manager_t *sm)
    {
        if (!sm)
            return;
        try
        {
            sm->impl.reset();
            delete sm;
        }
        catch (...)
        {
        }
    }

    alc_status_t alc_stream_manager_add_sender(alc_stream_manager_t *sm, const char *id, const char *target_host, int32_t target_port, const char *input_device_uid_nullable)
    {
        if (!sm || !sm->impl || !id || id[0] == '\0' || !target_host || target_host[0] == '\0' || target_port <= 0)
            return ALC_STATUS_INVALID_ARGUMENT;
        try
        {
            auto inputDevice = findDeviceByUid(input_device_uid_nullable);
            sm->impl->addSender(id, target_host, static_cast<int>(target_port), inputDevice);
            return ALC_STATUS_OK;
        }
        catch (...)
        {
            return ALC_STATUS_INTERNAL_ERROR;
        }
    }

    alc_status_t alc_stream_manager_remove_sender(alc_stream_manager_t *sm, const char *id)
    {
        if (!sm || !sm->impl || !id || id[0] == '\0')
            return ALC_STATUS_INVALID_ARGUMENT;
        try
        {
            sm->impl->removeSender(id);
            return ALC_STATUS_OK;
        }
        catch (...)
        {
            return ALC_STATUS_INTERNAL_ERROR;
        }
    }

    alc_status_t alc_stream_manager_remove_all_senders(alc_stream_manager_t *sm)
    {
        if (!sm || !sm->impl)
            return ALC_STATUS_INVALID_ARGUMENT;
        try
        {
            sm->impl->removeAllSenders();
            return ALC_STATUS_OK;
        }
        catch (...)
        {
            return ALC_STATUS_INTERNAL_ERROR;
        }
    }

    alc_status_t alc_stream_manager_add_receiver(alc_stream_manager_t *sm, const char *id, const char *output_device_uid_nullable)
    {
        if (!sm || !sm->impl || !id || id[0] == '\0')
            return ALC_STATUS_INVALID_ARGUMENT;
        try
        {
            auto outputDevice = findDeviceByUid(output_device_uid_nullable);
            sm->impl->addReceiver(id, outputDevice);
            return ALC_STATUS_OK;
        }
        catch (...)
        {
            return ALC_STATUS_INTERNAL_ERROR;
        }
    }

    alc_status_t alc_stream_manager_remove_receiver(alc_stream_manager_t *sm, const char *id)
    {
        if (!sm || !sm->impl || !id || id[0] == '\0')
            return ALC_STATUS_INVALID_ARGUMENT;
        try
        {
            sm->impl->removeReceiver(id);
            return ALC_STATUS_OK;
        }
        catch (...)
        {
            return ALC_STATUS_INTERNAL_ERROR;
        }
    }

    alc_status_t alc_stream_manager_remove_all_receivers(alc_stream_manager_t *sm)
    {
        if (!sm || !sm->impl)
            return ALC_STATUS_INVALID_ARGUMENT;
        try
        {
            sm->impl->removeAllReceivers();
            return ALC_STATUS_OK;
        }
        catch (...)
        {
            return ALC_STATUS_INTERNAL_ERROR;
        }
    }

    int32_t alc_stream_manager_get_receiver_port(alc_stream_manager_t *sm, const char *id)
    {
        if (!sm || !sm->impl || !id || id[0] == '\0')
            return -1;
        return static_cast<int32_t>(sm->impl->getReceiverPort(id));
    }

    int32_t alc_stream_manager_has_sender(alc_stream_manager_t *sm, const char *id)
    {
        if (!sm || !sm->impl || !id || id[0] == '\0')
            return 0;
        return sm->impl->hasSender(id) ? 1 : 0;
    }

    int32_t alc_stream_manager_has_receiver(alc_stream_manager_t *sm, const char *id)
    {
        if (!sm || !sm->impl || !id || id[0] == '\0')
            return 0;
        return sm->impl->hasReceiver(id) ? 1 : 0;
    }

    /* ── AudioDeviceManager ── */

    size_t alc_device_get_count(void)
    {
        try
        {
            return getDeviceManager().findAllAudioDevices().size();
        }
        catch (...)
        {
            return 0;
        }
    }

    alc_status_t alc_device_get_json(size_t index, char *out, size_t out_len, size_t *required_len)
    {
        try
        {
            const auto devices = getDeviceManager().findAllAudioDevices();
            if (index >= devices.size())
            {
                if (required_len)
                    *required_len = 0;
                return ALC_STATUS_INVALID_ARGUMENT;
            }

            const auto &device = devices[index];
            json j;
            j["name"] = device.name;
            j["uid"] = device.uid;
            j["hasInput"] = device.hasInput;
            j["hasOutput"] = device.hasOutput;
            return writeStringToOut(j.dump(), out, out_len, required_len);
        }
        catch (...)
        {
            return ALC_STATUS_INTERNAL_ERROR;
        }
    }

} // extern "C"
