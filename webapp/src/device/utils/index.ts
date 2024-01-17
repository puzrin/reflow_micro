import { type Point } from "@/device/types"

export function clamp(value: number, min: number, max: number) {
  return Math.max(min, Math.min(max, value));
}

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
