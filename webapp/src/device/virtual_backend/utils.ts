import { Point } from '@/proto/generated/types'
import { ADRC } from './adrc'
import { AdrcParams } from '@/proto/generated/types'

function near(p1: Point, p2: Point, precision: number): boolean {
  return Math.abs(p1.x - p2.x) < precision && Math.abs(p1.y - p2.y) < precision;
}

export function sparsedPush(arr: Point[], newPoint: Point, precision: number = 1.0) {
  if (arr.length < 3) {
    arr.push(newPoint);
    return;
  }

  const last = arr.length - 1;

  if (near(arr[last-1], newPoint, precision)) {
    arr.pop();
    arr.push(newPoint);
    return;
  }

  arr.push(newPoint);
}

export function createADRC(config: AdrcParams): ADRC {
  const τ = config.response
  const b0 = config.b0

  const ω_o = config.N / τ
  const ω_c = ω_o / config.M
  const Kp = ω_c / b0

  return  new ADRC(b0, ω_o, Kp)
}
