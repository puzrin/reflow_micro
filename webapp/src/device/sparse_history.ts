import { Point } from '@/proto/generated/types'


export class SparseHistory {
  // TODO: Consider switch to M4 Aggregates for data packing

  // Thresholds for data packing
  static DELTA_Y = 1.0
  static X_CHART_LENGTH = 400

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
      if (this.is_last_point_landed()) this.data.push(p)
      else this.data[this.data.length-1] = p
    })
  }

  private is_last_point_landed(): boolean {
    if (this.data.length < 2) return true

    if (Math.abs(this.data.at(-1)!.y - this.data.at(-2)!.y) >= SparseHistory.DELTA_Y) return true

    const delta_x = Math.max(this.data.at(-1)!.x / SparseHistory.X_CHART_LENGTH, 1)
    if (Math.abs(this.data.at(-1)!.x - this.data.at(-2)!.x) >= delta_x) return true

    return false
  }

  get_data_after(x: number): Point[] {
    for (let i = 0; i < this.data.length; i++) {
      if (this.data[i].x > x) return this.data.slice(i)
    }
    return []
  }

}
