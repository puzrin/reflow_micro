import { Point } from '@/proto/generated/types'


export class SparseHistory {
  // TODO: Consider switch to M4 Aggregates for data packing

  // Thresholds for data packing
  static Y_THRESHOLD = 1.0
  static X_THRESHOLD = 2.0
  static X_SCALE_AFTER = 400

  data: Point[] = [];

  static from(data: Point[]): SparseHistory {
    const history = new SparseHistory()
    history.data = data
    return history
  }

  reset() { this.data.length = 0 }

  add(...points: Point[]) {
    if (points.length == 0) return

    points.forEach(p => {
      if (this.data.length) {
        // Skip identical points
        const last = this.data.at(-1)!
        if (last.x === p.x && last.y === p.y) return
      }

      if (this.is_last_point_landed()) this.data.push(p)
      else this.data[this.data.length-1] = p
    })
  }

  private is_last_point_landed(): boolean {
    if (this.data.length < 2) return true

    const last = this.data.at(-1)!
    const prev = this.data.at(-2)!

    if (Math.abs(last.y - prev.y) >= SparseHistory.Y_THRESHOLD) return true

    const x_threshold = Math.max(SparseHistory.X_THRESHOLD, last.x / SparseHistory.X_SCALE_AFTER)
    if (Math.abs(last.x - prev.x) >= x_threshold) return true

    return false
  }

  get_data_from(x: number): Point[] {
    for (let i = 0; i < this.data.length; i++) {
      if (this.data[i].x >= x) return this.data.slice(i)
    }
    return []
  }
}
