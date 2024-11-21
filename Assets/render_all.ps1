param(
    [Parameter(Mandatory=$true)]
    [string]$FolderPath
)

# Check if the folder exists
if (-not (Test-Path -Path $FolderPath -PathType Container)) {
    Write-Error "Error: Folder '$FolderPath' does not exist or is not accessible."
    exit 1
}

# Get all XML files in the specified directory
Get-ChildItem -Path $FolderPath -Filter "*.xml" | ForEach-Object {
    $cmd = ".\raytracer.exe $($_.FullName)"
    Write-Host $cmd
    Invoke-Expression $cmd
}
