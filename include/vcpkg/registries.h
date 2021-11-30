#pragma once

#include <vcpkg/base/fwd/files.h>
#include <vcpkg/base/fwd/json.h>

#include <vcpkg/fwd/configuration.h>
#include <vcpkg/fwd/registries.h>
#include <vcpkg/fwd/vcpkgpaths.h>

#include <vcpkg/base/stringview.h>
#include <vcpkg/base/view.h>

#include <vcpkg/versiondeserializers.h>
#include <vcpkg/versiont.h>

#include <map>
#include <memory>
#include <string>
#include <system_error>
#include <vector>

namespace vcpkg
{
    struct LockFile
    {
        struct EntryData
        {
            std::string reference;
            std::string commit_id;
            bool stale;
        };

        using LockDataType = std::multimap<std::string, EntryData, std::less<>>;
        struct Entry
        {
            LockFile* lockfile;
            LockDataType::iterator data;

            const std::string& reference() const { return data->second.reference; }
            const std::string& commit_id() const { return data->second.commit_id; }
            bool stale() const { return data->second.stale; }
            const std::string& uri() const { return data->first; }

            void ensure_up_to_date(const VcpkgPaths& paths) const;
        };

        Entry get_or_fetch(const VcpkgPaths& paths, StringView repo, StringView reference);

        LockDataType lockdata;
        bool modified = false;
    };

    struct RegistryEntry
    {
        virtual View<VersionT> get_port_versions() const = 0;

        virtual ExpectedS<Path> get_path_to_version(const VersionT& version) const = 0;

        virtual ~RegistryEntry() = default;
    };

    struct RegistryImplementation
    {
        virtual StringLiteral kind() const = 0;

        // returns nullptr if the port doesn't exist
        virtual std::unique_ptr<RegistryEntry> get_port_entry(StringView port_name) const = 0;

        // appends the names of the ports to the out parameter
        // may result in duplicated port names; make sure to Util::sort_unique_erase at the end
        virtual void get_all_port_names(std::vector<std::string>& port_names) const = 0;

        virtual Optional<VersionT> get_baseline_version(StringView port_name) const = 0;

        virtual Optional<Path> get_path_to_baseline_version(StringView port_name) const;

        virtual ~RegistryImplementation() = default;
    };

    struct Registry
    {
        // requires: static_cast<bool>(implementation)
        Registry(std::vector<std::string>&& packages, std::unique_ptr<RegistryImplementation>&& implementation);

        Registry(std::vector<std::string>&&, std::nullptr_t) = delete;

        // always ordered lexicographically
        View<std::string> packages() const { return packages_; }
        const RegistryImplementation& implementation() const { return *implementation_; }

        friend RegistrySet; // for experimental_set_builtin_registry_baseline

    private:
        std::vector<std::string> packages_;
        std::unique_ptr<RegistryImplementation> implementation_;
    };

    // this type implements the registry fall back logic from the registries RFC:
    // A port name maps to one of the non-default registries if that registry declares
    // that it is the registry for that port name, else it maps to the default registry
    // if that registry exists; else, there is no registry for a port.
    // The way one sets this up is via the `"registries"` and `"default_registry"`
    // configuration fields.
    struct RegistrySet
    {
        RegistrySet(std::unique_ptr<RegistryImplementation>&& x, std::vector<Registry>&& y)
            : default_registry_(std::move(x)), registries_(std::move(y))
        {
        }

        // finds the correct registry for the port name
        // Returns the null pointer if there is no registry set up for that name
        const RegistryImplementation* registry_for_port(StringView port_name) const;
        Optional<VersionT> baseline_for_port(StringView port_name) const;

        View<Registry> registries() const { return registries_; }

        const RegistryImplementation* default_registry() const { return default_registry_.get(); }

        bool is_default_builtin_registry() const;

        // returns whether the registry set has any modifications to the default
        // (i.e., whether `default_registry` was set, or `registries` had any entries)
        // for checking against the registry feature flag.
        bool has_modifications() const;

    private:
        std::unique_ptr<RegistryImplementation> default_registry_;
        std::vector<Registry> registries_;
    };

    std::unique_ptr<RegistryImplementation> make_builtin_registry(const VcpkgPaths& paths);
    std::unique_ptr<RegistryImplementation> make_builtin_registry(const VcpkgPaths& paths, std::string baseline);
    std::unique_ptr<RegistryImplementation> make_git_registry(const VcpkgPaths& paths,
                                                              std::string repo,
                                                              std::string reference,
                                                              std::string baseline);
    std::unique_ptr<RegistryImplementation> make_filesystem_registry(const Filesystem& fs,
                                                                     Path path,
                                                                     std::string baseline);

    ExpectedS<std::vector<std::pair<SchemedVersion, std::string>>> get_builtin_versions(const VcpkgPaths& paths,
                                                                                        StringView port_name);

    ExpectedS<std::map<std::string, VersionT, std::less<>>> get_builtin_baseline(const VcpkgPaths& paths);

    bool is_git_commit_sha(StringView sv);

    struct VersionDbEntry
    {
        VersionT version;
        Versions::Scheme scheme = Versions::Scheme::String;

        // only one of these may be non-empty
        std::string git_tree;
        Path p;
    };

    // VersionDbType::Git => VersionDbEntry.git_tree is filled
    // VersionDbType::Filesystem => VersionDbEntry.path is filled
    enum class VersionDbType
    {
        Git,
        Filesystem,
    };

    ExpectedS<std::vector<VersionDbEntry>> deserialize_version_db_array(const Json::Value& v,
                                                                        VersionDbType type,
                                                                        const Path& root);
}
