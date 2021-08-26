# Use as:
#   x-script,C:\path\to\pwsh.exe C:\path\to\terrapin.ps1 -az C:\path\to\az\cli\az.cmd -url ${url} -sha512 ${sha512} -path ${dst};x-block-origin

# You may need to run this first:
#   az login --scope https://mspmecloud.onmicrosoft.com/RebuildManager.Web/.default

[CmdletBinding(PositionalBinding=$False)]
Param(
    [Parameter(Mandatory=$True)]
    [String]$az,

    [Parameter(Mandatory=$True)]
    [String]$Url,

    [Parameter(Mandatory=$True)]
    [String]$Sha512,

    [Parameter(Mandatory=$True)]
    [String]$Path
)

$res_gat = & $az account get-access-token --resource https://mspmecloud.onmicrosoft.com/RebuildManager.Web | ConvertFrom-Json
if ($LASTEXITCODE -ne 0) {
    throw "Failed to execute `"$az account get-access-token --resource https://mspmecloud.onmicrosoft.com/RebuildManager.Web`" :`n$res_gat"
}

curl.exe `
    --fail `
    -L `
    -H "Authorization: Bearer $($res_gat.accessToken)" `
    "https://api.devpackages.microsoft.io/api/Vcpkg/${Sha512}?api-version=1.0" `
    --output $Path

if ($LASTEXITCODE -eq 0) {
    exit 0
}

$data = @{
    ArtifactUrl=$Url; ArtifactSha512=$Sha512
} | ConvertTo-Json -Compress | % { $_ -replace "`"","\`"" }

curl.exe `
    -X POST `
    -H "Authorization: Bearer $($res_gat.accessToken)" `
    -H "Content-Type: application/json" `
    -d $data `
    "https://api.devpackages.microsoft.io/api/Vcpkg?api-version=1.0"

curl.exe `
    --fail `
    -L `
    -H "Authorization: Bearer $($res_gat.accessToken)" `
    "https://api.devpackages.microsoft.io/api/Vcpkg/${Sha512}?api-version=1.0" `
    --output $Path

exit $LASTEXITCODE
