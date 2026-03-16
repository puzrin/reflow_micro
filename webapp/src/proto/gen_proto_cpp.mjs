import { execFileSync } from 'node:child_process'
import { dirname, resolve } from 'node:path'
import { fileURLToPath } from 'node:url'

const scriptDir = dirname(fileURLToPath(import.meta.url))
const webappRoot = resolve(scriptDir, '..', '..')
const firmwareGeneratedDir = resolve(webappRoot, '..', 'firmware', 'src', 'proto', 'generated')
const protocIncludePath = resolve(webappRoot, 'node_modules', 'protoc', 'include')

function resolveNanopbProtoPath() {
  try {
    return execFileSync(
      'python3',
      ['-c', "import inspect, nanopb, os; print(os.path.join(os.path.dirname(inspect.getfile(nanopb)), 'generator/proto'))"],
      { encoding: 'utf8' },
    ).trim()
  } catch (error) {
    const output = typeof error?.stdout === 'string' ? error.stdout.trim() : ''
    if (output) {
      return output
    }
    throw error
  }
}

execFileSync(
  'nanopb_generator',
  [
    `--proto-path=${scriptDir}`,
    `--proto-path=${resolveNanopbProtoPath()}`,
    `--proto-path=${protocIncludePath}`,
    'types.proto',
    '--output-dir',
    firmwareGeneratedDir,
  ],
  {
    cwd: scriptDir,
    stdio: 'inherit',
  },
)
