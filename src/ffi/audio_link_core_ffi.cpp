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

#include "audio_streamer.hpp"

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

    alc_streamer_t *alc_streamer_create(int32_t port, const char *my_id)
    {
        if (port <= 0 || !my_id || my_id[0] == '\0')
            return nullptr;

        try
        {
            auto *s = new alc_streamer();
            s->impl = std::make_unique<AudioStreamer>(static_cast<int>(port), std::string(my_id));
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
            const auto peers = s->impl->getPeerIds();
            return peers.size();
        }
        catch (...)
        {
            return 0;
        }
    }

    alc_status_t alc_streamer_get_peer_id(alc_streamer_t *s, size_t index, char *out, size_t out_len, size_t *required_len)
    {
        if (!s || !s->impl)
            return ALC_STATUS_INVALID_ARGUMENT;

        try
        {
            const auto peers = s->impl->getPeerIds();
            if (index >= peers.size())
            {
                setError(s, "index out of range");
                if (required_len)
                    *required_len = 0;
                return ALC_STATUS_INVALID_ARGUMENT;
            }

            return writeStringToOut(peers[index], out, out_len, required_len);
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
