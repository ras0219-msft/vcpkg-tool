. $PSScriptRoot/../end-to-end-tests-prelude.ps1

# Test simple installation
Run-Vcpkg -TestArgs ($commonArgs + @("install", "rapidjson", "--binarycaching", "--x-binarysource=clear;files,$ArchiveRoot,write"))
Throw-IfFailed

# Test simple removal
Run-Vcpkg -TestArgs ($commonArgs + @("remove", "rapidjson"))
Throw-IfFailed
Require-FileNotExists "$installRoot/$Triplet/include"

if(-Not $IsLinux) {
    # Test simple nuget installation
    Run-Vcpkg -TestArgs ($commonArgs + @("install", "rapidjson", "--binarycaching", "--x-binarysource=clear;nuget,$NuGetRoot,readwrite"))
    Throw-IfFailed
}

# Test restoring from files archive
Remove-Item -Recurse -Force $installRoot
Remove-Item -Recurse -Force $buildtreesRoot
Run-Vcpkg -TestArgs ($commonArgs + @("install","rapidjson","--binarycaching","--x-binarysource=clear;files,$ArchiveRoot,read"))
Throw-IfFailed
Require-FileExists "$installRoot/$Triplet/include/rapidjson/rapidjson.h"
Require-FileNotExists "$buildtreesRoot/rapidjson/src"
Require-FileExists "$buildtreesRoot/detect_compiler"

# Test --no-binarycaching
Remove-Item -Recurse -Force $installRoot
Remove-Item -Recurse -Force $buildtreesRoot
Run-Vcpkg -TestArgs ($commonArgs + @("install","rapidjson","--no-binarycaching","--x-binarysource=clear;files,$ArchiveRoot,read"))
Throw-IfFailed
Require-FileExists "$installRoot/$Triplet/include/rapidjson/rapidjson.h"
Require-FileExists "$buildtreesRoot/rapidjson/src"
Require-FileExists "$buildtreesRoot/detect_compiler"

# Test --editable
Remove-Item -Recurse -Force $installRoot
Remove-Item -Recurse -Force $buildtreesRoot
Run-Vcpkg -TestArgs ($commonArgs + @("install","rapidjson","--editable","--x-binarysource=clear;files,$ArchiveRoot,read"))
Throw-IfFailed
Require-FileExists "$installRoot/$Triplet/include/rapidjson/rapidjson.h"
Require-FileExists "$buildtreesRoot/rapidjson/src"
Require-FileNotExists "$buildtreesRoot/detect_compiler"

if(-Not $IsLinux) {
    # Test restoring from nuget
    Remove-Item -Recurse -Force $installRoot
    Remove-Item -Recurse -Force $buildtreesRoot
    Run-Vcpkg -TestArgs ($commonArgs + @("install", "rapidjson", "--binarycaching", "--x-binarysource=clear;nuget,$NuGetRoot"))
    Throw-IfFailed
    Require-FileExists "$installRoot/$Triplet/include/rapidjson/rapidjson.h"
    Require-FileNotExists "$buildtreesRoot/rapidjson/src"

    # Test four-phase flow
    Remove-Item -Recurse -Force $installRoot -ErrorAction SilentlyContinue
    Run-Vcpkg -TestArgs ($commonArgs + @("install", "rapidjson", "--dry-run", "--x-write-nuget-packages-config=$TestingRoot/packages.config"))
    Throw-IfFailed
    Require-FileNotExists "$installRoot/$Triplet/include/rapidjson/rapidjson.h"
    Require-FileNotExists "$buildtreesRoot/rapidjson/src"
    Require-FileExists "$TestingRoot/packages.config"
    $fetchNuGetArgs = $commonArgs + @('fetch', 'nuget')
    if ($IsLinux -or $IsMacOS) {
        mono $(Run-Vcpkg @fetchNuGetArgs) restore $TestingRoot/packages.config -OutputDirectory "$NuGetRoot2" -Source "$NuGetRoot"
    } else {
        & $(Run-Vcpkg @fetchNuGetArgs) restore $TestingRoot/packages.config -OutputDirectory "$NuGetRoot2" -Source "$NuGetRoot"
    }
    Throw-IfFailed
    Remove-Item -Recurse -Force $NuGetRoot -ErrorAction SilentlyContinue
    mkdir $NuGetRoot
    Run-Vcpkg -TestArgs ($commonArgs + @("install", "rapidjson", "zlib", "--binarycaching", "--x-binarysource=clear;nuget,$NuGetRoot2;nuget,$NuGetRoot,write"))
    Throw-IfFailed
    Require-FileExists "$installRoot/$Triplet/include/rapidjson/rapidjson.h"
    Require-FileExists "$installRoot/$Triplet/include/zlib.h"
    Require-FileNotExists "$buildtreesRoot/rapidjson/src"
    Require-FileExists "$buildtreesRoot/zlib/src"
    if ((Get-ChildItem $NuGetRoot -Filter '*.nupkg' | Measure-Object).Count -ne 1) {
        throw "In '$CurrentTest': did not create exactly 1 NuGet package"
    }

    # Test export
    $CurrentTest = 'Exporting'
    Write-Trace $CurrentTest
    if ((Get-ChildItem $TestingRoot -Filter '*.nupkg' | Measure-Object).Count -ne 0) {
        throw "Expected no nupkg's in root before test"
    }
    Run-Vcpkg -TestArgs ($commonArgs + @("export", "rapidjson", "zlib", "--x-chocolatey", "--nuget", "--nuget-id=vcpkg-export",
        "--nuget-version=1.0.0", "--output=vcpkg-export-output", "--output-dir=$TestingRoot"))
    Require-FileExists "$TestingRoot/vcpkg-export.1.0.0.nupkg"
    # Can't Require-FileExists because we don't know the version numbers of zlib and rapidjson
    if ((Get-ChildItem $NuGetRoot -Filter '*.nupkg' | Measure-Object).Count -ne 1) {
        throw "In '$CurrentTest': did not create exactly 3 NuGet packages"
    }
}

# Test export
$CurrentTest = 'Exporting'
Write-Trace $CurrentTest
Require-FileNotExists "$TestingRoot/vcpkg-export-output"
Require-FileNotExists "$TestingRoot/vcpkg-export-output.zip"
Run-Vcpkg -TestArgs ($commonArgs + @("export", "rapidjson", "zlib", "--output=vcpkg-export-output", "--raw", "--zip", "--output-dir=$TestingRoot"))
Require-FileExists "$TestingRoot/vcpkg-export-output"
Require-FileExists "$TestingRoot/vcpkg-export-output/installed/vcpkg/status"
Require-FileExists "$TestingRoot/vcpkg-export-output/installed/$Triplet/share/zlib"
Require-FileExists "$TestingRoot/vcpkg-export-output.zip"
