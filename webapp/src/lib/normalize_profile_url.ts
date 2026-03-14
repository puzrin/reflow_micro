import ky from 'ky'

interface GitHubGistFile {
  filename?: string | null
  raw_url?: string | null
}

interface GitHubGist {
  files: Record<string, GitHubGistFile>
}

interface ResolvedGitHubGistFile {
  filename: string
  raw_url: string
}

type ProfileUrlNormalizer = (url: URL) => Promise<string | null>

const normalizers: ProfileUrlNormalizer[] = [
  normalizeGistUrl,
]

export async function normalizeProfileUrl(value: string): Promise<string> {
  let url: URL

  try {
    url = new URL(value)
  } catch {
    throw new Error('Invalid URL')
  }

  for (const normalize of normalizers) {
    const normalizedUrl = await normalize(url)
    if (normalizedUrl) return normalizedUrl
  }

  return url.toString()
}

async function normalizeGistUrl(url: URL): Promise<string | null> {
  if (url.hostname !== 'gist.github.com') return null

  // Supported gist page URLs:
  // - /<gist_id>
  // - /<user>/<gist_id>
  // Optional trailing slash is allowed.
  const gistId = url.pathname.match(/^\/(?:[^/]+\/)?([0-9a-f]+)\/?$/i)?.[1]
  if (!gistId) throw new Error('Invalid gist URL')

  let gist: GitHubGist

  try {
    gist = await ky(`https://api.github.com/gists/${gistId}`, { retry: 0 }).json<GitHubGist>()
  } catch {
    throw new Error('Failed to resolve gist URL')
  }

  const files = Object.values(gist.files)
    .filter((file): file is ResolvedGitHubGistFile =>
      typeof file.filename === 'string' && file.filename.length > 0
      && typeof file.raw_url === 'string' && file.raw_url.length > 0)

  if (!files.length) throw new Error('Gist has no files')

  if (files.length > 1) throw new Error('Gist URLs with multiple files are not supported; use a raw URL')

  return files[0].raw_url
}
