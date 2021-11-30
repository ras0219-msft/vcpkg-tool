#pragma once

#include <vcpkg/base/fwd/files.h>

#include <vcpkg/fwd/build.h>
#include <vcpkg/fwd/packagespec.h>
#include <vcpkg/fwd/vcpkgpaths.h>

namespace vcpkg::PostBuildLint
{
    size_t perform_all_checks(const PackageSpec& spec,
                              const VcpkgPaths& paths,
                              const Build::PreBuildInfo& pre_build_info,
                              const Build::BuildInfo& build_info,
                              const Path& port_dir);
}
