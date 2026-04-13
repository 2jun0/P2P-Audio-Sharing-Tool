#ifndef audio_link_core_ffi_h
#define audio_link_core_ffi_h

/*
  C ABI wrapper for AudioSender, AudioReceiver, LookupService, and AudioDeviceManager.
  Each module is independent and can be used separately.
*/

#include <stddef.h>
#include <stdint.h>

#if defined(_WIN32) || defined(_WIN64)
#define NOMINMAX
#if defined(AUDIO_LINK_CORE_DLL_EXPORTS)
#define ALC_FFI_API __declspec(dllexport)
#else
#define ALC_FFI_API __declspec(dllimport)
#endif
#elif defined(__APPLE__)
#define ALC_FFI_API __attribute__((visibility("default")))
#else
#define ALC_FFI_API
#endif

#ifdef __cplusplus
extern "C"
{
#endif

  typedef enum alc_status
  {
    ALC_STATUS_OK = 0,
    ALC_STATUS_INVALID_ARGUMENT = 1,
    ALC_STATUS_INTERNAL_ERROR = 2
  } alc_status_t;

  ALC_FFI_API const char *alc_version_string(void);

  /* ── GStreamer ── */

  ALC_FFI_API void alc_gst_init(void);

  /* ── LookupService ── */

  typedef struct alc_lookup alc_lookup_t;

  ALC_FFI_API alc_lookup_t *alc_lookup_create(int32_t port, const char *my_id, const char *my_name, const char *my_type);
  ALC_FFI_API void alc_lookup_destroy(alc_lookup_t *lk);

  ALC_FFI_API alc_status_t alc_lookup_start(alc_lookup_t *lk);
  ALC_FFI_API alc_status_t alc_lookup_stop(alc_lookup_t *lk);

  ALC_FFI_API size_t alc_lookup_get_peer_count(alc_lookup_t *lk);
  ALC_FFI_API alc_status_t alc_lookup_get_peer_json(
      alc_lookup_t *lk,
      size_t index,
      char *out,
      size_t out_len,
      size_t *required_len);

  /* ── StreamManager ── */

  typedef struct alc_stream_manager alc_stream_manager_t;

  ALC_FFI_API alc_stream_manager_t *alc_stream_manager_create(void);
  ALC_FFI_API void alc_stream_manager_destroy(alc_stream_manager_t *sm);

  ALC_FFI_API alc_status_t alc_stream_manager_add_sender(
      alc_stream_manager_t *sm,
      const char *id,
      const char *target_host,
      int32_t target_port,
      const char *input_device_uid_nullable);
  ALC_FFI_API alc_status_t alc_stream_manager_remove_sender(alc_stream_manager_t *sm, const char *id);
  ALC_FFI_API alc_status_t alc_stream_manager_remove_all_senders(alc_stream_manager_t *sm);

  ALC_FFI_API alc_status_t alc_stream_manager_add_receiver(
      alc_stream_manager_t *sm,
      const char *id,
      const char *output_device_uid_nullable);
  ALC_FFI_API alc_status_t alc_stream_manager_remove_receiver(alc_stream_manager_t *sm, const char *id);
  ALC_FFI_API alc_status_t alc_stream_manager_remove_all_receivers(alc_stream_manager_t *sm);

  ALC_FFI_API int32_t alc_stream_manager_get_receiver_port(alc_stream_manager_t *sm, const char *id);
  ALC_FFI_API int32_t alc_stream_manager_has_sender(alc_stream_manager_t *sm, const char *id);
  ALC_FFI_API int32_t alc_stream_manager_has_receiver(alc_stream_manager_t *sm, const char *id);

  /* ── AudioDeviceManager ── */

  ALC_FFI_API size_t alc_device_get_count(void);
  ALC_FFI_API alc_status_t alc_device_get_json(
      size_t index,
      char *out,
      size_t out_len,
      size_t *required_len);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // audio_link_core_ffi_h
