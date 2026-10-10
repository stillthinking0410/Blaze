import { useCallback, useEffect, useState } from 'react'
import './App.css'

interface AppConfig {
  sourceFolder: string
  destinationFolder: string
}

interface SortingProgress {
  totalFiles: number
  processedFiles: number
  skippedFiles: number
  failedFiles: number
  currentFile: string
  percentage: number
}

type SortingStatus =
  | 'idle'
  | 'running'
  | 'cancelling'
  | 'completed'
  | 'completed_with_errors'
  | 'failed'
  | 'cancelled'

const initialProgress: SortingProgress = {
  totalFiles: 0,
  processedFiles: 0,
  skippedFiles: 0,
  failedFiles: 0,
  currentFile: '',
  percentage: 0,
}

function App() {
  const [sourceFolder, setSourceFolder] = useState('')
  const [destinationFolder, setDestinationFolder] = useState('')
  const [loading, setLoading] = useState(true)
  const [saving, setSaving] = useState(false)
  const [starting, setStarting] = useState(false)
  const [cancelling, setCancelling] = useState(false)
  const [status, setStatus] = useState<SortingStatus>('idle')
  const [progress, setProgress] = useState(initialProgress)
  const [error, setError] = useState('')
  const [message, setMessage] = useState('')

  const isActive = status === 'running' || status === 'cancelling'
  const canStart =
    !loading &&
    !saving &&
    !starting &&
    !isActive &&
    sourceFolder.trim().length > 0 &&
    destinationFolder.trim().length > 0

  useEffect(() => {
    let active = true

    async function loadInitialState() {
      try {
        const [configResponse, statusResponse] = await Promise.all([
          fetch('/api/config'),
          fetch('/api/sorting/status'),
        ])

        if (!configResponse.ok || !statusResponse.ok) {
          throw new Error('Unable to load Blaze configuration or status.')
        }

        const config: AppConfig = await configResponse.json()
        const currentStatus: { status: SortingStatus } =
          await statusResponse.json()

        const progressResponse = await fetch('/api/sorting/progress')
        const currentProgress: SortingProgress = progressResponse.ok
          ? await progressResponse.json()
          : initialProgress

        if (active) {
          setSourceFolder(config.sourceFolder)
          setDestinationFolder(config.destinationFolder)
          setStatus(currentStatus.status)
          setProgress(currentProgress)
        }
      } catch {
        if (active) {
          setError('Could not connect to Blaze. Check that the backend is running.')
        }
      } finally {
        if (active) {
          setLoading(false)
        }
      }
    }

    void loadInitialState()

    return () => {
      active = false
    }
  }, [])

  const saveConfig = async () => {
    setError('')
    setMessage('')
    setSaving(true)

    try {
      const response = await fetch('/api/config', {
        method: 'PUT',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({
          sourceFolder: sourceFolder.trim(),
          destinationFolder: destinationFolder.trim(),
        }),
      })

      if (!response.ok) {
        const result = await response.json().catch(() => null)
        throw new Error(result?.error ?? 'Unable to save the configuration.')
      }

      setMessage('Folder configuration saved successfully.')
    } catch (err) {
      setError(
        err instanceof Error
          ? err.message
          : 'An unexpected error occurred while saving.',
      )
    } finally {
      setSaving(false)
    }
  }

  const startSorting = async () => {
    setError('')
    setMessage('')
    setStarting(true)

    try {
      // Save the latest folder paths before starting the operation.
      const configResponse = await fetch('/api/config', {
        method: 'PUT',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({
          sourceFolder: sourceFolder.trim(),
          destinationFolder: destinationFolder.trim(),
        }),
      })

      if (!configResponse.ok) {
        const result = await configResponse.json().catch(() => null)
        throw new Error(result?.error ?? 'Unable to save the folder configuration.')
      }

      setProgress(initialProgress)

      const response = await fetch('/api/sorting/start', {
        method: 'POST',
      })

      const result = await response.json().catch(() => null)

      if (!response.ok) {
        throw new Error(result?.error ?? 'Unable to start sorting.')
      }

      setStatus('running')
      setMessage('Sorting started.')
    } catch (err) {
      setError(
        err instanceof Error ? err.message : 'Unable to start sorting.',
      )
    } finally {
      setStarting(false)
    }
  }

  const cancelSorting = async () => {
    setError('')
    setMessage('')
    setCancelling(true)

    try {
      const response = await fetch('/api/sorting/cancel', {
        method: 'POST',
      })

      const result = await response.json().catch(() => null)

      if (!response.ok) {
        throw new Error(result?.error ?? 'Unable to cancel sorting.')
      }

      setStatus('cancelling')
      setMessage('Cancellation requested.')
    } catch (err) {
      setError(
        err instanceof Error ? err.message : 'Unable to cancel sorting.',
      )
    } finally {
      setCancelling(false)
    }
  }

  const refreshStatus = useCallback(async () => {
    try {
      const response = await fetch('/api/sorting/status')
      if (!response.ok) return

      const result: { status: SortingStatus } = await response.json()
      setStatus(result.status)
    } catch {
      // The SSE connection or initial API load will report connection issues.
    }
  }, [])

  useEffect(() => {
    if (!isActive) return

    const events = new EventSource('/api/sorting/progress/stream')

    events.onmessage = (event) => {
      try {
        const nextProgress: SortingProgress = JSON.parse(event.data)
        setProgress(nextProgress)
      } catch {
        setError('Received invalid progress data from Blaze.')
      }
    }

    events.onerror = () => {
      // SSE may close when the sorting operation reaches a terminal state.
      // Query the status endpoint instead of treating every close as a failure.
      void refreshStatus()
      events.close()
    }

    return () => events.close()
  }, [isActive, refreshStatus])

  useEffect(() => {
    if (!isActive) return

    let timer: number | undefined

    const checkStatus = async () => {
      try {
        const response = await fetch('/api/sorting/status')
        if (!response.ok) return

        const result: { status: SortingStatus } = await response.json()
        setStatus(result.status)
      } catch {
        // Preserve the last known state during temporary connection failures.
      }
    }

    timer = window.setInterval(() => void checkStatus(), 1500)

    return () => {
      if (timer !== undefined) window.clearInterval(timer)
    }
  }, [isActive])

  const statusLabel: Record<SortingStatus, string> = {
    idle: 'Ready to organize',
    running: 'Sorting in progress',
    cancelling: 'Cancellation requested',
    completed: 'Sorting completed',
    completed_with_errors: 'Completed with errors',
    failed: 'Sorting failed',
    cancelled: 'Sorting cancelled',
  }

  return (
    <main className="app-shell">
      <aside className="sidebar">
        <div className="brand">
          <div className="brand-icon">B</div>
          <span>blaze</span>
        </div>

        <div className="sidebar-section-label">WORKSPACE</div>
        <button className="nav-item active" type="button">
          <span className="nav-icon">▦</span>
          <span>Photo organizer</span>
        </button>

        <div className="sidebar-footer">
          <div className="status-dot" />
          <div>
            <div className="footer-title">Blaze is ready</div>
            <div className="footer-subtitle">Local processing</div>
          </div>
        </div>
      </aside>

      <section className="main-panel">
        <header className="topbar">
          <div>
            <div className="breadcrumb">Workspace / Organizer</div>
            <h1>Photo organizer</h1>
          </div>
          <div className="local-badge">
            <span className="status-dot" />
            Files stay on your device
          </div>
        </header>

        <div className="content">
          <section className="welcome">
            <div className="eyebrow">YOUR MEDIA, ORGANIZED</div>
            <h2>A little more order.<br />A lot less effort.</h2>
            <p>
              Organize your photos and videos by capture date.
              Your original files are never moved or modified.
            </p>
          </section>

          <section className="folder-card">
            <div className="card-heading">
              <div>
                <h3>Folder configuration</h3>
                <p>Choose where Blaze reads and organizes your media.</p>
              </div>
              <span className="step-label">STEP 01</span>
            </div>

            {loading ? (
              <p role="status">Loading saved configuration…</p>
            ) : (
              <>
                <div className="folder-field">
                  <label htmlFor="source-folder">SOURCE FOLDER</label>
                  <div className="path-input">
                    <span className="folder-icon">↳</span>
                    <input
                      id="source-folder"
                      type="text"
                      value={sourceFolder}
                      disabled={isActive || saving || starting}
                      onChange={(event) => {
                        setSourceFolder(event.target.value)
                        setError('')
                        setMessage('')
                      }}
                      placeholder="/Users/you/Pictures/Unsorted"
                    />
                    <button className="browse-button" type="button" disabled>
                      Browse
                    </button>
                  </div>
                  <span className="field-hint">
                    Supported formats: JPG, JPEG, HEIC, MOV and MP4.
                  </span>
                </div>

                <div className="folder-field">
                  <label htmlFor="destination-folder">DESTINATION FOLDER</label>
                  <div className="path-input">
                    <span className="folder-icon">↳</span>
                    <input
                      id="destination-folder"
                      type="text"
                      value={destinationFolder}
                      disabled={isActive || saving || starting}
                      onChange={(event) => {
                        setDestinationFolder(event.target.value)
                        setError('')
                        setMessage('')
                      }}
                      placeholder="/Users/you/Pictures/Organized"
                    />
                    <button className="browse-button" type="button" disabled>
                      Browse
                    </button>
                  </div>
                  <span className="field-hint">
                    Blaze creates the destination folder if it doesn't exist.
                  </span>
                </div>

                <div className="privacy-note">
                  <span className="privacy-icon">✓</span>
                  <span>
                    <strong>Originals are protected.</strong> Blaze copies files
                    into the destination and keeps your source files untouched.
                  </span>
                </div>

                {error && <p className="feedback error" role="alert">{error}</p>}
                {message && <p className="feedback success" role="status">{message}</p>}

                <div className="card-actions">
                  <span className="required-note">Both folders are required.</span>
                  <button
                    className="secondary-button"
                    type="button"
                    disabled={isActive || saving || starting || loading}
                    onClick={() => void saveConfig()}
                  >
                    {saving ? 'Saving…' : 'Save configuration'}
                  </button>
                </div>
              </>
            )}
          </section>

          <section className="folder-card sorting-card">
            <div className="card-heading">
              <div>
                <h3>Sorting activity</h3>
                <p>Track your media organization as it happens.</p>
              </div>
              <span className="step-label">STEP 02</span>
            </div>

            <div className="sorting-status">
              <span className={`status-indicator ${isActive ? 'active' : ''}`} />
              <span>{statusLabel[status]}</span>
            </div>

            <div className="progress-track" aria-label="Sorting progress">
              <div
                className="progress-fill"
                style={{ width: `${Math.max(0, Math.min(100, progress.percentage))}%` }}
              />
            </div>

            <div className="progress-caption">
              <span>{progress.percentage}% complete</span>
              <span>{progress.processedFiles + progress.skippedFiles + progress.failedFiles} / {progress.totalFiles} files</span>
            </div>

            <div className="stats-grid">
              <div className="stat-item">
                <span className="stat-number">{progress.processedFiles}</span>
                <span className="stat-label">Copied</span>
              </div>
              <div className="stat-item">
                <span className="stat-number">{progress.skippedFiles}</span>
                <span className="stat-label">Skipped</span>
              </div>
              <div className="stat-item">
                <span className="stat-number">{progress.failedFiles}</span>
                <span className="stat-label">Failed</span>
              </div>
            </div>

            <div className="current-file">
              <span className="field-hint">CURRENT FILE</span>
              <span className="current-file-name">
                {isActive && progress.currentFile
                  ? progress.currentFile
                  : 'No file currently being processed'}
              </span>
            </div>

            <div className="card-actions">
              <span className="required-note">
                {isActive ? 'You can cancel an active run.' : 'Ready when you are.'}
              </span>
              {isActive ? (
                <button
                  className="danger-button"
                  type="button"
                  disabled={status === 'cancelling' || cancelling}
                  onClick={() => void cancelSorting()}
                >
                  {cancelling || status === 'cancelling' ? 'Cancelling…' : 'Cancel sorting'}
                </button>
              ) : (
                <button
                  className="primary-button"
                  type="button"
                  disabled={!canStart}
                  onClick={() => void startSorting()}
                >
                  {starting ? 'Starting…' : 'Start organizing'}
                  <span>→</span>
                </button>
              )}
            </div>
          </section>

          <footer className="content-footer">
            <span>BLAZE · DESKTOP MEDIA ORGANIZER</span>
            <span>Local-first by design</span>
          </footer>
        </div>
      </section>
    </main>
  )
}

export default App