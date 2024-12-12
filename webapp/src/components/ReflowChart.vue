<script setup lang="ts">
import * as d3 from 'd3'
import { onMounted, onUnmounted, ref, watch, inject, toRaw } from 'vue';
import { Device } from '@/device'
import { Profile, Constants, Point, DeviceState } from '@/proto/generated/types'

const device: Device = inject('device')!

const svgRef = ref<SVGSVGElement | null>(null)
const margin = { top: 30, right: 40, bottom: 30, left: 40 }

const props = withDefaults(defineProps<{
  profile: Profile | null
  history?: Point[]
  show_history?: boolean
  id: string
}>(), { show_history: true })

const s = (selector: string) => `#${props.id} .root ${selector}`.trim()

const x = d3.scaleLinear()
const y = d3.scaleLinear()

function buildChart() {
  // d3 is not ok with ts types. Disable some rules.
  /* eslint-disable @typescript-eslint/no-explicit-any */
  if (!svgRef.value || !svgRef.value.parentElement) return

  const width = svgRef.value.parentElement.clientWidth - margin.left - margin.right
  const height = svgRef.value.parentElement.clientHeight - margin.top - margin.bottom

  //
  // Collect profile points/segments data
  //

  const profilePoints: Point[] = [{ x: 0, y: Constants.START_TEMPERATURE }]
  props.profile?.segments.forEach((segment, i) => {
    profilePoints.push({ x: profilePoints[i].x + segment.duration, y: segment.target })
  })
  // No real segments - remove single point to avoid artifacts
  if (profilePoints.length === 1) profilePoints.pop()

  //
  // Collect probe history data
  //

  const historyPoints: Point[] = props.show_history ? toRaw(props.history || []) : []

  const profileMaxX = d3.max(profilePoints.map(p => p.x))||0
  const profileMaxY = d3.max(profilePoints.map(p => p.y))||0
  const probeMaxX = d3.max(historyPoints.map(p => p.x))||0
  const probeMaxY = d3.max(historyPoints.map(p => p.y))||0

  const xRangeMax = Math.max(profileMaxX, probeMaxX) || 200
  const yRangeMax = (Math.max(profileMaxY, probeMaxY) || 300) + 10

  // Setup root
  d3.select(s(''))
    .attr('transform', `translate(${margin.left}, ${margin.top})`)

  // Y axis (temperature)
  y.domain([0, yRangeMax]).range([height, 0]);
  d3.select(s('.y-axis'))
    .transition()
    .call(d3.axisLeft(y) as any)

  // X axis (time)
  x.domain([0, xRangeMax]).range([0, width]);
  d3.select(s('.x-axis'))
    .transition()
    .attr("transform", `translate(0, ${height})`)
    .call(d3.axisBottom(x) as any);

  const line = d3.line((p: Point) => x(p.x), (p: Point) => y(p.y))

  // Profile zigzag line
  d3.select('.pchart-profile-line').datum(profilePoints)
    .transition()
    .attr('d', line)

  // Probe line
  d3.select('.pchart-probe-line').datum(historyPoints)
    //.transition()
    .attr('d', line)

  //
  // Draw last point guides in `running` mode
  //

  let verticalGuide: Point[] = []
  let horizontalGuide: Point[] = []

  if (device.status.value.state === DeviceState.Reflow) {
    if (historyPoints.length > 1) {
      const lastProbe = historyPoints[historyPoints.length - 1]
      verticalGuide = [{ x: lastProbe.x, y: 0 }, { x: lastProbe.x, y: yRangeMax }]
      horizontalGuide = [{ x: 0, y: lastProbe.y }, { x: xRangeMax, y: lastProbe.y }]
    }
  }

  d3.select('.guide-vertical').datum(verticalGuide)
    //.transition()
    .attr('d', line)
  d3.select('.guide-horizontal').datum(horizontalGuide)
    //.transition()
    .attr('d', line)
}

onMounted(() => {
  if (!svgRef.value || !svgRef.value.parentElement) return

  watch(() => props.profile, buildChart, { immediate: true, deep: true })
  watch([props.history || ref([]), () => device.status.value.state, () => props.show_history], buildChart)
  window.addEventListener('resize', buildChart)
})

onUnmounted(() => {
  window.removeEventListener('resize', buildChart)
})
</script>

<template>
  <svg :id="props.id" width="100%" height="100%" ref="svgRef">
    <g class="root">
      <g class="x-axis"></g>
      <g class="y-axis"></g>
      <path class="pchart-profile-line" />
      <path class="pchart-probe-line" />
      <path class="guide-vertical" />
      <path class="guide-horizontal" />
    </g>
  </svg>
</template>

<style scoped>
  .pchart-profile-line {
    fill: none;
    stroke: steelblue;
    stroke-width: 4px;
    stroke-linejoin: round;
    stroke-linecap: round;
  }

  .pchart-probe-line {
    fill: none;
    stroke: red;
    stroke-width: 2px;
    stroke-linejoin: round;
    stroke-linecap: round;
  }

  .guide-vertical,
  .guide-horizontal {
    fill: none;
    stroke: red;
    stroke-width: 1px;
    stroke-dasharray: 3, 3;
    opacity: 0.3;
  }
</style>
