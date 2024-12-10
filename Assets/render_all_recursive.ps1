param(
    [Parameter(Mandatory=$true)]
    [string]$FolderPath,

    [Parameter(Mandatory=$true)]
    [string]$ExecutablePath
)

# Check if the folder exists
if (-not (Test-Path -Path $FolderPath -PathType Container)) {
    Write-Error "Error: Folder '$FolderPath' does not exist or is not accessible."
    exit 1
}

# Check if the executable exists
if (-not (Test-Path -Path $ExecutablePath -PathType Leaf)) {
    Write-Error "Error: Executable '$ExecutablePath' does not exist or is not accessible."
    exit 1
}

# Get all XML files recursively in the specified directory and its subfolders
Get-ChildItem -Path $FolderPath -Filter "*.xml" -Recurse | ForEach-Object {
    $cmd = "& '$ExecutablePath' '$($_.FullName)'"
    Write-Host $cmd
    Invoke-Expression $cmd
}
