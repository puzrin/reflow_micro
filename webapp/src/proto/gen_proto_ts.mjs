import { execFileSync } from 'node:child_process'
import { cpSync, mkdirSync, mkdtempSync, rmSync } from 'node:fs'
import { dirname, join, resolve } from 'node:path'
import { fileURLToPath } from 'node:url'

const scriptDir = dirname(fileURLToPath(import.meta.url))
const webappRoot = resolve(scriptDir, '..', '..')
const generatedDir = resolve(scriptDir, 'generated')
const tempRootDir = resolve(webappRoot, 'target', 'tmp')

mkdirSync(tempRootDir, { recursive: true })

// Generate into temp to filter out extra files produced from
// imported nanopb/descriptor protos.
const tempOutDir = mkdtempSync(join(tempRootDir, 'proto-ts-'))

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

const nanopbProtoPath = resolveNanopbProtoPath()

const protocIncludePath = resolve(webappRoot, 'node_modules/protoc/include')

mkdirSync(generatedDir, { recursive: true })

let completed = false

try {
  execFileSync(
    'protoc',
    [
      `--ts_proto_out=${tempOutDir}`,
      '--ts_proto_opt=esModuleInterop=true,snakeToCamel=false,outputJsonMethods=false',
      `--proto_path=${scriptDir}`,
      `--proto_path=${nanopbProtoPath}`,
      `--proto_path=${protocIncludePath}`,
      'types.proto',
    ],
    {
      cwd: scriptDir,
      stdio: 'inherit',
    },
  )

  cpSync(resolve(tempOutDir, 'types.ts'), resolve(generatedDir, 'types.ts'))
  completed = true
} finally {
  if (completed) {
    rmSync(tempOutDir, { recursive: true, force: true })
  } else {
    console.error(`TS proto temp output kept at: ${tempOutDir}`)
  }
}
