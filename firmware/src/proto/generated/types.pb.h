/* Automatically generated nanopb header */
/* Generated by nanopb-0.4.9 */

#ifndef PB_TYPES_PB_H_INCLUDED
#define PB_TYPES_PB_H_INCLUDED
#include <pb.h>

#if PB_PROTO_HEADER_VERSION != 40
#error Regenerate this file with the current version of nanopb generator.
#endif

/* Enum definitions */
typedef enum _Constants {
    CONSTANT_UNSPECIFIED = 0,
    /* Initial temperature for all profiles */
    START_TEMPERATURE = 30,
    /* Static sizes for repeated/maps */
    MAX_REFLOW_PROFILES = 10,
    MAX_REFLOW_SEGMENTS = 10,
    MAX_HISTORY_CHUNK = 100,
    /* History IDs for tasks (selected to not conflict with profile IDs) */
    HISTORY_ID_SENSOR_BAKE_MODE = 4000,
    HISTORY_ID_ADRC_TEST_MODE = 4001,
    HISTORY_ID_STEP_RESPONSE = 4002
} Constants;

typedef enum _DeviceState {
    DeviceState_Init = 0,
    DeviceState_Idle = 1,
    DeviceState_Reflow = 2,
    DeviceState_SensorBake = 3,
    DeviceState_AdrcTest = 4,
    DeviceState_StepResponse = 5,
    DeviceState_Bonding = 6,
    DeviceState_NumberOfStates = 7
} DeviceState;

/* Struct definitions */
typedef struct _DeviceStatus {
    /* Main */
    DeviceState state;
    bool hotplate_connected;
    uint32_t hotplate_id;
    float temperature;
    /* Debug info */
    float watts;
    float volts;
    float amperes;
    float max_watts;
    float duty_cycle; /* 0..1 */
    float resistance;
} DeviceStatus;

typedef struct _Segment {
    /* Target temperature in Celsius */
    int32_t target;
    /* Duration in seconds */
    int32_t duration;
} Segment;

typedef struct _Profile {
    /* Unique profile identifier */
    int32_t id;
    /* Profile name */
    char name[51];
    /* Temperature segments sequence */
    pb_size_t segments_count;
    Segment segments[10];
} Profile;

typedef struct _ProfilesData {
    /* Available profiles */
    pb_size_t items_count;
    Profile items[10];
    /* Currently selected profile id */
    int32_t selectedId;
} ProfilesData;

typedef struct _Point {
    float x;
    float y;
} Point;

typedef struct _HistoryChunk {
    int32_t type;
    int32_t version;
    pb_size_t data_count;
    Point data[100];
} HistoryChunk;

typedef struct _AdrcParams {
    /* System response time (when temperature reaches 63% of final value) */
    float response;
    /* Scale. Max derivative / power */
    float b0;
    /* ω_observer = N / τ. Usually 3..10
 5 is good for the start. Increase until oscillates, then back 10-20%. */
    float N;
    /* ω_controller = ω_observer / M. Usually 2..5
 3 is a good for the start. Probably, changes not required. */
    float M;
} AdrcParams;

typedef struct _SensorParams {
    float p0_temperature;
    float p0_value;
    float p1_temperature;
    float p1_value;
} SensorParams;


#ifdef __cplusplus
extern "C" {
#endif

/* Helper constants for enums */
#define _Constants_MIN CONSTANT_UNSPECIFIED
#define _Constants_MAX HISTORY_ID_STEP_RESPONSE
#define _Constants_ARRAYSIZE ((Constants)(HISTORY_ID_STEP_RESPONSE+1))
#define Constants_CONSTANT_UNSPECIFIED CONSTANT_UNSPECIFIED
#define Constants_START_TEMPERATURE START_TEMPERATURE
#define Constants_MAX_REFLOW_PROFILES MAX_REFLOW_PROFILES
#define Constants_MAX_REFLOW_SEGMENTS MAX_REFLOW_SEGMENTS
#define Constants_MAX_HISTORY_CHUNK MAX_HISTORY_CHUNK
#define Constants_HISTORY_ID_SENSOR_BAKE_MODE HISTORY_ID_SENSOR_BAKE_MODE
#define Constants_HISTORY_ID_ADRC_TEST_MODE HISTORY_ID_ADRC_TEST_MODE
#define Constants_HISTORY_ID_STEP_RESPONSE HISTORY_ID_STEP_RESPONSE

#define _DeviceState_MIN DeviceState_Init
#define _DeviceState_MAX DeviceState_NumberOfStates
#define _DeviceState_ARRAYSIZE ((DeviceState)(DeviceState_NumberOfStates+1))

#define DeviceStatus_state_ENUMTYPE DeviceState









/* Initializer values for message structs */
#define DeviceStatus_init_default                {_DeviceState_MIN, 0, 0, 0, 0, 0, 0, 0, 0, 0}
#define Segment_init_default                     {0, 0}
#define Profile_init_default                     {0, "", 0, {Segment_init_default, Segment_init_default, Segment_init_default, Segment_init_default, Segment_init_default, Segment_init_default, Segment_init_default, Segment_init_default, Segment_init_default, Segment_init_default}}
#define ProfilesData_init_default                {0, {Profile_init_default, Profile_init_default, Profile_init_default, Profile_init_default, Profile_init_default, Profile_init_default, Profile_init_default, Profile_init_default, Profile_init_default, Profile_init_default}, 0}
#define Point_init_default                       {0, 0}
#define HistoryChunk_init_default                {0, 0, 0, {Point_init_default, Point_init_default, Point_init_default, Point_init_default, Point_init_default, Point_init_default, Point_init_default, Point_init_default, Point_init_default, Point_init_default, Point_init_default, Point_init_default, Point_init_default, Point_init_default, Point_init_default, Point_init_default, Point_init_default, Point_init_default, Point_init_default, Point_init_default, Point_init_default, Point_init_default, Point_init_default, Point_init_default, Point_init_default, Point_init_default, Point_init_default, Point_init_default, Point_init_default, Point_init_default, Point_init_default, Point_init_default, Point_init_default, Point_init_default, Point_init_default, Point_init_default, Point_init_default, Point_init_default, Point_init_default, Point_init_default, Point_init_default, Point_init_default, Point_init_default, Point_init_default, Point_init_default, Point_init_default, Point_init_default, Point_init_default, Point_init_default, Point_init_default, Point_init_default, Point_init_default, Point_init_default, Point_init_default, Point_init_default, Point_init_default, Point_init_default, Point_init_default, Point_init_default, Point_init_default, Point_init_default, Point_init_default, Point_init_default, Point_init_default, Point_init_default, Point_init_default, Point_init_default, Point_init_default, Point_init_default, Point_init_default, Point_init_default, Point_init_default, Point_init_default, Point_init_default, Point_init_default, Point_init_default, Point_init_default, Point_init_default, Point_init_default, Point_init_default, Point_init_default, Point_init_default, Point_init_default, Point_init_default, Point_init_default, Point_init_default, Point_init_default, Point_init_default, Point_init_default, Point_init_default, Point_init_default, Point_init_default, Point_init_default, Point_init_default, Point_init_default, Point_init_default, Point_init_default, Point_init_default, Point_init_default, Point_init_default}}
#define AdrcParams_init_default                  {0, 0, 0, 0}
#define SensorParams_init_default                {0, 0, 0, 0}
#define DeviceStatus_init_zero                   {_DeviceState_MIN, 0, 0, 0, 0, 0, 0, 0, 0, 0}
#define Segment_init_zero                        {0, 0}
#define Profile_init_zero                        {0, "", 0, {Segment_init_zero, Segment_init_zero, Segment_init_zero, Segment_init_zero, Segment_init_zero, Segment_init_zero, Segment_init_zero, Segment_init_zero, Segment_init_zero, Segment_init_zero}}
#define ProfilesData_init_zero                   {0, {Profile_init_zero, Profile_init_zero, Profile_init_zero, Profile_init_zero, Profile_init_zero, Profile_init_zero, Profile_init_zero, Profile_init_zero, Profile_init_zero, Profile_init_zero}, 0}
#define Point_init_zero                          {0, 0}
#define HistoryChunk_init_zero                   {0, 0, 0, {Point_init_zero, Point_init_zero, Point_init_zero, Point_init_zero, Point_init_zero, Point_init_zero, Point_init_zero, Point_init_zero, Point_init_zero, Point_init_zero, Point_init_zero, Point_init_zero, Point_init_zero, Point_init_zero, Point_init_zero, Point_init_zero, Point_init_zero, Point_init_zero, Point_init_zero, Point_init_zero, Point_init_zero, Point_init_zero, Point_init_zero, Point_init_zero, Point_init_zero, Point_init_zero, Point_init_zero, Point_init_zero, Point_init_zero, Point_init_zero, Point_init_zero, Point_init_zero, Point_init_zero, Point_init_zero, Point_init_zero, Point_init_zero, Point_init_zero, Point_init_zero, Point_init_zero, Point_init_zero, Point_init_zero, Point_init_zero, Point_init_zero, Point_init_zero, Point_init_zero, Point_init_zero, Point_init_zero, Point_init_zero, Point_init_zero, Point_init_zero, Point_init_zero, Point_init_zero, Point_init_zero, Point_init_zero, Point_init_zero, Point_init_zero, Point_init_zero, Point_init_zero, Point_init_zero, Point_init_zero, Point_init_zero, Point_init_zero, Point_init_zero, Point_init_zero, Point_init_zero, Point_init_zero, Point_init_zero, Point_init_zero, Point_init_zero, Point_init_zero, Point_init_zero, Point_init_zero, Point_init_zero, Point_init_zero, Point_init_zero, Point_init_zero, Point_init_zero, Point_init_zero, Point_init_zero, Point_init_zero, Point_init_zero, Point_init_zero, Point_init_zero, Point_init_zero, Point_init_zero, Point_init_zero, Point_init_zero, Point_init_zero, Point_init_zero, Point_init_zero, Point_init_zero, Point_init_zero, Point_init_zero, Point_init_zero, Point_init_zero, Point_init_zero, Point_init_zero, Point_init_zero, Point_init_zero, Point_init_zero}}
#define AdrcParams_init_zero                     {0, 0, 0, 0}
#define SensorParams_init_zero                   {0, 0, 0, 0}

/* Field tags (for use in manual encoding/decoding) */
#define DeviceStatus_state_tag                   1
#define DeviceStatus_hotplate_connected_tag      2
#define DeviceStatus_hotplate_id_tag             3
#define DeviceStatus_temperature_tag             4
#define DeviceStatus_watts_tag                   5
#define DeviceStatus_volts_tag                   6
#define DeviceStatus_amperes_tag                 7
#define DeviceStatus_max_watts_tag               8
#define DeviceStatus_duty_cycle_tag              9
#define DeviceStatus_resistance_tag              10
#define Segment_target_tag                       1
#define Segment_duration_tag                     2
#define Profile_id_tag                           1
#define Profile_name_tag                         2
#define Profile_segments_tag                     3
#define ProfilesData_items_tag                   1
#define ProfilesData_selectedId_tag              2
#define Point_x_tag                              1
#define Point_y_tag                              2
#define HistoryChunk_type_tag                    1
#define HistoryChunk_version_tag                 2
#define HistoryChunk_data_tag                    3
#define AdrcParams_response_tag                  1
#define AdrcParams_b0_tag                        2
#define AdrcParams_N_tag                         3
#define AdrcParams_M_tag                         4
#define SensorParams_p0_temperature_tag          1
#define SensorParams_p0_value_tag                2
#define SensorParams_p1_temperature_tag          3
#define SensorParams_p1_value_tag                4

/* Struct field encoding specification for nanopb */
#define DeviceStatus_FIELDLIST(X, a) \
X(a, STATIC,   SINGULAR, UENUM,    state,             1) \
X(a, STATIC,   SINGULAR, BOOL,     hotplate_connected,   2) \
X(a, STATIC,   SINGULAR, UINT32,   hotplate_id,       3) \
X(a, STATIC,   SINGULAR, FLOAT,    temperature,       4) \
X(a, STATIC,   SINGULAR, FLOAT,    watts,             5) \
X(a, STATIC,   SINGULAR, FLOAT,    volts,             6) \
X(a, STATIC,   SINGULAR, FLOAT,    amperes,           7) \
X(a, STATIC,   SINGULAR, FLOAT,    max_watts,         8) \
X(a, STATIC,   SINGULAR, FLOAT,    duty_cycle,        9) \
X(a, STATIC,   SINGULAR, FLOAT,    resistance,       10)
#define DeviceStatus_CALLBACK NULL
#define DeviceStatus_DEFAULT NULL

#define Segment_FIELDLIST(X, a) \
X(a, STATIC,   SINGULAR, INT32,    target,            1) \
X(a, STATIC,   SINGULAR, INT32,    duration,          2)
#define Segment_CALLBACK NULL
#define Segment_DEFAULT NULL

#define Profile_FIELDLIST(X, a) \
X(a, STATIC,   SINGULAR, INT32,    id,                1) \
X(a, STATIC,   SINGULAR, STRING,   name,              2) \
X(a, STATIC,   REPEATED, MESSAGE,  segments,          3)
#define Profile_CALLBACK NULL
#define Profile_DEFAULT NULL
#define Profile_segments_MSGTYPE Segment

#define ProfilesData_FIELDLIST(X, a) \
X(a, STATIC,   REPEATED, MESSAGE,  items,             1) \
X(a, STATIC,   SINGULAR, INT32,    selectedId,        2)
#define ProfilesData_CALLBACK NULL
#define ProfilesData_DEFAULT NULL
#define ProfilesData_items_MSGTYPE Profile

#define Point_FIELDLIST(X, a) \
X(a, STATIC,   SINGULAR, FLOAT,    x,                 1) \
X(a, STATIC,   SINGULAR, FLOAT,    y,                 2)
#define Point_CALLBACK NULL
#define Point_DEFAULT NULL

#define HistoryChunk_FIELDLIST(X, a) \
X(a, STATIC,   SINGULAR, INT32,    type,              1) \
X(a, STATIC,   SINGULAR, INT32,    version,           2) \
X(a, STATIC,   REPEATED, MESSAGE,  data,              3)
#define HistoryChunk_CALLBACK NULL
#define HistoryChunk_DEFAULT NULL
#define HistoryChunk_data_MSGTYPE Point

#define AdrcParams_FIELDLIST(X, a) \
X(a, STATIC,   SINGULAR, FLOAT,    response,          1) \
X(a, STATIC,   SINGULAR, FLOAT,    b0,                2) \
X(a, STATIC,   SINGULAR, FLOAT,    N,                 3) \
X(a, STATIC,   SINGULAR, FLOAT,    M,                 4)
#define AdrcParams_CALLBACK NULL
#define AdrcParams_DEFAULT NULL

#define SensorParams_FIELDLIST(X, a) \
X(a, STATIC,   SINGULAR, FLOAT,    p0_temperature,    1) \
X(a, STATIC,   SINGULAR, FLOAT,    p0_value,          2) \
X(a, STATIC,   SINGULAR, FLOAT,    p1_temperature,    3) \
X(a, STATIC,   SINGULAR, FLOAT,    p1_value,          4)
#define SensorParams_CALLBACK NULL
#define SensorParams_DEFAULT NULL

extern const pb_msgdesc_t DeviceStatus_msg;
extern const pb_msgdesc_t Segment_msg;
extern const pb_msgdesc_t Profile_msg;
extern const pb_msgdesc_t ProfilesData_msg;
extern const pb_msgdesc_t Point_msg;
extern const pb_msgdesc_t HistoryChunk_msg;
extern const pb_msgdesc_t AdrcParams_msg;
extern const pb_msgdesc_t SensorParams_msg;

/* Defines for backwards compatibility with code written before nanopb-0.4.0 */
#define DeviceStatus_fields &DeviceStatus_msg
#define Segment_fields &Segment_msg
#define Profile_fields &Profile_msg
#define ProfilesData_fields &ProfilesData_msg
#define Point_fields &Point_msg
#define HistoryChunk_fields &HistoryChunk_msg
#define AdrcParams_fields &AdrcParams_msg
#define SensorParams_fields &SensorParams_msg

/* Maximum encoded size of messages (where known) */
#define AdrcParams_size                          20
#define DeviceStatus_size                        45
#define HistoryChunk_size                        1222
#define Point_size                               10
#define Profile_size                             303
#define ProfilesData_size                        3071
#define Segment_size                             22
#define SensorParams_size                        20
#define TYPES_PB_H_MAX_SIZE                      ProfilesData_size

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif
