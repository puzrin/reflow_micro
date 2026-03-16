import { type Profile, type Segment } from '@/proto/generated/types'
import { SharedConstants as Constants } from '@/lib/shared_constants'
import { PROFILE_LIMITS } from '@/lib/web_limits'

export type ExchangeMark = 'heating_profile'

export const EXCHANGE_MARK: ExchangeMark = 'heating_profile'
export const EXCHANGE_VERSION = 1 as const

const MAX_EXCHANGE_BYTES = 20 * 1024
const textEncoder = new TextEncoder()

export interface ProfileSegmentPayloadV1 {
  target_c: number
  duration_sec: number
}

export interface ProfilePayloadV1 {
  name: string
  segments: ProfileSegmentPayloadV1[]
}

export interface ProfilePayloadDocumentV1 {
  mark: ExchangeMark
  version: 1
  profile: ProfilePayloadV1
}

export function decodePayload(payload: string): { profile: Profile } {
  if (textEncoder.encode(payload).length > MAX_EXCHANGE_BYTES) {
    decodeError('import data is too big')
  }

  let parsedJson: unknown

  try {
    parsedJson = JSON.parse(payload)
  } catch {
    decodeError('invalid JSON')
  }

  if (!isRecord(parsedJson)) decodeError('input must be an object')

  if (parsedJson.mark !== EXCHANGE_MARK) decodeError('no required watermark')

  if (parsedJson.version !== EXCHANGE_VERSION) decodeError('unsupported version')

  if (!isRecord(parsedJson.profile)) decodeError('profile must be an object')

  return {
    profile: {
      id: 0,
      name: parseName(parsedJson.profile.name, 'profile.name'),
      segments: decodePayloadSegments(parsedJson.profile.segments),
    },
  }
}

function decodePayloadSegments(value: unknown): Segment[] {
  if (!Array.isArray(value)) decodeError('segments must be an array')

  if (value.length === 0) decodeError('at least one segment required')

  if (value.length > Constants.MAX_REFLOW_SEGMENTS) {
    decodeError(`max ${Constants.MAX_REFLOW_SEGMENTS} segments allowed`)
  }

  return value.map((segment, index) => {
    if (!isRecord(segment)) decodeError(`segment ${index + 1} must be an object`)

    return {
      target: parseInteger(segment.target_c, `profile.segments[${index}].target_c`),
      duration: parseDuration(segment.duration_sec, `profile.segments[${index}].duration_sec`),
    }
  })
}

export function encodeProfile(profile: Profile): string {
  const payloadDocument: ProfilePayloadDocumentV1 = {
    mark: EXCHANGE_MARK,
    version: EXCHANGE_VERSION,
    profile: {
      name: profile.name,
      segments: profile.segments.map((segment) => ({
        target_c: segment.target,
        duration_sec: segment.duration,
      })),
    },
  }

  return JSON.stringify(payloadDocument, null, 2)
}

function parseName(value: unknown, path: string): string {
  if (typeof value !== 'string') decodeError(`${path} must be a string`)

  const normalized = value.trim()

  if (!normalized) decodeError(`${path} cannot be empty`)
  if (normalized.length < PROFILE_LIMITS.nameMinChars) {
    decodeError(`${path} must be at least ${PROFILE_LIMITS.nameMinChars} characters`)
  }
  if (textEncoder.encode(normalized).length > Constants.MAX_PROFILE_NAME_LENGTH) {
    decodeError(`${path} must be at most ${Constants.MAX_PROFILE_NAME_LENGTH} bytes`)
  }

  return normalized
}

function parseInteger(value: unknown, path: string): number {
  if (typeof value !== 'number' || !Number.isFinite(value) || !Number.isInteger(value)) decodeError(`${path} must be an integer`)

  return value
}

function parseDuration(value: unknown, path: string): number {
  const parsed = parseInteger(value, path)

  if (parsed < PROFILE_LIMITS.durationMin || parsed > PROFILE_LIMITS.durationMax) {
    decodeError(`${path} must be between ${PROFILE_LIMITS.durationMin} and ${PROFILE_LIMITS.durationMax}`)
  }

  return parsed
}

function decodeError(message: string): never {
  throw new Error(`Bad format (${message})`)
}

function isRecord(value: unknown): value is Record<string, unknown> {
  return typeof value === 'object' && value !== null && !Array.isArray(value)
}
