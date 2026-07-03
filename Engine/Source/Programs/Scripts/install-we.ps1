param(
    [Parameter(ValueFromRemainingArguments = $true)]
    [string[]]$Args
)

$ErrorActionPreference = "Stop"
$RepoRoot = Resolve-Path (Join-Path $PSScriptRoot "..\..\..\..")
& (Join-Path $RepoRoot "we.ps1") setup @Args
exit $LASTEXITCODE
