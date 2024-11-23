// Code generated by protoc-gen-ts_proto. DO NOT EDIT.
// versions:
//   protoc-gen-ts_proto  v2.2.5
//   protoc               v3.20.3
// source: types.proto

/* eslint-disable */
import { BinaryReader, BinaryWriter } from "@bufbuild/protobuf/wire";

export const protobufPackage = "";

export enum Constants {
  CONSTANT_UNSPECIFIED = 0,
  /** START_TEMPERATURE - Initial temperature for all profiles */
  START_TEMPERATURE = 30,
  /** MAX_REFLOW_PROFILES - Static sizes for repeated/maps */
  MAX_REFLOW_PROFILES = 10,
  UNRECOGNIZED = -1,
}

export function constantsFromJSON(object: any): Constants {
  switch (object) {
    case 0:
    case "CONSTANT_UNSPECIFIED":
      return Constants.CONSTANT_UNSPECIFIED;
    case 30:
    case "START_TEMPERATURE":
      return Constants.START_TEMPERATURE;
    case 10:
    case "MAX_REFLOW_PROFILES":
      return Constants.MAX_REFLOW_PROFILES;
    case -1:
    case "UNRECOGNIZED":
    default:
      return Constants.UNRECOGNIZED;
  }
}

export function constantsToJSON(object: Constants): string {
  switch (object) {
    case Constants.CONSTANT_UNSPECIFIED:
      return "CONSTANT_UNSPECIFIED";
    case Constants.START_TEMPERATURE:
      return "START_TEMPERATURE";
    case Constants.MAX_REFLOW_PROFILES:
      return "MAX_REFLOW_PROFILES";
    case Constants.UNRECOGNIZED:
    default:
      return "UNRECOGNIZED";
  }
}

export enum DeviceState {
  DEVICE_STATE_UNSPECIFIED = 0,
  Idle = 1,
  Reflow = 2,
  SensorBake = 3,
  AdrcTest = 4,
  StepResponse = 5,
  UNRECOGNIZED = -1,
}

export function deviceStateFromJSON(object: any): DeviceState {
  switch (object) {
    case 0:
    case "DEVICE_STATE_UNSPECIFIED":
      return DeviceState.DEVICE_STATE_UNSPECIFIED;
    case 1:
    case "Idle":
      return DeviceState.Idle;
    case 2:
    case "Reflow":
      return DeviceState.Reflow;
    case 3:
    case "SensorBake":
      return DeviceState.SensorBake;
    case 4:
    case "AdrcTest":
      return DeviceState.AdrcTest;
    case 5:
    case "StepResponse":
      return DeviceState.StepResponse;
    case -1:
    case "UNRECOGNIZED":
    default:
      return DeviceState.UNRECOGNIZED;
  }
}

export function deviceStateToJSON(object: DeviceState): string {
  switch (object) {
    case DeviceState.DEVICE_STATE_UNSPECIFIED:
      return "DEVICE_STATE_UNSPECIFIED";
    case DeviceState.Idle:
      return "Idle";
    case DeviceState.Reflow:
      return "Reflow";
    case DeviceState.SensorBake:
      return "SensorBake";
    case DeviceState.AdrcTest:
      return "AdrcTest";
    case DeviceState.StepResponse:
      return "StepResponse";
    case DeviceState.UNRECOGNIZED:
    default:
      return "UNRECOGNIZED";
  }
}

export interface Segment {
  /** Target temperature in Celsius */
  target: number;
  /** Duration in seconds */
  duration: number;
}

export interface Profile {
  /** Unique profile identifier */
  id: number;
  /** Profile name */
  name: string;
  /** Temperature segments sequence */
  segments: Segment[];
}

export interface ProfilesData {
  /** Available profiles */
  items: Profile[];
  /** Currently selected profile id */
  selectedId: number;
}

export interface Point {
  x: number;
  y: number;
}

export interface HistoryChunk {
  type: number;
  version: number;
  data: Point[];
}

export interface AdrcParams {
  /** System response time (when temperature reaches 63% of final value) */
  response: number;
  /** Scale. Max derivative / power */
  b0: number;
  /**
   * ω_observer = N / τ. Usually 3..10
   * 5 is good for the start. Increase until oscillates, then back 10-20%.
   */
  N: number;
  /**
   * ω_controller = ω_observer / M. Usually 2..5
   * 3 is a good for the start. Probably, changes not required.
   */
  M: number;
}

export interface SensorParams {
  p0_temperature: number;
  p0_value: number;
  p1_temperature: number;
  p1_value: number;
}

export interface HeaterParams {
  adrc: AdrcParams | undefined;
  sensor: SensorParams | undefined;
}

function createBaseSegment(): Segment {
  return { target: 0, duration: 0 };
}

export const Segment: MessageFns<Segment> = {
  encode(message: Segment, writer: BinaryWriter = new BinaryWriter()): BinaryWriter {
    if (message.target !== 0) {
      writer.uint32(8).int32(message.target);
    }
    if (message.duration !== 0) {
      writer.uint32(16).int32(message.duration);
    }
    return writer;
  },

  decode(input: BinaryReader | Uint8Array, length?: number): Segment {
    const reader = input instanceof BinaryReader ? input : new BinaryReader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseSegment();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1: {
          if (tag !== 8) {
            break;
          }

          message.target = reader.int32();
          continue;
        }
        case 2: {
          if (tag !== 16) {
            break;
          }

          message.duration = reader.int32();
          continue;
        }
      }
      if ((tag & 7) === 4 || tag === 0) {
        break;
      }
      reader.skip(tag & 7);
    }
    return message;
  },

  fromJSON(object: any): Segment {
    return {
      target: isSet(object.target) ? globalThis.Number(object.target) : 0,
      duration: isSet(object.duration) ? globalThis.Number(object.duration) : 0,
    };
  },

  toJSON(message: Segment): unknown {
    const obj: any = {};
    if (message.target !== 0) {
      obj.target = Math.round(message.target);
    }
    if (message.duration !== 0) {
      obj.duration = Math.round(message.duration);
    }
    return obj;
  },

  create<I extends Exact<DeepPartial<Segment>, I>>(base?: I): Segment {
    return Segment.fromPartial(base ?? ({} as any));
  },
  fromPartial<I extends Exact<DeepPartial<Segment>, I>>(object: I): Segment {
    const message = createBaseSegment();
    message.target = object.target ?? 0;
    message.duration = object.duration ?? 0;
    return message;
  },
};

function createBaseProfile(): Profile {
  return { id: 0, name: "", segments: [] };
}

export const Profile: MessageFns<Profile> = {
  encode(message: Profile, writer: BinaryWriter = new BinaryWriter()): BinaryWriter {
    if (message.id !== 0) {
      writer.uint32(8).int32(message.id);
    }
    if (message.name !== "") {
      writer.uint32(18).string(message.name);
    }
    for (const v of message.segments) {
      Segment.encode(v!, writer.uint32(26).fork()).join();
    }
    return writer;
  },

  decode(input: BinaryReader | Uint8Array, length?: number): Profile {
    const reader = input instanceof BinaryReader ? input : new BinaryReader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseProfile();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1: {
          if (tag !== 8) {
            break;
          }

          message.id = reader.int32();
          continue;
        }
        case 2: {
          if (tag !== 18) {
            break;
          }

          message.name = reader.string();
          continue;
        }
        case 3: {
          if (tag !== 26) {
            break;
          }

          message.segments.push(Segment.decode(reader, reader.uint32()));
          continue;
        }
      }
      if ((tag & 7) === 4 || tag === 0) {
        break;
      }
      reader.skip(tag & 7);
    }
    return message;
  },

  fromJSON(object: any): Profile {
    return {
      id: isSet(object.id) ? globalThis.Number(object.id) : 0,
      name: isSet(object.name) ? globalThis.String(object.name) : "",
      segments: globalThis.Array.isArray(object?.segments) ? object.segments.map((e: any) => Segment.fromJSON(e)) : [],
    };
  },

  toJSON(message: Profile): unknown {
    const obj: any = {};
    if (message.id !== 0) {
      obj.id = Math.round(message.id);
    }
    if (message.name !== "") {
      obj.name = message.name;
    }
    if (message.segments?.length) {
      obj.segments = message.segments.map((e) => Segment.toJSON(e));
    }
    return obj;
  },

  create<I extends Exact<DeepPartial<Profile>, I>>(base?: I): Profile {
    return Profile.fromPartial(base ?? ({} as any));
  },
  fromPartial<I extends Exact<DeepPartial<Profile>, I>>(object: I): Profile {
    const message = createBaseProfile();
    message.id = object.id ?? 0;
    message.name = object.name ?? "";
    message.segments = object.segments?.map((e) => Segment.fromPartial(e)) || [];
    return message;
  },
};

function createBaseProfilesData(): ProfilesData {
  return { items: [], selectedId: 0 };
}

export const ProfilesData: MessageFns<ProfilesData> = {
  encode(message: ProfilesData, writer: BinaryWriter = new BinaryWriter()): BinaryWriter {
    for (const v of message.items) {
      Profile.encode(v!, writer.uint32(10).fork()).join();
    }
    if (message.selectedId !== 0) {
      writer.uint32(16).int32(message.selectedId);
    }
    return writer;
  },

  decode(input: BinaryReader | Uint8Array, length?: number): ProfilesData {
    const reader = input instanceof BinaryReader ? input : new BinaryReader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseProfilesData();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1: {
          if (tag !== 10) {
            break;
          }

          message.items.push(Profile.decode(reader, reader.uint32()));
          continue;
        }
        case 2: {
          if (tag !== 16) {
            break;
          }

          message.selectedId = reader.int32();
          continue;
        }
      }
      if ((tag & 7) === 4 || tag === 0) {
        break;
      }
      reader.skip(tag & 7);
    }
    return message;
  },

  fromJSON(object: any): ProfilesData {
    return {
      items: globalThis.Array.isArray(object?.items) ? object.items.map((e: any) => Profile.fromJSON(e)) : [],
      selectedId: isSet(object.selectedId) ? globalThis.Number(object.selectedId) : 0,
    };
  },

  toJSON(message: ProfilesData): unknown {
    const obj: any = {};
    if (message.items?.length) {
      obj.items = message.items.map((e) => Profile.toJSON(e));
    }
    if (message.selectedId !== 0) {
      obj.selectedId = Math.round(message.selectedId);
    }
    return obj;
  },

  create<I extends Exact<DeepPartial<ProfilesData>, I>>(base?: I): ProfilesData {
    return ProfilesData.fromPartial(base ?? ({} as any));
  },
  fromPartial<I extends Exact<DeepPartial<ProfilesData>, I>>(object: I): ProfilesData {
    const message = createBaseProfilesData();
    message.items = object.items?.map((e) => Profile.fromPartial(e)) || [];
    message.selectedId = object.selectedId ?? 0;
    return message;
  },
};

function createBasePoint(): Point {
  return { x: 0, y: 0 };
}

export const Point: MessageFns<Point> = {
  encode(message: Point, writer: BinaryWriter = new BinaryWriter()): BinaryWriter {
    if (message.x !== 0) {
      writer.uint32(8).int32(message.x);
    }
    if (message.y !== 0) {
      writer.uint32(16).int32(message.y);
    }
    return writer;
  },

  decode(input: BinaryReader | Uint8Array, length?: number): Point {
    const reader = input instanceof BinaryReader ? input : new BinaryReader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBasePoint();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1: {
          if (tag !== 8) {
            break;
          }

          message.x = reader.int32();
          continue;
        }
        case 2: {
          if (tag !== 16) {
            break;
          }

          message.y = reader.int32();
          continue;
        }
      }
      if ((tag & 7) === 4 || tag === 0) {
        break;
      }
      reader.skip(tag & 7);
    }
    return message;
  },

  fromJSON(object: any): Point {
    return {
      x: isSet(object.x) ? globalThis.Number(object.x) : 0,
      y: isSet(object.y) ? globalThis.Number(object.y) : 0,
    };
  },

  toJSON(message: Point): unknown {
    const obj: any = {};
    if (message.x !== 0) {
      obj.x = Math.round(message.x);
    }
    if (message.y !== 0) {
      obj.y = Math.round(message.y);
    }
    return obj;
  },

  create<I extends Exact<DeepPartial<Point>, I>>(base?: I): Point {
    return Point.fromPartial(base ?? ({} as any));
  },
  fromPartial<I extends Exact<DeepPartial<Point>, I>>(object: I): Point {
    const message = createBasePoint();
    message.x = object.x ?? 0;
    message.y = object.y ?? 0;
    return message;
  },
};

function createBaseHistoryChunk(): HistoryChunk {
  return { type: 0, version: 0, data: [] };
}

export const HistoryChunk: MessageFns<HistoryChunk> = {
  encode(message: HistoryChunk, writer: BinaryWriter = new BinaryWriter()): BinaryWriter {
    if (message.type !== 0) {
      writer.uint32(8).int32(message.type);
    }
    if (message.version !== 0) {
      writer.uint32(16).int32(message.version);
    }
    for (const v of message.data) {
      Point.encode(v!, writer.uint32(26).fork()).join();
    }
    return writer;
  },

  decode(input: BinaryReader | Uint8Array, length?: number): HistoryChunk {
    const reader = input instanceof BinaryReader ? input : new BinaryReader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseHistoryChunk();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1: {
          if (tag !== 8) {
            break;
          }

          message.type = reader.int32();
          continue;
        }
        case 2: {
          if (tag !== 16) {
            break;
          }

          message.version = reader.int32();
          continue;
        }
        case 3: {
          if (tag !== 26) {
            break;
          }

          message.data.push(Point.decode(reader, reader.uint32()));
          continue;
        }
      }
      if ((tag & 7) === 4 || tag === 0) {
        break;
      }
      reader.skip(tag & 7);
    }
    return message;
  },

  fromJSON(object: any): HistoryChunk {
    return {
      type: isSet(object.type) ? globalThis.Number(object.type) : 0,
      version: isSet(object.version) ? globalThis.Number(object.version) : 0,
      data: globalThis.Array.isArray(object?.data) ? object.data.map((e: any) => Point.fromJSON(e)) : [],
    };
  },

  toJSON(message: HistoryChunk): unknown {
    const obj: any = {};
    if (message.type !== 0) {
      obj.type = Math.round(message.type);
    }
    if (message.version !== 0) {
      obj.version = Math.round(message.version);
    }
    if (message.data?.length) {
      obj.data = message.data.map((e) => Point.toJSON(e));
    }
    return obj;
  },

  create<I extends Exact<DeepPartial<HistoryChunk>, I>>(base?: I): HistoryChunk {
    return HistoryChunk.fromPartial(base ?? ({} as any));
  },
  fromPartial<I extends Exact<DeepPartial<HistoryChunk>, I>>(object: I): HistoryChunk {
    const message = createBaseHistoryChunk();
    message.type = object.type ?? 0;
    message.version = object.version ?? 0;
    message.data = object.data?.map((e) => Point.fromPartial(e)) || [];
    return message;
  },
};

function createBaseAdrcParams(): AdrcParams {
  return { response: 0, b0: 0, N: 0, M: 0 };
}

export const AdrcParams: MessageFns<AdrcParams> = {
  encode(message: AdrcParams, writer: BinaryWriter = new BinaryWriter()): BinaryWriter {
    if (message.response !== 0) {
      writer.uint32(13).float(message.response);
    }
    if (message.b0 !== 0) {
      writer.uint32(21).float(message.b0);
    }
    if (message.N !== 0) {
      writer.uint32(29).float(message.N);
    }
    if (message.M !== 0) {
      writer.uint32(37).float(message.M);
    }
    return writer;
  },

  decode(input: BinaryReader | Uint8Array, length?: number): AdrcParams {
    const reader = input instanceof BinaryReader ? input : new BinaryReader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseAdrcParams();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1: {
          if (tag !== 13) {
            break;
          }

          message.response = reader.float();
          continue;
        }
        case 2: {
          if (tag !== 21) {
            break;
          }

          message.b0 = reader.float();
          continue;
        }
        case 3: {
          if (tag !== 29) {
            break;
          }

          message.N = reader.float();
          continue;
        }
        case 4: {
          if (tag !== 37) {
            break;
          }

          message.M = reader.float();
          continue;
        }
      }
      if ((tag & 7) === 4 || tag === 0) {
        break;
      }
      reader.skip(tag & 7);
    }
    return message;
  },

  fromJSON(object: any): AdrcParams {
    return {
      response: isSet(object.response) ? globalThis.Number(object.response) : 0,
      b0: isSet(object.b0) ? globalThis.Number(object.b0) : 0,
      N: isSet(object.N) ? globalThis.Number(object.N) : 0,
      M: isSet(object.M) ? globalThis.Number(object.M) : 0,
    };
  },

  toJSON(message: AdrcParams): unknown {
    const obj: any = {};
    if (message.response !== 0) {
      obj.response = message.response;
    }
    if (message.b0 !== 0) {
      obj.b0 = message.b0;
    }
    if (message.N !== 0) {
      obj.N = message.N;
    }
    if (message.M !== 0) {
      obj.M = message.M;
    }
    return obj;
  },

  create<I extends Exact<DeepPartial<AdrcParams>, I>>(base?: I): AdrcParams {
    return AdrcParams.fromPartial(base ?? ({} as any));
  },
  fromPartial<I extends Exact<DeepPartial<AdrcParams>, I>>(object: I): AdrcParams {
    const message = createBaseAdrcParams();
    message.response = object.response ?? 0;
    message.b0 = object.b0 ?? 0;
    message.N = object.N ?? 0;
    message.M = object.M ?? 0;
    return message;
  },
};

function createBaseSensorParams(): SensorParams {
  return { p0_temperature: 0, p0_value: 0, p1_temperature: 0, p1_value: 0 };
}

export const SensorParams: MessageFns<SensorParams> = {
  encode(message: SensorParams, writer: BinaryWriter = new BinaryWriter()): BinaryWriter {
    if (message.p0_temperature !== 0) {
      writer.uint32(13).float(message.p0_temperature);
    }
    if (message.p0_value !== 0) {
      writer.uint32(21).float(message.p0_value);
    }
    if (message.p1_temperature !== 0) {
      writer.uint32(29).float(message.p1_temperature);
    }
    if (message.p1_value !== 0) {
      writer.uint32(37).float(message.p1_value);
    }
    return writer;
  },

  decode(input: BinaryReader | Uint8Array, length?: number): SensorParams {
    const reader = input instanceof BinaryReader ? input : new BinaryReader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseSensorParams();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1: {
          if (tag !== 13) {
            break;
          }

          message.p0_temperature = reader.float();
          continue;
        }
        case 2: {
          if (tag !== 21) {
            break;
          }

          message.p0_value = reader.float();
          continue;
        }
        case 3: {
          if (tag !== 29) {
            break;
          }

          message.p1_temperature = reader.float();
          continue;
        }
        case 4: {
          if (tag !== 37) {
            break;
          }

          message.p1_value = reader.float();
          continue;
        }
      }
      if ((tag & 7) === 4 || tag === 0) {
        break;
      }
      reader.skip(tag & 7);
    }
    return message;
  },

  fromJSON(object: any): SensorParams {
    return {
      p0_temperature: isSet(object.p0_temperature) ? globalThis.Number(object.p0_temperature) : 0,
      p0_value: isSet(object.p0_value) ? globalThis.Number(object.p0_value) : 0,
      p1_temperature: isSet(object.p1_temperature) ? globalThis.Number(object.p1_temperature) : 0,
      p1_value: isSet(object.p1_value) ? globalThis.Number(object.p1_value) : 0,
    };
  },

  toJSON(message: SensorParams): unknown {
    const obj: any = {};
    if (message.p0_temperature !== 0) {
      obj.p0_temperature = message.p0_temperature;
    }
    if (message.p0_value !== 0) {
      obj.p0_value = message.p0_value;
    }
    if (message.p1_temperature !== 0) {
      obj.p1_temperature = message.p1_temperature;
    }
    if (message.p1_value !== 0) {
      obj.p1_value = message.p1_value;
    }
    return obj;
  },

  create<I extends Exact<DeepPartial<SensorParams>, I>>(base?: I): SensorParams {
    return SensorParams.fromPartial(base ?? ({} as any));
  },
  fromPartial<I extends Exact<DeepPartial<SensorParams>, I>>(object: I): SensorParams {
    const message = createBaseSensorParams();
    message.p0_temperature = object.p0_temperature ?? 0;
    message.p0_value = object.p0_value ?? 0;
    message.p1_temperature = object.p1_temperature ?? 0;
    message.p1_value = object.p1_value ?? 0;
    return message;
  },
};

function createBaseHeaterParams(): HeaterParams {
  return { adrc: undefined, sensor: undefined };
}

export const HeaterParams: MessageFns<HeaterParams> = {
  encode(message: HeaterParams, writer: BinaryWriter = new BinaryWriter()): BinaryWriter {
    if (message.adrc !== undefined) {
      AdrcParams.encode(message.adrc, writer.uint32(10).fork()).join();
    }
    if (message.sensor !== undefined) {
      SensorParams.encode(message.sensor, writer.uint32(18).fork()).join();
    }
    return writer;
  },

  decode(input: BinaryReader | Uint8Array, length?: number): HeaterParams {
    const reader = input instanceof BinaryReader ? input : new BinaryReader(input);
    let end = length === undefined ? reader.len : reader.pos + length;
    const message = createBaseHeaterParams();
    while (reader.pos < end) {
      const tag = reader.uint32();
      switch (tag >>> 3) {
        case 1: {
          if (tag !== 10) {
            break;
          }

          message.adrc = AdrcParams.decode(reader, reader.uint32());
          continue;
        }
        case 2: {
          if (tag !== 18) {
            break;
          }

          message.sensor = SensorParams.decode(reader, reader.uint32());
          continue;
        }
      }
      if ((tag & 7) === 4 || tag === 0) {
        break;
      }
      reader.skip(tag & 7);
    }
    return message;
  },

  fromJSON(object: any): HeaterParams {
    return {
      adrc: isSet(object.adrc) ? AdrcParams.fromJSON(object.adrc) : undefined,
      sensor: isSet(object.sensor) ? SensorParams.fromJSON(object.sensor) : undefined,
    };
  },

  toJSON(message: HeaterParams): unknown {
    const obj: any = {};
    if (message.adrc !== undefined) {
      obj.adrc = AdrcParams.toJSON(message.adrc);
    }
    if (message.sensor !== undefined) {
      obj.sensor = SensorParams.toJSON(message.sensor);
    }
    return obj;
  },

  create<I extends Exact<DeepPartial<HeaterParams>, I>>(base?: I): HeaterParams {
    return HeaterParams.fromPartial(base ?? ({} as any));
  },
  fromPartial<I extends Exact<DeepPartial<HeaterParams>, I>>(object: I): HeaterParams {
    const message = createBaseHeaterParams();
    message.adrc = (object.adrc !== undefined && object.adrc !== null)
      ? AdrcParams.fromPartial(object.adrc)
      : undefined;
    message.sensor = (object.sensor !== undefined && object.sensor !== null)
      ? SensorParams.fromPartial(object.sensor)
      : undefined;
    return message;
  },
};

type Builtin = Date | Function | Uint8Array | string | number | boolean | undefined;

export type DeepPartial<T> = T extends Builtin ? T
  : T extends globalThis.Array<infer U> ? globalThis.Array<DeepPartial<U>>
  : T extends ReadonlyArray<infer U> ? ReadonlyArray<DeepPartial<U>>
  : T extends {} ? { [K in keyof T]?: DeepPartial<T[K]> }
  : Partial<T>;

type KeysOfUnion<T> = T extends T ? keyof T : never;
export type Exact<P, I extends P> = P extends Builtin ? P
  : P & { [K in keyof P]: Exact<P[K], I[K]> } & { [K in Exclude<keyof I, KeysOfUnion<P>>]: never };

function isSet(value: any): boolean {
  return value !== null && value !== undefined;
}

export interface MessageFns<T> {
  encode(message: T, writer?: BinaryWriter): BinaryWriter;
  decode(input: BinaryReader | Uint8Array, length?: number): T;
  fromJSON(object: any): T;
  toJSON(message: T): unknown;
  create<I extends Exact<DeepPartial<T>, I>>(base?: I): T;
  fromPartial<I extends Exact<DeepPartial<T>, I>>(object: I): T;
}