param([string]$OutputPath = (Join-Path $PSScriptRoot 'ChannelTTS-1.5.0-win64.ts3_plugin'))
$ErrorActionPreference='Stop'
Add-Type -AssemblyName System.IO.Compression
$dll=Join-Path $PSScriptRoot 'channel_tts_win64.dll'
if(!(Test-Path -LiteralPath $dll)){throw '请先构建并将 DLL 复制到项目根目录。'}
$stream=[System.IO.File]::Open($OutputPath,[System.IO.FileMode]::Create)
$zip=[System.IO.Compression.ZipArchive]::new($stream,[System.IO.Compression.ZipArchiveMode]::Create)
try {
 foreach($item in @(@('package.ini',(Join-Path $PSScriptRoot 'package.ini')),@('plugins/channel_tts_win64.dll',$dll))) {
  $entry=$zip.CreateEntry($item[0],[System.IO.Compression.CompressionLevel]::Optimal)
  $output=$entry.Open();$inputFile=[System.IO.File]::OpenRead($item[1])
  try{$inputFile.CopyTo($output)}finally{$inputFile.Dispose();$output.Dispose()}
 }
}finally{$zip.Dispose();$stream.Dispose()}
Write-Output $OutputPath

