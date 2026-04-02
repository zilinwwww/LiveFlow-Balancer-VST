param(
    [string]$HostExe = "C:\Program Files\Fender\Studio Pro 8\Studio Pro.exe",
    [string]$InstalledPluginDir = "C:\Program Files\Common Files\VST3\LiveFlow Balancer.vst3",
    [string]$BuiltPluginDir = "",
    [int]$WaitSeconds = 5,
    [switch]$PurgeHostCache,
    [switch]$SkipLaunch
)

$ErrorActionPreference = "Stop"

$scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$repoRoot = Split-Path -Parent $scriptDir

if ([string]::IsNullOrWhiteSpace($BuiltPluginDir)) {
    $BuiltPluginDir = Join-Path $repoRoot "build-win\LiveFlowBalancer_artefacts\Release\VST3\LiveFlow Balancer.vst3"
}

$processNames = @(
    "Studio Pro",
    "Studio One",
    "VSTPlugInScanner"
)

function Write-Step([string]$Message) {
    Write-Host "[LiveFlow] $Message"
}

function Remove-PluginCacheEntries {
    param(
        [string]$PluginLeafName
    )

    $settingsRoot = Join-Path $env:APPDATA "Fender\Studio Pro 8\x64"
    if (-not (Test-Path -LiteralPath $settingsRoot)) {
        return
    }

    $settingsFiles = @()
    $settingsFiles += Get-ChildItem -LiteralPath $settingsRoot -Filter "Plugins-*.settings" -ErrorAction SilentlyContinue
    $settingsFiles += Get-ChildItem -LiteralPath $settingsRoot -Filter "PluginBlacklist.settings" -ErrorAction SilentlyContinue

    foreach ($settingsFile in $settingsFiles) {
        $content = Get-Content -LiteralPath $settingsFile.FullName -Raw -ErrorAction SilentlyContinue
        if ([string]::IsNullOrWhiteSpace($content)) {
            continue
        }

        $escapedLeaf = [regex]::Escape($PluginLeafName)
        $updated = $content

        $updated = [regex]::Replace(
            $updated,
            "(?ms)^\s*<Section path=""[^""]*/$escapedLeaf"">.*?</Section>\s*",
            "")

        $updated = [regex]::Replace(
            $updated,
            "(?m)^\s*<Url[^>]*url=""[^""]*/$escapedLeaf(?:/[^""]*)?""[^>]*/>\s*\r?\n?",
            "")

        if ($updated -ne $content) {
            $backupPath = "$($settingsFile.FullName).bak"
            if (-not (Test-Path -LiteralPath $backupPath)) {
                Copy-Item -LiteralPath $settingsFile.FullName -Destination $backupPath -Force
            }

            $utf8Bom = New-Object System.Text.UTF8Encoding($true)
            [System.IO.File]::WriteAllText($settingsFile.FullName, $updated, $utf8Bom)
        }
    }
}

if (-not (Test-Path -LiteralPath $BuiltPluginDir)) {
    throw "Built plugin directory not found: $BuiltPluginDir"
}

if (-not (Test-Path -LiteralPath $HostExe)) {
    throw "Host executable not found: $HostExe"
}

$pluginLeafName = Split-Path -Leaf $InstalledPluginDir

Write-Step "Stopping host/scanner processes if they are running..."
foreach ($processName in $processNames) {
    Get-Process -Name $processName -ErrorAction SilentlyContinue | Stop-Process -Force -ErrorAction SilentlyContinue
}

if ($PurgeHostCache) {
    Write-Step "Purging Fender Studio Pro cache entries for $pluginLeafName ..."
    Remove-PluginCacheEntries -PluginLeafName $pluginLeafName
}

Write-Step "Replacing installed VST3 bundle..."
$installParent = Split-Path -Parent $InstalledPluginDir
if (-not (Test-Path -LiteralPath $installParent)) {
    New-Item -ItemType Directory -Path $installParent | Out-Null
}

$robocopyArgs = @(
    $BuiltPluginDir
    $InstalledPluginDir
    "/E"
    "/COPY:DAT"
    "/R:0"
    "/W:0"
)

$null = & "$env:SystemRoot\System32\robocopy.exe" @robocopyArgs
$robocopyExitCode = $LASTEXITCODE

if ($robocopyExitCode -ge 8) {
    throw "robocopy failed with exit code $robocopyExitCode"
}

Write-Step "Installed plugin:"
if (Test-Path -LiteralPath $InstalledPluginDir) {
    Get-Item -LiteralPath $InstalledPluginDir | Select-Object FullName, LastWriteTime | Format-List
} else {
    Write-Warning "Installed plugin directory was not immediately visible: $InstalledPluginDir"
}

if ($SkipLaunch) {
    Write-Step "Skipping host launch."
    exit 0
}

Write-Step "Waiting $WaitSeconds second(s) before relaunch..."
Start-Sleep -Seconds $WaitSeconds

Write-Step "Launching Fender Studio Pro 8..."
Start-Process -FilePath $HostExe
