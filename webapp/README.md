# Reflow Micro web UI

Vue 3 + TypeScript single-page app that talks to the Reflow Micro table over Web Bluetooth.

## Requirements

- Node.js 20+
- npm 10+

## Install

```sh
npm install
```

## Common scripts

- `npm run dev` — start the Vite dev server on http://localhost:5173
- `npm run build` — run `vue-tsc` and produce the production bundle in `dist/`
- `npm run preview` — preview the latest production build
- `npm run test` — lint, type-check, and run the Vitest suite
- `npm run lint:fix` — auto-fix lint issues where possible
- `npm run format` — format the sources with Prettier

## Protobuf sources

Message formats live in `src/proto/types.proto`. To regenerate all artifacts (TS types, firmware stubs, and default payloads), run:

```sh
npm run gen:proto
```

This command shells out to `protoc`, `nanopb_generator`, and `ts-proto`. They are installed via dev dependencies; ensure you have Python available for `nanopb_generator`.
