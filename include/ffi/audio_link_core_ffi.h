#ifndef audio_link_core_ffi_h
#define audio_link_core_ffi_h

/*
  C ABI wrapper for using AudioStreamer via FFI.
  - macOS / Windows only (for now)
  - Polling style: call getters periodically from Dart.
*/

#include <stddef.h>
#include <stdint.h>

#if defined(_WIN32) || defined(_WIN64)
#define NOMINMAX
#if defined(AUDIO_LINK_CORE_FFI_EXPORTS)
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

  typedef struct alc_streamer alc_streamer_t;

  typedef enum alc_status
  {
    ALC_STATUS_OK = 0,
    ALC_STATUS_INVALID_ARGUMENT = 1,
    ALC_STATUS_INTERNAL_ERROR = 2
  } alc_status_t;

  ALC_FFI_API const char *alc_version_string(void);

  ALC_FFI_API alc_streamer_t *alc_streamer_create(int32_t port, const char *my_id, const char *my_name, const char *my_type);
  ALC_FFI_API void alc_streamer_destroy(alc_streamer_t *s);

  ALC_FFI_API alc_status_t alc_streamer_start(alc_streamer_t *s);
  ALC_FFI_API alc_status_t alc_streamer_stop(alc_streamer_t *s);

  ALC_FFI_API alc_status_t alc_streamer_start_sending_to(
      alc_streamer_t *s,
      const char *peer_id,
      const char *input_device_uid_nullable);

  ALC_FFI_API alc_status_t alc_streamer_stop_sending_to(alc_streamer_t *s, const char *peer_id);

  ALC_FFI_API alc_status_t alc_streamer_start_receiving_from(
      alc_streamer_t *s,
      const char *peer_id,
      const char *output_device_uid_nullable);

  ALC_FFI_API alc_status_t alc_streamer_stop_receiving_from(alc_streamer_t *s, const char *peer_id);

  ALC_FFI_API alc_status_t alc_streamer_change_output_device(
      alc_streamer_t *s,
      const char *peer_id,
      const char *output_device_uid_nullable);

  ALC_FFI_API alc_status_t alc_streamer_change_input_device(
      alc_streamer_t *s,
      const char *peer_id,
      const char *input_device_uid_nullable);

  /*
    Polling APIs
    - Use a 2-step approach to avoid heap allocations across FFI.
  */
  ALC_FFI_API size_t alc_streamer_get_peer_count(alc_streamer_t *s);

  /*
    Writes a JSON representation of Peer for `index` into `out` (NUL-terminated).
    If `out` is NULL or `out_len` is 0, returns required length including NUL via `required_len`.
  */
  ALC_FFI_API alc_status_t alc_streamer_get_peer_json(
      alc_streamer_t *s,
      size_t index,
      char *out,
      size_t out_len,
      size_t *required_len);

  /*
    Audio device polling APIs
    - Available on macOS / Windows.
  */
  ALC_FFI_API size_t alc_streamer_get_audio_device_count(alc_streamer_t *s);

  /*
    Writes a JSON representation of AudioDevice for `index` into `out` (NUL-terminated).
    If `out` is NULL or `out_len` is 0, returns required length including NUL via `required_len`.
  */
  ALC_FFI_API alc_status_t alc_streamer_get_audio_device_json(
      alc_streamer_t *s,
      size_t index,
      char *out,
      size_t out_len,
      size_t *required_len);

  /*
    Writes a JSON representation of the current default output device into `out` (NUL-terminated).
    If `out` is NULL or `out_len` is 0, returns required length including NUL via `required_len`.
  */
  ALC_FFI_API alc_status_t alc_streamer_get_default_output_device_json(
      alc_streamer_t *s,
      char *out,
      size_t out_len,
      size_t *required_len);

  /*
    Error retrieval
    - On any non-OK return, call this to get a human-readable message.
  */
  ALC_FFI_API alc_status_t alc_streamer_last_error(
      alc_streamer_t *s,
      char *out,
      size_t out_len,
      size_t *required_len);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // audio_link_core_ffi_h
