#include "audio_link_core_ffi.h"

#if !defined(_WIN32) && !defined(_WIN64) && !defined(__APPLE__)

const char *alc_version_string(void)
{
    return "audio_link_core_ffi unsupported platform";
}

#else

#include <algorithm>
#include <cstring>
#include <memory>
#include <mutex>
#include <string>

#include <nlohmann/json.hpp>

#include "audio_streamer.hpp"

using json = nlohmann::json;

struct alc_streamer
{
    std::mutex mu;
    std::string lastError;
    std::unique_ptr<AudioStreamer> impl;
};

static void setError(alc_streamer_t *s, const std::string &msg)
{
    if (!s)
        return;
    std::scoped_lock lock(s->mu);
    s->lastError = msg;
}

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

extern "C"
{

    const char *alc_version_string(void)
    {
        return "audio_link_core_ffi/0.1";
    }

    alc_streamer_t *alc_streamer_create(int32_t port, const char *my_id, const char *my_name, const char *my_type)
    {
        if (port <= 0 || !my_id || my_id[0] == '\0' || !my_name || !my_type)
            return nullptr;

        try
        {
            auto *s = new alc_streamer();
            s->impl = std::make_unique<AudioStreamer>(static_cast<int>(port), std::string(my_id), std::string(my_name), std::string(my_type));
            return s;
        }
        catch (...)
        {
            return nullptr;
        }
    }

    void alc_streamer_destroy(alc_streamer_t *s)
    {
        if (!s)
            return;

        try
        {
            s->impl.reset();
            delete s;
        }
        catch (...)
        {
            // swallow
        }
    }

    alc_status_t alc_streamer_start(alc_streamer_t *s)
    {
        if (!s || !s->impl)
            return ALC_STATUS_INVALID_ARGUMENT;

        try
        {
            s->impl->start();
            return ALC_STATUS_OK;
        }
        catch (const std::exception &e)
        {
            setError(s, e.what());
            return ALC_STATUS_INTERNAL_ERROR;
        }
        catch (...)
        {
            setError(s, "unknown error");
            return ALC_STATUS_INTERNAL_ERROR;
        }
    }

    alc_status_t alc_streamer_stop(alc_streamer_t *s)
    {
        if (!s || !s->impl)
            return ALC_STATUS_INVALID_ARGUMENT;

        try
        {
            s->impl->stop();
            return ALC_STATUS_OK;
        }
        catch (const std::exception &e)
        {
            setError(s, e.what());
            return ALC_STATUS_INTERNAL_ERROR;
        }
        catch (...)
        {
            setError(s, "unknown error");
            return ALC_STATUS_INTERNAL_ERROR;
        }
    }

    alc_status_t alc_streamer_start_sending_to(alc_streamer_t *s, const char *peer_id, const char *input_device_uid_nullable)
    {
        if (!s || !s->impl || !peer_id || peer_id[0] == '\0')
            return ALC_STATUS_INVALID_ARGUMENT;

        try
        {
            if (input_device_uid_nullable && input_device_uid_nullable[0] != '\0')
                s->impl->startSendingTo(std::string(peer_id), std::string(input_device_uid_nullable));
            else
                s->impl->startSendingTo(std::string(peer_id), std::nullopt);
            return ALC_STATUS_OK;
        }
        catch (const std::exception &e)
        {
            setError(s, e.what());
            return ALC_STATUS_INTERNAL_ERROR;
        }
        catch (...)
        {
            setError(s, "unknown error");
            return ALC_STATUS_INTERNAL_ERROR;
        }
    }

    alc_status_t alc_streamer_stop_sending_to(alc_streamer_t *s, const char *peer_id)
    {
        if (!s || !s->impl || !peer_id || peer_id[0] == '\0')
            return ALC_STATUS_INVALID_ARGUMENT;

        try
        {
            s->impl->stopSendingTo(std::string(peer_id));
            return ALC_STATUS_OK;
        }
        catch (const std::exception &e)
        {
            setError(s, e.what());
            return ALC_STATUS_INTERNAL_ERROR;
        }
        catch (...)
        {
            setError(s, "unknown error");
            return ALC_STATUS_INTERNAL_ERROR;
        }
    }

    alc_status_t alc_streamer_start_receiving_from(alc_streamer_t *s, const char *peer_id, const char *output_device_uid_nullable)
    {
        if (!s || !s->impl || !peer_id || peer_id[0] == '\0')
            return ALC_STATUS_INVALID_ARGUMENT;

        try
        {
            if (output_device_uid_nullable && output_device_uid_nullable[0] != '\0')
                s->impl->startReceivingFrom(std::string(peer_id), std::string(output_device_uid_nullable));
            else
                s->impl->startReceivingFrom(std::string(peer_id), std::nullopt);
            return ALC_STATUS_OK;
        }
        catch (const std::exception &e)
        {
            setError(s, e.what());
            return ALC_STATUS_INTERNAL_ERROR;
        }
        catch (...)
        {
            setError(s, "unknown error");
            return ALC_STATUS_INTERNAL_ERROR;
        }
    }

    alc_status_t alc_streamer_stop_receiving_from(alc_streamer_t *s, const char *peer_id)
    {
        if (!s || !s->impl || !peer_id || peer_id[0] == '\0')
            return ALC_STATUS_INVALID_ARGUMENT;

        try
        {
            s->impl->stopReceivingFrom(std::string(peer_id));
            return ALC_STATUS_OK;
        }
        catch (const std::exception &e)
        {
            setError(s, e.what());
            return ALC_STATUS_INTERNAL_ERROR;
        }
        catch (...)
        {
            setError(s, "unknown error");
            return ALC_STATUS_INTERNAL_ERROR;
        }
    }

    alc_status_t alc_streamer_change_output_device(alc_streamer_t *s, const char *peer_id, const char *output_device_uid_nullable)
    {
        if (!s || !s->impl || !peer_id || peer_id[0] == '\0')
            return ALC_STATUS_INVALID_ARGUMENT;

        try
        {
            if (output_device_uid_nullable && output_device_uid_nullable[0] != '\0')
                s->impl->changeOutputDevice(std::string(peer_id), std::string(output_device_uid_nullable));
            else
                s->impl->changeOutputDevice(std::string(peer_id), std::nullopt);
            return ALC_STATUS_OK;
        }
        catch (const std::exception &e)
        {
            setError(s, e.what());
            return ALC_STATUS_INTERNAL_ERROR;
        }
        catch (...)
        {
            setError(s, "unknown error");
            return ALC_STATUS_INTERNAL_ERROR;
        }
    }

    alc_status_t alc_streamer_change_input_device(alc_streamer_t *s, const char *peer_id, const char *input_device_uid_nullable)
    {
        if (!s || !s->impl || !peer_id || peer_id[0] == '\0')
            return ALC_STATUS_INVALID_ARGUMENT;

        try
        {
            if (input_device_uid_nullable && input_device_uid_nullable[0] != '\0')
                s->impl->changeInputDevice(std::string(peer_id), std::string(input_device_uid_nullable));
            else
                s->impl->changeInputDevice(std::string(peer_id), std::nullopt);
            return ALC_STATUS_OK;
        }
        catch (const std::exception &e)
        {
            setError(s, e.what());
            return ALC_STATUS_INTERNAL_ERROR;
        }
        catch (...)
        {
            setError(s, "unknown error");
            return ALC_STATUS_INTERNAL_ERROR;
        }
    }

    size_t alc_streamer_get_peer_count(alc_streamer_t *s)
    {
        if (!s || !s->impl)
            return 0;

        try
        {
            const auto peers = s->impl->getPeers();
            return peers.size();
        }
        catch (...)
        {
            return 0;
        }
    }

    alc_status_t alc_streamer_get_peer_json(alc_streamer_t *s, size_t index, char *out, size_t out_len, size_t *required_len)
    {
        if (!s || !s->impl)
            return ALC_STATUS_INVALID_ARGUMENT;

        try
        {
            const auto peers = s->impl->getPeers();
            if (index >= peers.size())
            {
                setError(s, "index out of range");
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
            j["sendPortTo"] = peer.sendPortTo;
            j["receivePortFrom"] = peer.receivePortFrom;
            j["sendingTo"] = peer.sendingTo;
            j["receivingFrom"] = peer.receivingFrom;
            j["wantToSendTo"] = peer.wantToSendTo;
            j["wantToReceiveFrom"] = peer.wantToReceiveFrom;
            j["isReachable"] = peer.isReachable;

            if (peer.inputDevice.has_value())
            {
                json d;
                d["name"] = peer.inputDevice->name;
                d["uid"] = peer.inputDevice->uid;
                d["hasInput"] = peer.inputDevice->hasInput;
                d["hasOutput"] = peer.inputDevice->hasOutput;
#if defined(__APPLE__)
                d["id"] = peer.inputDevice->id;
#endif
                j["inputDevice"] = std::move(d);
            }
            else
            {
                j["inputDevice"] = nullptr;
            }

            if (peer.outputDevice.has_value())
            {
                json d;
                d["name"] = peer.outputDevice->name;
                d["uid"] = peer.outputDevice->uid;
                d["hasInput"] = peer.outputDevice->hasInput;
                d["hasOutput"] = peer.outputDevice->hasOutput;
#if defined(__APPLE__)
                d["id"] = peer.outputDevice->id;
#endif
                j["outputDevice"] = std::move(d);
            }
            else
            {
                j["outputDevice"] = nullptr;
            }

            return writeStringToOut(j.dump(), out, out_len, required_len);
        }
        catch (const std::exception &e)
        {
            setError(s, e.what());
            return ALC_STATUS_INTERNAL_ERROR;
        }
        catch (...)
        {
            setError(s, "unknown error");
            return ALC_STATUS_INTERNAL_ERROR;
        }
    }

    alc_status_t alc_streamer_last_error(alc_streamer_t *s, char *out, size_t out_len, size_t *required_len)
    {
        if (!s)
            return ALC_STATUS_INVALID_ARGUMENT;

        std::string msg;
        {
            std::scoped_lock lock(s->mu);
            msg = s->lastError;
        }

        return writeStringToOut(msg, out, out_len, required_len);
    }

} // extern "C"

#endif
