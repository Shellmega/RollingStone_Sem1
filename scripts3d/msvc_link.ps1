param([string]$Tool, [Parameter(ValueFromRemainingArguments=$true)][string[]]$LinkArgs)
# Ninja writes UTF-8 response files; MSVC lib.exe assumes the active ANSI code page.
# Convert response files to BOM-tagged UTF-16 without changing objects or options.
$ErrorActionPreference = 'Stop'
$temporary = @()
try {
    $forward = @()
    foreach ($argument in $LinkArgs) {
        if ($argument.StartsWith('@')) {
            $inputPath = [IO.Path]::GetFullPath($argument.Substring(1).Trim('"'))
            $outputPath = $inputPath + '.' + [Guid]::NewGuid().ToString('N') + '.utf16.rsp'
            $content = [IO.File]::ReadAllText($inputPath, [Text.Encoding]::UTF8)
            [IO.File]::WriteAllText($outputPath, $content, [Text.Encoding]::Unicode)
            $temporary += $outputPath
            $forward += '@' + $outputPath
        } else { $forward += $argument }
    }
    & $Tool @forward
    $code = $LASTEXITCODE
} finally {
    foreach ($path in $temporary) {
        if (Test-Path -LiteralPath $path) { Remove-Item -LiteralPath $path }
    }
}
exit $code
