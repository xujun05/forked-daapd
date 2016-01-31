
#ifndef __OUTPUTS_H__
#define __OUTPUTS_H__

/* Outputs is a generic interface between the player and a media output method,
 * like for instance AirPlay (raop) or ALSA. The purpose of the interface is to
 * make it easier to add new outputs without messing too much with the player or
 * existing output methods.
 * 
 * An output method will have a general type, and it will be able to detect
 * supported devices that are available for output. A device will be typically
 * be something like an AirPlay speaker.
 *
 * When a device is started the output backend will typically create a session.
 * This session is only passed around as an opaque object in this interface.
 *
 */

// Must be in sync with outputs[] in outputs.c
enum output_types
{
  OUTPUT_TYPE_RAOP,
#ifdef CHROMECAST
  OUTPUT_TYPE_CAST,
#endif
/* TODO
  OUTPUT_TYPE_STREAMING,
  OUTPUT_TYPE_ALSA,
  OUTPUT_TYPE_OSS,
  OUTPUT_TYPE_DUMMY,
*/
};

/* Output session state */

// Session is starting up
#define OUTPUT_STATE_F_STARTUP    (1 << 15)
// Streaming is up (connection established)
#define OUTPUT_STATE_F_CONNECTED  (1 << 16)
enum output_device_state
{
  OUTPUT_STATE_STOPPED   = 0,

  // Session startup
  // TODO Make this less RAOP-specific
  OUTPUT_STATE_OPTIONS   = OUTPUT_STATE_F_STARTUP | 0x01,
  OUTPUT_STATE_ANNOUNCE  = OUTPUT_STATE_F_STARTUP | 0x02,
  OUTPUT_STATE_SETUP     = OUTPUT_STATE_F_STARTUP | 0x03,
  OUTPUT_STATE_RECORD    = OUTPUT_STATE_F_STARTUP | 0x04,

  // Session established
  // - streaming ready (RECORD sent and acked, connection established)
  // - commands (SET_PARAMETER) are possible
  OUTPUT_STATE_CONNECTED = OUTPUT_STATE_F_CONNECTED,

  // Audio data is being sent
  OUTPUT_STATE_STREAMING = OUTPUT_STATE_F_CONNECTED | 0x01,

  // Session is failed, couldn't startup or error occurred
  OUTPUT_STATE_FAILED    = -1,

  // Password issue: unknown password or bad password
  OUTPUT_STATE_PASSWORD  = -2,
};

/* Linked list of device info used by the player for each device
 */
struct output_device
{
  // Device id
  uint64_t id;

  // Name of the device, e.g. "Living Room"
  char *name;

  // Type of the device, will be used to determine which output backend to call
  enum output_types type;

  // Type of output (string)
  const char *type_name;

  // Misc device flags 
  unsigned selected:1;
  unsigned advertised:1;
  unsigned has_password:1;
  unsigned has_video:1;

  // Password if relevant
  const char *password;

  // Device volume
  int volume;
  int relvol;

  // Address
  char *v4_address;
  char *v6_address;
  short v4_port;
  short v6_port;

  // Opaque pointers to device and session data
  void *extra_device_info;
  struct output_session *session;

  struct output_device *next;
};

// Except for the type, sessions are opaque outside of the output backend
struct output_session
{
  enum output_types type;
  void *session;
};

// Linked list of metadata prepared by each output backend
struct output_metadata
{
  enum output_types type;
  void *metadata;
  struct output_metadata *next;
};

typedef void (*output_status_cb)(struct output_device *device, struct output_session *session, enum output_device_state status);

struct output_definition
{
  // Name of the output
  const char *name;

  // Type of output
  enum output_types type;

  // Priority to give this output when autoselecting an output, 1 is highest
  // 1 = highest priority, 0 = don't autoselect
  // TODO Not implemented yet
  int priority;

  // Set to 1 if the output initialization failed
  int disabled;

  // Initialization function called during startup
  // Output must call device_cb when an output device becomes available/unavailable
  int (*init)(void);

  // Deinitialization function called at shutdown
  void (*deinit)(void);

  // Prepare a playback session on device and call back
  int (*device_start)(struct output_device *device, output_status_cb cb, uint64_t rtptime);

  // Close a session prepared by device_start
  void (*device_stop)(struct output_session *session);

  // Test the connection to a device and call back
  int (*device_probe)(struct output_device *device, output_status_cb cb);

  // Free the private device data
  void (*device_free_extra)(struct output_device *device);

  // Set the volume and call back
  int (*device_volume_set)(struct output_device *device, output_status_cb cb);

  // Start/stop playback on devices that were started
  void (*playback_start)(uint64_t next_pkt, struct timespec *ts);
  void (*playback_stop)(void);

  // Write stream data to the output devices
  void (*write)(uint8_t *buf, uint64_t rtptime);

  // Flush all sessions, the return must be number of sessions pending the flush
  int (*flush)(output_status_cb cb, uint64_t rtptime);

  // Change the call back associated with a session
  void (*status_cb)(struct output_session *session, output_status_cb cb);

  // Metadata
  void *(*metadata_prepare)(int id);
  void (*metadata_send)(void *metadata, uint64_t rtptime, uint64_t offset, int startup);
  void (*metadata_purge)(void);
  void (*metadata_prune)(uint64_t rtptime);
};

int
outputs_device_start(struct output_device *device, output_status_cb cb, uint64_t rtptime);

void
outputs_device_stop(struct output_session *session);

int
outputs_device_probe(struct output_device *device, output_status_cb cb);

void
outputs_device_free(struct output_device *device);

int
outputs_device_volume_set(struct output_device *device, output_status_cb cb);

void
outputs_playback_start(uint64_t next_pkt, struct timespec *ts);

void
outputs_playback_stop(void);

void
outputs_write(uint8_t *buf, uint64_t rtptime);

int
outputs_flush(output_status_cb cb, uint64_t rtptime);

void
outputs_status_cb(struct output_session *session, output_status_cb cb);

struct output_metadata *
outputs_metadata_prepare(int id);

void
outputs_metadata_send(struct output_metadata *omd, uint64_t rtptime, uint64_t offset, int startup);

void
outputs_metadata_purge(void);

void
outputs_metadata_prune(uint64_t rtptime);

void
outputs_metadata_free(struct output_metadata *omd);

const char *
outputs_name(enum output_types type);

int
outputs_init(void);

void
outputs_deinit(void);

#endif /* !__OUTPUTS_H__ */