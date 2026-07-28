//! Copyright (c) 2026, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
//!
//! SPDX-License-Identifier: GPL-2.0-or-later

const std = @import("std");
const builtin = @import("builtin");

const project_version = "1.5.0";

pub fn build(b: *std.Build) void {
    const requested_target = b.standardTargetOptions(.{});
    const optimize = b.standardOptimizeOption(.{});

    // Prefer the MSVC ABI on Windows so we can consume vcpkg packages built for it.
    const target = resolveTarget(b, requested_target);

    const enable_dynarec = b.option(bool, "enable_dynarec", "Enable the dynamic recompiler (x86/x86_64 only)") orelse true;
    const build_win32_opt = b.option(bool, "build_win32", "Build the Win32 frontend and plugins") orelse null;
    const link_static = b.option(bool, "link_static", "Link vcpkg dependencies statically") orelse false;
    const build_tests = b.option(bool, "tests", "Build unit tests and the Lua test library") orelse true;
    const vcpkg_installed_opt = b.option([]const u8, "vcpkg_installed", "Path to a vcpkg installed triplet directory");
    const version_suffix_opt = b.option([]const u8, "version_suffix", "Version suffix embedded in the binary");
    const nightly_opt = b.option(bool, "nightly", "Build as a nightly (affects icons/branding)") orelse null;

    const target_is_windows = target.result.os.tag == .windows;
    const build_win32 = build_win32_opt orelse target_is_windows;

    if (target_is_windows and builtin.os.tag == .linux) {
        const triplet = vcpkgTriplet(target, link_static);
        std.log.warn(
            "Linux host detected: cross-compiling for Windows with Zig/libc++ (vcpkg triplet '{s}').\n" ++
                "Run the matching Zed vcpkg install task before building if dependencies are missing.",
            .{triplet},
        );
    }

    if (build_win32 and !target_is_windows) {
        @panic("-Dbuild_win32 requires a Windows target");
    }
    if (!target_is_windows) {
        std.log.warn("Building on platforms other than Windows is experimental.", .{});
    }

    const version_suffix = version_suffix_opt orelse b.graph.environ_map.get("VERSION_SUFFIX") orelse "";
    const nightly: bool = blk: {
        if (nightly_opt) |n| break :blk n;
        const env = b.graph.environ_map.get("NIGHTLY") orelse break :blk false;
        break :blk !(std.mem.eql(u8, env, "") or std.mem.eql(u8, env, "0"));
    };

    std.log.info("Full version name: {s}{s}", .{ project_version, version_suffix });
    std.log.info("Nightly build: {}", .{nightly});

    const arch = target.result.cpu.arch;
    const is_x86_family = arch == .x86 or arch == .x86_64;
    if (enable_dynarec and !is_x86_family) {
        @panic("Dynarec is not supported on this architecture");
    }

    const target_uses_msvc = target_is_windows and target.result.abi == .msvc;
    const cxx_flags = collectCxxFlags(b, optimize, is_x86_family, target_uses_msvc);
    const c_flags = collectCFlags(b, optimize, is_x86_family, target_uses_msvc);

    const vcpkg: ?VcpkgPaths = if (target_is_windows)
        resolveVcpkg(b, target, link_static, optimize, vcpkg_installed_opt)
    else
        null;

    // ── Vendored libraries ──────────────────────────────────────────────
    const aladdin_md5 = addStaticLib(b, target, optimize, "aladdin-md5", &.{
        "vendor/aladdin-md5/md5.c",
    }, c_flags, &.{b.path("vendor/aladdin-md5")});

    const hqx_flags = blk: {
        var flags: std.ArrayListUnmanaged([]const u8) = .empty;
        flags.appendSlice(b.allocator, c_flags) catch @panic("OOM");
        flags.append(b.allocator, "-w") catch @panic("OOM");
        break :blk flags.toOwnedSlice(b.allocator) catch @panic("OOM");
    };
    const hqx = addStaticLib(b, target, optimize, "hqx", &.{
        "vendor/hqx/init.c",
        "vendor/hqx/hq2x.c",
        "vendor/hqx/hq3x.c",
        "vendor/hqx/hq4x.c",
    }, hqx_flags, &.{b.path("vendor/hqx")});

    const xbrz = addStaticLib(b, target, optimize, "xbrz", &.{
        "vendor/xbrz/xbrz.cpp",
    }, cxx_flags, &.{b.path("vendor/xbrz")});

    // Header-only vendor include roots
    const vendor_argh = b.path("vendor/argh");
    const vendor_bs_thread_pool = b.path("vendor/bs-thread-pool");
    const vendor_microlru = b.path("vendor/microlru");
    const vendor_mini = b.path("vendor/mini");
    const vendor_xxhash64 = b.path("vendor/xxhash64");
    const vendor_windarkmode = b.path("vendor/windarkmode");

    // ── Common (interface / headers) ────────────────────────────────────
    // Common is header-only; consumers get its include path and deps.
    const common_inc = b.path("src/Common/include");
    const common_win32_inc = b.path("src/Common.Win32/include");
    const core_inc = b.path("src/Core/include");
    const views_headers_inc = b.path("src/Views.Win32/include");

    // ── Core ────────────────────────────────────────────────────────────
    var core_sources: std.ArrayListUnmanaged([]const u8) = .empty;
    core_sources.appendSlice(b.allocator, &core_sources_common) catch @panic("OOM");
    if (enable_dynarec) {
        const dynarec_dir: []const u8 = switch (arch) {
            .x86_64 => "src/Core/R4300/x86_64",
            .x86 => "src/Core/R4300/x86",
            else => unreachable,
        };
        for (dynarec_sources) |name| {
            core_sources.append(b.allocator, b.fmt("{s}/{s}", .{ dynarec_dir, name })) catch @panic("OOM");
        }
    }

    const core = b.addLibrary(.{
        .name = "Core",
        .linkage = .static,
        .root_module = createCppModule(b, target, optimize),
    });
    core.root_module.addCSourceFiles(.{
        .files = core_sources.items,
        .flags = withIncludePrefix(b, cxx_flags, "src/Common/include/CommonPCH.hpp"),
        .language = .cpp,
    });
    core.root_module.addIncludePath(core_inc);
    core.root_module.addIncludePath(b.path("src/Core"));
    core.root_module.addIncludePath(common_inc);
    core.root_module.addIncludePath(vendor_xxhash64);
    core.root_module.addIncludePath(b.path("vendor/aladdin-md5"));
    // CommonPCH.hpp pulls in SDL3 headers unconditionally.
    if (vcpkg) |vp| {
        applyVcpkgIncludes(core.root_module, vp);
        applyVcpkgLibPaths(core.root_module, vp);
    }
    if (enable_dynarec) core.root_module.addCMacro("MUPEN64RR_ENABLE_DYNAREC", "1");
    applyCommonDefines(core.root_module, optimize, target_is_windows, version_suffix, nightly);

    core.root_module.linkLibrary(aladdin_md5);
    linkLibdeflate(core.root_module, vcpkg);
    if (!target_is_windows) {
        core.root_module.linkSystemLibrary("safec", .{});
    }

    // ── Tests ───────────────────────────────────────────────────────────
    var core_tests_exe: ?*std.Build.Step.Compile = null;
    var lua_testlib: ?*std.Build.Step.Compile = null;

    if (build_tests) {
        const tests = b.addExecutable(.{
            .name = "Core.Tests",
            .root_module = createCppModule(b, target, optimize),
        });
        tests.root_module.addCSourceFiles(.{
            .files = &.{
                "src/Core.Tests/IOUtilsTests.cpp",
                "src/Core.Tests/VCRTests.cpp",
                "src/Core.Tests/VRTests.cpp",
            },
            .flags = withIncludePrefix(b, cxx_flags, "src/Core.Tests/stdafx.h"),
            .language = .cpp,
        });
        tests.root_module.addIncludePath(b.path("src/Core.Tests"));
        tests.root_module.addIncludePath(b.path("src"));
        tests.root_module.addIncludePath(b.path("src/Core"));
        tests.root_module.addIncludePath(core_inc);
        tests.root_module.addIncludePath(common_inc);
        tests.root_module.addIncludePath(vendor_xxhash64);
        if (vcpkg) |vp| {
            applyVcpkgIncludes(tests.root_module, vp);
            applyVcpkgLibPaths(tests.root_module, vp);
            linkCatch2(tests.root_module, vp, optimize);
            linkLibdeflate(tests.root_module, vp);
        } else {
            tests.root_module.linkSystemLibrary("catch2-with-main", .{});
            linkLibdeflate(tests.root_module, null);
        }
        if (enable_dynarec) tests.root_module.addCMacro("MUPEN64RR_ENABLE_DYNAREC", "1");
        applyCommonDefines(tests.root_module, optimize, target_is_windows, version_suffix, nightly);
        tests.root_module.linkLibrary(core);
        tests.root_module.linkLibrary(aladdin_md5);
        // Console entry (Catch2 provides main, not WinMain).
        tests.entry = .{ .symbol_name = "mainCRTStartup" };
        applyWindowsLinkFlags(tests, target);
        core_tests_exe = tests;

        const luatest = b.addLibrary(.{
            .name = "luatestlib",
            .linkage = .dynamic,
            .root_module = createCppModule(b, target, optimize),
        });
        luatest.root_module.pic = true;
        luatest.root_module.addCSourceFiles(.{
            .files = &.{"src/Lua.TestLib/main.cpp"},
            .flags = cxx_flags,
            .language = .cpp,
        });
        if (vcpkg) |vp| {
            applyVcpkgIncludes(luatest.root_module, vp);
            applyVcpkgLibPaths(luatest.root_module, vp);
            linkLua(luatest.root_module, vp);
        } else {
            luatest.root_module.linkSystemLibrary("lua", .{});
        }
        applyCommonDefines(luatest.root_module, optimize, target_is_windows, version_suffix, nightly);
        applyWindowsLinkFlags(luatest, target);
        lua_testlib = luatest;
    }

    // ── Win32 frontend + plugins ────────────────────────────────────────
    var mupen_exe: ?*std.Build.Step.Compile = null;
    var plugins: std.ArrayListUnmanaged(*std.Build.Step.Compile) = .empty;

    if (build_win32) {
        if (vcpkg) |vp| {
            const views = b.addExecutable(.{
                .name = "mupen64",
                .root_module = createCppModule(b, target, optimize),
            });
            views.subsystem = .Windows;
            // App provides WinMain (ANSI LPSTR), not wWinMain, despite UNICODE APIs.
            views.entry = .{ .symbol_name = "WinMainCRTStartup" };
            views.linker_dynamicbase = false;
            views.root_module.addCSourceFiles(.{
                .files = &views_sources,
                .flags = withIncludePrefix(b, cxx_flags, "src/Views.Win32/stdafx.h"),
                .language = .cpp,
            });
            views.root_module.addWin32ResourceFile(.{
                .file = b.path("src/Views.Win32/Resource.rc"),
                .include_paths = resourceIncludePaths(b, target, &.{b.path("src/Views.Win32")}),
            });
            views.root_module.addIncludePath(b.path("src/Views.Win32"));
            views.root_module.addIncludePath(views_headers_inc);
            views.root_module.addIncludePath(common_inc);
            views.root_module.addIncludePath(common_win32_inc);
            views.root_module.addIncludePath(core_inc);
            views.root_module.addIncludePath(vendor_argh);
            views.root_module.addIncludePath(vendor_bs_thread_pool);
            views.root_module.addIncludePath(vendor_microlru);
            views.root_module.addIncludePath(vendor_mini);
            views.root_module.addIncludePath(vendor_xxhash64);
            views.root_module.addIncludePath(vendor_windarkmode);
            applyVcpkgIncludes(views.root_module, vp);
            applyVcpkgLibPaths(views.root_module, vp);
            applyCommonDefines(views.root_module, optimize, true, version_suffix, nightly);
            views.root_module.addCMacro("MINI_CASE_SENSITIVE", "1");
            if (enable_dynarec) views.root_module.addCMacro("MUPEN64RR_ENABLE_DYNAREC", "1");
            applySpdlogDefines(views.root_module, link_static);

            views.root_module.linkLibrary(core);
            views.root_module.linkLibrary(aladdin_md5);
            linkLibdeflate(views.root_module, vp);
            linkLua(views.root_module, vp);
            linkSpdlog(views.root_module, vp, optimize);
            linkSdl3(views.root_module, vp);
            linkNlohmannJson(views.root_module, vp);
            linkSpeexdsp(views.root_module, vp);
            linkViewsWinLibs(views.root_module);
            views.root_module.linkSystemLibrary("comdlg32", .{});
            applyWindowsLinkFlags(views, target);
            mupen_exe = views;

            // Plugin common include/macro helper applied per plugin
            const plugin_common = PluginCommon{
                .common_inc = common_inc,
                .common_win32_inc = common_win32_inc,
                .core_inc = core_inc,
                .views_headers_inc = views_headers_inc,
                .vendor_xxhash64 = vendor_xxhash64,
                .vcpkg = vp,
                .cxx_flags = cxx_flags,
                .version_suffix = version_suffix,
                .nightly = nightly,
                .optimize = optimize,
                .link_static = link_static,
                .enable_dynarec = enable_dynarec,
            };

            plugins.append(b.allocator, addPlugin(b, target, plugin_common, "NoAudio", &.{
                "src/Plugins.Win32.DummyAudio/Main.cpp",
            }, null, &.{})) catch @panic("OOM");

            plugins.append(b.allocator, addPlugin(b, target, plugin_common, "NoInput", &.{
                "src/Plugins.Win32.DummyInput/Main.cpp",
            }, null, &.{})) catch @panic("OOM");

            plugins.append(b.allocator, addPlugin(b, target, plugin_common, "NoVideo", &.{
                "src/Plugins.Win32.DummyVideo/Main.cpp",
            }, null, &.{})) catch @panic("OOM");

            const tas_audio = addPlugin(b, target, plugin_common, "TASAudio", &.{
                "src/Plugins.Win32.TASAudio/Config.cpp",
                "src/Plugins.Win32.TASAudio/Config_Win32.cpp",
                "src/Plugins.Win32.TASAudio/Main.cpp",
                "src/Plugins.Win32.TASAudio/Main_Win32.cpp",
                "src/Plugins.Win32.TASAudio/SDLBackend.cpp",
            }, "src/Plugins.Win32.TASAudio/Resource.rc", &.{
                b.path("src/Plugins.Win32.TASAudio"),
            });
            linkSdl3(tas_audio.root_module, vp);
            linkNlohmannJson(tas_audio.root_module, vp);
            plugins.append(b.allocator, tas_audio) catch @panic("OOM");

            const tas_input = addPlugin(b, target, plugin_common, "TASInput", &.{
                "src/Plugins.Win32.TASInput/TASInput.cpp",
                "src/Plugins.Win32.TASInput/NewConfig.cpp",
                "src/Plugins.Win32.TASInput/Main.cpp",
                "src/Plugins.Win32.TASInput/GamepadManager.cpp",
                "src/Plugins.Win32.TASInput/ConfigDialog.cpp",
                "src/Plugins.Win32.TASInput/Combo.cpp",
            }, "src/Plugins.Win32.TASInput/Resource.rc", &.{
                b.path("src/Plugins.Win32.TASInput"),
            });
            linkSdl3(tas_input.root_module, vp);
            linkNlohmannJson(tas_input.root_module, vp);
            for ([_][]const u8{ "setupapi", "hid", "imm32", "version", "winmm", "comctl32", "shcore", "gdiplus" }) |lib| {
                tas_input.root_module.linkSystemLibrary(lib, .{});
            }
            plugins.append(b.allocator, tas_input) catch @panic("OOM");

            const tas_rsp = addPlugin(b, target, plugin_common, "TASRSP", &.{
                "src/Plugins.Win32.TASRSP/Config.cpp",
                "src/Plugins.Win32.TASRSP/JPEG.cpp",
                "src/Plugins.Win32.TASRSP/Main.cpp",
                "src/Plugins.Win32.TASRSP/MP3.cpp",
                "src/Plugins.Win32.TASRSP/UCode1.cpp",
                "src/Plugins.Win32.TASRSP/UCode2.cpp",
                "src/Plugins.Win32.TASRSP/UCode3.cpp",
            }, null, &.{
                b.path("src/Plugins.Win32.TASRSP"),
            });
            linkNlohmannJson(tas_rsp.root_module, vp);
            plugins.append(b.allocator, tas_rsp) catch @panic("OOM");

            const tas_video = addPlugin(b, target, plugin_common, "TASVideo", &.{
                "src/Plugins.Win32.TASVideo/VI.cpp",
                "src/Plugins.Win32.TASVideo/Textures.cpp",
                "src/Plugins.Win32.TASVideo/unified_combiner.cpp",
                "src/Plugins.Win32.TASVideo/stdafx.cpp",
                "src/Plugins.Win32.TASVideo/S2DEX2.cpp",
                "src/Plugins.Win32.TASVideo/S2DEX.cpp",
                "src/Plugins.Win32.TASVideo/RSP.cpp",
                "src/Plugins.Win32.TASVideo/RDP.cpp",
                "src/Plugins.Win32.TASVideo/OpenGL.cpp",
                "src/Plugins.Win32.TASVideo/N64.cpp",
                "src/Plugins.Win32.TASVideo/L3DEX2.cpp",
                "src/Plugins.Win32.TASVideo/L3DEX.cpp",
                "src/Plugins.Win32.TASVideo/L3D.cpp",
                "src/Plugins.Win32.TASVideo/gSP.cpp",
                "src/Plugins.Win32.TASVideo/glN64.cpp",
                "src/Plugins.Win32.TASVideo/gDP.cpp",
                "src/Plugins.Win32.TASVideo/GBI.cpp",
                "src/Plugins.Win32.TASVideo/FrameBuffer.cpp",
                "src/Plugins.Win32.TASVideo/F3DWRUS.cpp",
                "src/Plugins.Win32.TASVideo/F3DPD.cpp",
                "src/Plugins.Win32.TASVideo/F3DEX2.cpp",
                "src/Plugins.Win32.TASVideo/F3DEX.cpp",
                "src/Plugins.Win32.TASVideo/F3DDKR.cpp",
                "src/Plugins.Win32.TASVideo/F3D.cpp",
                "src/Plugins.Win32.TASVideo/DepthBuffer.cpp",
                "src/Plugins.Win32.TASVideo/CRC.cpp",
                "src/Plugins.Win32.TASVideo/Config.cpp",
                "src/Plugins.Win32.TASVideo/Combiner.cpp",
                "src/Plugins.Win32.TASVideo/2xSAI.cpp",
            }, "src/Plugins.Win32.TASVideo/Resource.rc", &.{
                b.path("src/Plugins.Win32.TASVideo"),
                b.path("vendor/hqx"),
                b.path("vendor/xbrz"),
            });
            tas_video.root_module.linkLibrary(hqx);
            tas_video.root_module.linkLibrary(xbrz);
            linkNlohmannJson(tas_video.root_module, vp);
            linkGlew(tas_video.root_module, vp, optimize, link_static);
            linkSdl3(tas_video.root_module, vp);
            for ([_][]const u8{ "comctl32", "uxtheme", "msimg32", "gdiplus", "opengl32", "glu32", "winmm" }) |lib| {
                tas_video.root_module.linkSystemLibrary(lib, .{});
            }
            plugins.append(b.allocator, tas_video) catch @panic("OOM");
        } else {
            std.log.warn("Win32 artifacts are unavailable until vcpkg dependencies are installed. Run `zig build vcpkg`.", .{});
        }
    }

    // ── Install layout (mirrors former CMake output dirs) ───────────────
    //   <prefix>/out/mupen64.exe
    //   <prefix>/out/plugin/*.dll
    //   <prefix>/test/out/Core.Tests.exe
    //   <prefix>/test/out/luatestlib.dll
    if (mupen_exe) |exe| {
        b.getInstallStep().dependOn(&b.addInstallArtifact(exe, .{
            .dest_dir = .{ .override = .{ .custom = "out" } },
        }).step);
        if (vcpkg) |vp| {
            if (!link_static) {
                b.getInstallStep().dependOn(&b.addInstallDirectory(.{
                    .source_dir = .{ .cwd_relative = vp.bin_dir },
                    .install_dir = .{ .custom = "out" },
                    .install_subdir = "",
                    .include_extensions = &.{".dll"},
                }).step);
                // The Zig/MinGW speexdsp build emits liblibspeexdsp.dll, while
                // its import library records the runtime name libspeexdsp.dll.
                const speex_dll = b.fmt("{s}/liblibspeexdsp.dll", .{vp.bin_dir});
                if (std.Io.Dir.cwd().access(b.graph.io, speex_dll, .{})) |_| {
                    b.getInstallStep().dependOn(&b.addInstallFile(.{ .cwd_relative = speex_dll }, "out/libspeexdsp.dll").step);
                } else |_| {}
            }
        }
    }
    for (plugins.items) |plugin| {
        b.getInstallStep().dependOn(&b.addInstallArtifact(plugin, .{
            .dest_dir = .{ .override = .{ .custom = "out/plugin" } },
        }).step);
    }
    if (core_tests_exe) |tests| {
        b.getInstallStep().dependOn(&b.addInstallArtifact(tests, .{
            .dest_dir = .{ .override = .{ .custom = "test/out" } },
        }).step);
        if (vcpkg) |vp| {
            if (!link_static) {
                b.getInstallStep().dependOn(&b.addInstallDirectory(.{
                    .source_dir = .{ .cwd_relative = vp.bin_dir },
                    .install_dir = .{ .custom = "test/out" },
                    .install_subdir = "",
                    .include_extensions = &.{".dll"},
                }).step);
            }
        }
    }
    if (lua_testlib) |lib| {
        b.getInstallStep().dependOn(&b.addInstallArtifact(lib, .{
            .dest_dir = .{ .override = .{ .custom = "test/out" } },
        }).step);
    }

    // ── Test step ───────────────────────────────────────────────────────
    if (core_tests_exe) |tests| {
        const test_step = b.step("test", "Run Core.Tests (one process per case)");
        if (target.result.os.tag == builtin.os.tag) {
            const isolated = addIsolatedCatchTests(b, tests, if (vcpkg) |vp| vp.bin_dir else null);
            test_step.dependOn(&isolated.step);
        } else {
            // A cross-compiled test executable cannot run on the build host.
            test_step.dependOn(&tests.step);
        }
    }

    // ── Convenience: vcpkg install helper ───────────────────────────────
    const vcpkg_step = b.step("vcpkg", "Install vcpkg dependencies for the active target triplet");
    if (target_is_windows) {
        const triplet = vcpkgTriplet(target, link_static);
        // Keep the historical layout under build/ so CI caches and local trees match.
        const install_root = "build/vcpkg_installed";
        const vcpkg_exe = findVcpkg(b);
        const run = b.addSystemCommand(&.{vcpkg_exe});
        run.addArgs(&.{
            "install",
            b.fmt("--triplet={s}", .{triplet}),
            "--overlay-triplets=triplets",
            "--overlay-ports=ports",
            "--x-manifest-root=.",
            b.fmt("--x-install-root={s}", .{install_root}),
        });
        vcpkg_step.dependOn(&run.step);
    }
}

fn findVcpkg(b: *std.Build) []const u8 {
    // Arch packages vcpkg separately from its ports checkout. When that
    // checkout has been bootstrapped, prefer its matching executable over a
    // potentially older system wrapper on PATH.
    const exe_name = if (builtin.os.tag == .windows) "vcpkg.exe" else "vcpkg";
    if (b.graph.environ_map.get("VCPKG_ROOT")) |root| {
        const candidate = b.fmt("{s}/{s}", .{ root, exe_name });
        if (std.Io.Dir.cwd().access(b.graph.io, candidate, .{})) |_| {
            return candidate;
        } else |_| {}
    }
    // Fish does not source Arch's /etc/profile.d/vcpkg.sh, so Zed tasks might
    // not inherit VCPKG_ROOT. Support Arch's documented default directly.
    if (builtin.os.tag != .windows) {
        if (b.graph.environ_map.get("HOME")) |home| {
            const candidate = b.fmt("{s}/.local/share/vcpkg/{s}", .{ home, exe_name });
            if (std.Io.Dir.cwd().access(b.graph.io, candidate, .{})) |_| {
                return candidate;
            } else |_| {}
        }
    }
    if (b.findProgram(&.{"vcpkg"}, &.{})) |p| return p else |_| {}
    if (b.graph.environ_map.get("VCINSTALLDIR")) |vc| {
        const candidate = b.fmt("{s}/vcpkg/vcpkg.exe", .{vc});
        std.Io.Dir.cwd().access(b.graph.io, candidate, .{}) catch {
            return "vcpkg";
        };
        return candidate;
    }
    return "vcpkg";
}

fn addIsolatedCatchTests(b: *std.Build, tests: *std.Build.Step.Compile, vcpkg_bin: ?[]const u8) *IsolatedCatchTests {
    const isolated = b.allocator.create(IsolatedCatchTests) catch @panic("OOM");
    isolated.* = .{
        .step = std.Build.Step.init(.{
            .id = .custom,
            .name = "run Core.Tests (isolated)",
            .owner = b,
            .makeFn = IsolatedCatchTests.make,
        }),
        .exe = tests,
        .vcpkg_bin = if (vcpkg_bin) |p| b.dupePath(p) else null,
    };
    isolated.step.dependOn(&tests.step);
    return isolated;
}

const IsolatedCatchTests = struct {
    step: std.Build.Step,
    exe: *std.Build.Step.Compile,
    vcpkg_bin: ?[]const u8,

    fn make(step: *std.Build.Step, options: std.Build.Step.MakeOptions) !void {
        _ = options;
        const self: *IsolatedCatchTests = @fieldParentPtr("step", step);
        const b = step.owner;
        const allocator = b.allocator;

        const exe_path = self.exe.getEmittedBin().getPath2(b, step);

        var env_map = std.process.Environ.Map.init(allocator);
        defer env_map.deinit();
        for (b.graph.environ_map.keys(), b.graph.environ_map.values()) |key, value| {
            try env_map.put(key, value);
        }
        if (self.vcpkg_bin) |bin_dir| {
            const old_path = env_map.get("PATH") orelse "";
            const sep = if (builtin.os.tag == .windows) ";" else ":";
            const new_path = try std.fmt.allocPrint(allocator, "{s}{s}{s}", .{ bin_dir, sep, old_path });
            try env_map.put("PATH", new_path);
        }

        // 1) Enumerate cases.
        // Catch2 v3: one name per line when quiet.
        const list_result = try std.process.run(allocator, b.graph.io, .{
            .argv = &.{ exe_path, "--list-tests", "--verbosity", "quiet" },
            .environ_map = &env_map,
        });
        defer allocator.free(list_result.stdout);
        defer allocator.free(list_result.stderr);

        switch (list_result.term) {
            .exited => |code| if (code != 0) {
                return step.fail("failed to list Catch2 tests (exit {d}):\n{s}{s}", .{
                    code,
                    list_result.stdout,
                    list_result.stderr,
                });
            },
            else => return step.fail("failed to list Catch2 tests ({any}):\n{s}{s}", .{
                list_result.term,
                list_result.stdout,
                list_result.stderr,
            }),
        }

        var names: std.ArrayListUnmanaged([]const u8) = .empty;
        defer {
            for (names.items) |n| allocator.free(n);
            names.deinit(allocator);
        }

        var line_it = std.mem.splitScalar(u8, list_result.stdout, '\n');
        while (line_it.next()) |raw| {
            const line = std.mem.trim(u8, raw, " \t\r");
            if (line.len == 0) continue;
            try names.append(allocator, try allocator.dupe(u8, line));
        }

        if (names.items.len == 0) {
            return step.fail("Catch2 reported zero tests", .{});
        }

        std.log.info("Running {d} Catch2 cases in isolated processes...", .{names.items.len});

        // 2) One fresh process per case.
        var failures: usize = 0;
        for (names.items) |name| {
            const case_result = try std.process.run(allocator, b.graph.io, .{
                .argv = &.{ exe_path, name, "--order", "decl", "--rng-seed", "0" },
                .environ_map = &env_map,
            });
            defer allocator.free(case_result.stdout);
            defer allocator.free(case_result.stderr);

            const ok = switch (case_result.term) {
                .exited => |code| code == 0,
                else => false,
            };
            if (!ok) {
                failures += 1;
                std.log.err("FAILED: {s}\n{s}{s}", .{ name, case_result.stdout, case_result.stderr });
            } else {
                std.log.info("passed: {s}", .{name});
            }
        }

        if (failures != 0) {
            return step.fail("{d}/{d} Catch2 cases failed", .{ failures, names.items.len });
        }

        std.log.info("All {d} Catch2 cases passed (isolated processes)", .{names.items.len});
    }
};

// ─── helpers ────────────────────────────────────────────────────────────

/// Create a module configured for C++ compilation.
/// On Windows/MSVC we use the MSVC C++ runtime (not libc++).
fn createCppModule(b: *std.Build, target: std.Build.ResolvedTarget, optimize: std.builtin.OptimizeMode) *std.Build.Module {
    const use_mingw_sdk = minGWRoot(target) != null;
    const use_libcxx = target.result.os.tag != .windows or target.result.abi != .msvc;
    // For MSVC we deliberately avoid Zig's bundled/auto libc (-lc pulls
    // libucrt) and link the matching MSVC CRT import libs ourselves.
    const mod = b.createModule(.{
        .target = target,
        .optimize = optimize,
        .link_libc = if (use_libcxx or use_mingw_sdk) true else false,
        .link_libcpp = if (use_libcxx) true else null,
        .sanitize_c = .off,
    });
    if (!use_libcxx) {
        addWindowsSdkPaths(mod, optimize, target);
        // Link the MSVC CRT directly (no Zig -lc). Debug uses MDd; release uses MD.
        // msvcrt(d) provides process/DLL startup (e.g. _DllMainCRTStartup).
        if (optimize == .Debug) {
            mod.linkSystemLibrary("msvcprtd", .{});
            mod.linkSystemLibrary("msvcrtd", .{});
            mod.linkSystemLibrary("ucrtd", .{});
            mod.linkSystemLibrary("vcruntimed", .{});
        } else {
            mod.linkSystemLibrary("msvcprt", .{});
            mod.linkSystemLibrary("msvcrt", .{});
            mod.linkSystemLibrary("ucrt", .{});
            mod.linkSystemLibrary("vcruntime", .{});
        }
        mod.linkSystemLibrary("oldnames", .{});
        mod.linkSystemLibrary("kernel32", .{});
        // Zig's ubsan/debug runtime needs ntdll when -lc is not used.
        mod.linkSystemLibrary("ntdll", .{});
    } else if (use_mingw_sdk) {
        addMinGWPaths(mod, target);
    }
    return mod;
}

fn minGWRoot(target: std.Build.ResolvedTarget) ?[]const u8 {
    if (builtin.os.tag != .linux or target.result.os.tag != .windows or target.result.abi != .gnu) return null;
    return switch (target.result.cpu.arch) {
        .x86_64 => "/usr/x86_64-w64-mingw32",
        .x86 => "/usr/i686-w64-mingw32",
        else => null,
    };
}

/// Zig's bundled Windows libc does not include Arch's MinGW-w64 SDK headers.
/// Add the external SDK when a GNU Windows target is built from Linux.
fn addMinGWPaths(mod: *std.Build.Module, target: std.Build.ResolvedTarget) void {
    const root = minGWRoot(target) orelse return;
    const b = mod.owner;
    const include_dir = b.fmt("{s}/include", .{root});
    const lib_dir = b.fmt("{s}/lib", .{root});
    if (dirExists(b, include_dir)) mod.addSystemIncludePath(.{ .cwd_relative = include_dir });
    if (dirExists(b, lib_dir)) mod.addLibraryPath(.{ .cwd_relative = lib_dir });
}

fn resourceIncludePaths(b: *std.Build, target: std.Build.ResolvedTarget, paths: []const std.Build.LazyPath) []const std.Build.LazyPath {
    const root = minGWRoot(target) orelse return paths;
    const include_dir = b.fmt("{s}/include", .{root});
    if (!dirExists(b, include_dir)) return paths;

    var result: std.ArrayListUnmanaged(std.Build.LazyPath) = .empty;
    result.appendSlice(b.allocator, paths) catch @panic("OOM");
    result.append(b.allocator, .{ .cwd_relative = include_dir }) catch @panic("OOM");
    return result.toOwnedSlice(b.allocator) catch @panic("OOM");
}

/// Add MSVC/WinSDK include + library paths from the VS developer environment
/// (INCLUDE/LIB) when available, with a filesystem fallback for the kit roots.
fn addWindowsSdkPaths(mod: *std.Build.Module, optimize: std.builtin.OptimizeMode, target: std.Build.ResolvedTarget) void {
    _ = optimize;
    const b = mod.owner;

    if (b.graph.environ_map.get("INCLUDE")) |inc| {
        var it = std.mem.splitScalar(u8, inc, ';');
        while (it.next()) |part| {
            if (part.len == 0) continue;
            mod.addSystemIncludePath(.{ .cwd_relative = b.dupePath(part) });
        }
    } else {
        addDefaultMsvcIncludes(mod);
        addDefaultWinKitIncludes(mod);
    }

    if (b.graph.environ_map.get("LIB")) |lib| {
        var it = std.mem.splitScalar(u8, lib, ';');
        while (it.next()) |part| {
            if (part.len == 0) continue;
            mod.addLibraryPath(.{ .cwd_relative = b.dupePath(part) });
        }
    } else {
        addDefaultMsvcLibs(mod, target);
        addDefaultWinKitLibs(mod, target);
    }
}

fn addDefaultMsvcIncludes(mod: *std.Build.Module) void {
    const b = mod.owner;
    const root = "C:/Program Files/Microsoft Visual Studio/18/Community/VC/Tools/MSVC";
    const ver = newestSubdirBefore(b, root, "14.52") orelse return;
    const include_dir = b.fmt("{s}/{s}/include", .{ root, ver });
    if (dirExists(b, include_dir)) mod.addSystemIncludePath(.{ .cwd_relative = include_dir });
}

fn addDefaultMsvcLibs(mod: *std.Build.Module, target: std.Build.ResolvedTarget) void {
    const b = mod.owner;
    const root = "C:/Program Files/Microsoft Visual Studio/18/Community/VC/Tools/MSVC";
    const ver = newestSubdirBefore(b, root, "14.52") orelse return;
    const arch = switch (target.result.cpu.arch) {
        .x86 => "x86",
        .x86_64 => "x64",
        .aarch64 => "arm64",
        else => return,
    };
    const lib_dir = b.fmt("{s}/{s}/lib/{s}", .{ root, ver, arch });
    if (dirExists(b, lib_dir)) mod.addLibraryPath(.{ .cwd_relative = lib_dir });
}

fn addDefaultWinKitIncludes(mod: *std.Build.Module) void {
    const b = mod.owner;
    const kits_root = "C:/Program Files (x86)/Windows Kits/10/Include";
    const ver = newestSubdir(b, kits_root) orelse return;
    for ([_][]const u8{ "ucrt", "um", "shared", "winrt", "cppwinrt" }) |leaf| {
        const p = b.fmt("{s}/{s}/{s}", .{ kits_root, ver, leaf });
        if (dirExists(b, p)) mod.addSystemIncludePath(.{ .cwd_relative = p });
    }
}

fn addDefaultWinKitLibs(mod: *std.Build.Module, target: std.Build.ResolvedTarget) void {
    const b = mod.owner;
    const kits_root = "C:/Program Files (x86)/Windows Kits/10/Lib";
    const ver = newestSubdir(b, kits_root) orelse return;
    const arch = switch (target.result.cpu.arch) {
        .x86 => "x86",
        .x86_64 => "x64",
        .aarch64 => "arm64",
        else => return,
    };
    for ([_][]const u8{ "ucrt", "um" }) |kind| {
        const p = b.fmt("{s}/{s}/{s}/{s}", .{ kits_root, ver, kind, arch });
        if (dirExists(b, p)) mod.addLibraryPath(.{ .cwd_relative = p });
    }
}

fn newestSubdir(b: *std.Build, root: []const u8) ?[]const u8 {
    return newestSubdirBefore(b, root, null);
}

/// Zig 0.16's Clang frontend is compatible with the 14.51 MSVC STL but not
/// with the 14.52 STL, which requires Clang 22 or newer.
fn newestSubdirBefore(b: *std.Build, root: []const u8, upper_bound: ?[]const u8) ?[]const u8 {
    const io = b.graph.io;
    var dir = std.Io.Dir.cwd().openDir(io, root, .{ .iterate = true }) catch return null;
    defer dir.close(io);
    var best: ?[]const u8 = null;
    var iter = dir.iterate();
    while (iter.next(io) catch null) |entry| {
        if (entry.kind != .directory) continue;
        if (upper_bound) |bound| {
            if (std.mem.order(u8, entry.name, bound) != .lt) continue;
        }
        if (best == null or std.mem.order(u8, entry.name, best.?) == .gt) {
            best = b.dupe(entry.name);
        }
    }
    return best;
}

fn resolveTarget(b: *std.Build, requested: std.Build.ResolvedTarget) std.Build.ResolvedTarget {
    var query = requested.query;

    // An ABI explicitly supplied through -Dtarget always wins. For target-neutral
    // Windows triples, use the native ABI on Windows and MinGW when cross-compiling.
    // This keeps the Zed tasks portable between Windows and Linux.
    if (requested.result.os.tag == .windows and query.abi == null) {
        query.os_tag = .windows;
        query.abi = if (builtin.os.tag == .windows) .msvc else .gnu;
        query.cpu_arch = requested.result.cpu.arch;
    }

    // Dynarec and several plugins require SSSE3. Zig defaults to `-mcpu baseline`,
    // which omits it, so enable the feature explicitly on x86 family targets.
    const arch = query.cpu_arch orelse requested.result.cpu.arch;
    if (arch == .x86 or arch == .x86_64) {
        query.cpu_features_add.addFeature(@intFromEnum(std.Target.x86.Feature.ssse3));
    }

    return b.resolveTargetQuery(query);
}

fn collectCxxFlags(b: *std.Build, optimize: std.builtin.OptimizeMode, is_x86_family: bool, target_uses_msvc: bool) []const []const u8 {
    var flags: std.ArrayListUnmanaged([]const u8) = .empty;
    flags.appendSlice(b.allocator, &.{
        "-std=c++23",
        // nlohmann-json's UTF-8 lookup table has 400 entries; Zig's Clang
        // misdiagnoses the uint8_t index range as a hard error.
        "-Wno-tautological-constant-out-of-range-compare",
    }) catch @panic("OOM");

    if (target_uses_msvc) {
        // Match vcpkg's MultiThreadedDLL CRT (MD / MDd).
        const crt = if (optimize == .Debug) "-fms-runtime-lib=dll_dbg" else "-fms-runtime-lib=dll";
        flags.appendSlice(b.allocator, &.{
            "-fms-extensions",
            "-fms-compatibility",
            "-fms-compatibility-version=19.40",
            crt,
            "-finput-charset=UTF-8",
            "-fexec-charset=UTF-8",
            // Treat unused-argument noise from Zig's C++ driver as non-fatal.
            "-Wno-unused-command-line-argument",
        }) catch @panic("OOM");
    }
    if (is_x86_family) {
        flags.append(b.allocator, "-mssse3") catch @panic("OOM");
    }
    switch (optimize) {
        .Debug => flags.append(b.allocator, "-g") catch @panic("OOM"),
        .ReleaseSafe, .ReleaseFast => flags.appendSlice(b.allocator, &.{ "-O2", "-g" }) catch @panic("OOM"),
        .ReleaseSmall => flags.append(b.allocator, "-Os") catch @panic("OOM"),
    }
    return flags.toOwnedSlice(b.allocator) catch @panic("OOM");
}

fn collectCFlags(b: *std.Build, optimize: std.builtin.OptimizeMode, is_x86_family: bool, target_uses_msvc: bool) []const []const u8 {
    var flags: std.ArrayListUnmanaged([]const u8) = .empty;
    flags.append(b.allocator, "-std=c11") catch @panic("OOM");
    if (target_uses_msvc) {
        const crt = if (optimize == .Debug) "-fms-runtime-lib=dll_dbg" else "-fms-runtime-lib=dll";
        flags.appendSlice(b.allocator, &.{ "-fms-extensions", crt, "-finput-charset=UTF-8", "-Wno-unused-command-line-argument" }) catch @panic("OOM");
    }
    if (is_x86_family) {
        flags.append(b.allocator, "-mssse3") catch @panic("OOM");
    }
    switch (optimize) {
        .Debug => flags.append(b.allocator, "-g") catch @panic("OOM"),
        .ReleaseSafe, .ReleaseFast => flags.appendSlice(b.allocator, &.{ "-O2", "-g" }) catch @panic("OOM"),
        .ReleaseSmall => flags.append(b.allocator, "-Os") catch @panic("OOM"),
    }
    return flags.toOwnedSlice(b.allocator) catch @panic("OOM");
}

fn withIncludePrefix(b: *std.Build, base: []const []const u8, header: []const u8) []const []const u8 {
    var flags: std.ArrayListUnmanaged([]const u8) = .empty;
    flags.appendSlice(b.allocator, base) catch @panic("OOM");
    // Force-include the former PCH header so translation units keep compiling.
    flags.append(b.allocator, b.fmt("-include{s}", .{header})) catch @panic("OOM");
    return flags.toOwnedSlice(b.allocator) catch @panic("OOM");
}

fn applyCommonDefines(
    mod: *std.Build.Module,
    optimize: std.builtin.OptimizeMode,
    target_is_windows: bool,
    version_suffix: []const u8,
    nightly: bool,
) void {
    const b = mod.owner;
    if (target_is_windows) {
        mod.addCMacro("UNICODE", "1");
        mod.addCMacro("_UNICODE", "1");
        // NOMINMAX is defined by project headers; don't redefine it here.
        // Avoid WIN32_LEAN_AND_MEAN so commdlg/shell APIs stay available.
    }
    if (optimize == .Debug) {
        mod.addCMacro("_DEBUG", "1");
    }
    // VERSION_SUFFIX is expanded as a wide string literal in sources.
    mod.addCMacro("VERSION_SUFFIX", b.fmt("L\"{s}\"", .{version_suffix}));
    if (nightly) {
        mod.addCMacro("NIGHTLY", "1");
    }
}

fn applyWindowsLinkFlags(compile: *std.Build.Step.Compile, target: std.Build.ResolvedTarget) void {
    if (target.result.os.tag != .windows) return;
    compile.linker_dynamicbase = false;
    // LARGEADDRESSAWARE is the historical default we want; lld enables it for
    // PE images when linking with zig on MSVC targets in practice for x64.
    // 32-bit historically also disabled NXCOMPAT; zig/lld has no first-class
    // toggle, so x86 builds may differ slightly from the old CMake flags.
}

fn addStaticLib(
    b: *std.Build,
    target: std.Build.ResolvedTarget,
    optimize: std.builtin.OptimizeMode,
    name: []const u8,
    sources: []const []const u8,
    flags: []const []const u8,
    includes: []const std.Build.LazyPath,
) *std.Build.Step.Compile {
    const is_cpp = for (sources) |s| {
        if (std.mem.endsWith(u8, s, ".cpp") or std.mem.endsWith(u8, s, ".cxx") or std.mem.endsWith(u8, s, ".cc"))
            break true;
    } else false;

    const lib = b.addLibrary(.{
        .name = name,
        .linkage = .static,
        .root_module = if (is_cpp)
            createCppModule(b, target, optimize)
        else blk: {
            // Pure C vendor libs: on MSVC, libc is provided by the final link unit.
            const use_libcxx_abi = target.result.os.tag != .windows or target.result.abi != .msvc;
            const mod = b.createModule(.{
                .target = target,
                .optimize = optimize,
                .link_libc = use_libcxx_abi,
                .sanitize_c = .off,
            });
            if (!use_libcxx_abi) addWindowsSdkPaths(mod, optimize, target);
            break :blk mod;
        },
    });
    lib.root_module.addCSourceFiles(.{
        .files = sources,
        .flags = flags,
        .language = if (is_cpp) .cpp else .c,
    });
    for (includes) |inc| lib.root_module.addIncludePath(inc);
    return lib;
}

const PluginCommon = struct {
    common_inc: std.Build.LazyPath,
    common_win32_inc: std.Build.LazyPath,
    core_inc: std.Build.LazyPath,
    views_headers_inc: std.Build.LazyPath,
    vendor_xxhash64: std.Build.LazyPath,
    vcpkg: VcpkgPaths,
    cxx_flags: []const []const u8,
    version_suffix: []const u8,
    nightly: bool,
    optimize: std.builtin.OptimizeMode,
    link_static: bool,
    enable_dynarec: bool,
};

fn addPlugin(
    b: *std.Build,
    target: std.Build.ResolvedTarget,
    common: PluginCommon,
    name: []const u8,
    sources: []const []const u8,
    rc_file: ?[]const u8,
    extra_includes: []const std.Build.LazyPath,
) *std.Build.Step.Compile {
    const lib = b.addLibrary(.{
        .name = name,
        .linkage = .dynamic,
        .root_module = createCppModule(b, target, common.optimize),
    });
    lib.root_module.pic = true;
    lib.linker_dynamicbase = false;
    lib.root_module.addCSourceFiles(.{
        .files = sources,
        .flags = withIncludePrefix(b, common.cxx_flags, "src/Common/include/CommonPCH.hpp"),
        .language = .cpp,
    });
    if (rc_file) |rc| {
        lib.root_module.addWin32ResourceFile(.{
            .file = b.path(rc),
            .include_paths = resourceIncludePaths(b, target, extra_includes),
        });
    }
    lib.root_module.addIncludePath(common.common_inc);
    lib.root_module.addIncludePath(common.common_win32_inc);
    lib.root_module.addIncludePath(common.core_inc);
    lib.root_module.addIncludePath(common.views_headers_inc);
    lib.root_module.addIncludePath(common.vendor_xxhash64);
    for (extra_includes) |inc| lib.root_module.addIncludePath(inc);
    applyVcpkgIncludes(lib.root_module, common.vcpkg);
    applyVcpkgLibPaths(lib.root_module, common.vcpkg);
    applyCommonDefines(lib.root_module, common.optimize, true, common.version_suffix, common.nightly);
    lib.root_module.addCMacro("PLUGIN_WITH_CALLBACKS", "1");
    if (common.enable_dynarec) lib.root_module.addCMacro("MUPEN64RR_ENABLE_DYNAREC", "1");
    linkLibdeflate(lib.root_module, common.vcpkg);
    // Baseline Win32 libs used across plugins (dialogs, windows, shell).
    for ([_][]const u8{ "user32", "gdi32", "comdlg32", "shell32", "advapi32", "ole32", "uuid", "comctl32" }) |wlib| {
        lib.root_module.linkSystemLibrary(wlib, .{});
    }
    applyWindowsLinkFlags(lib, target);
    return lib;
}

fn linkViewsWinLibs(mod: *std.Build.Module) void {
    for ([_][]const u8{
        "shlwapi",  "vfw32",   "winmm",  "propsys", "comctl32",    "uxtheme",
        "msimg32",  "gdiplus", "d2d1",   "dwrite",  "dbghelp",     "dcomp",
        "d3d11",    "dxgi",    "dxguid", "dwmapi",  "d3dcompiler", "winhttp",
        "ole32",    "uuid",    "gdi32",  "user32",  "shell32",     "advapi32",
        "kernel32",
    }) |lib| {
        mod.linkSystemLibrary(lib, .{});
    }
}

// ─── vcpkg ──────────────────────────────────────────────────────────────

const VcpkgPaths = struct {
    root: []const u8,
    include_dir: []const u8,
    lib_dir: []const u8,
    lib_dir_manual: []const u8,
    bin_dir: []const u8,
};

fn vcpkgTriplet(target: std.Build.ResolvedTarget, link_static: bool) []const u8 {
    const arch = switch (target.result.cpu.arch) {
        .x86_64 => "x64",
        .x86 => "x86",
        .aarch64 => "arm64",
        else => "x64",
    };
    if (target.result.abi == .msvc) {
        if (link_static) {
            return std.fmt.allocPrint(std.heap.page_allocator, "{s}-windows-static", .{arch}) catch @panic("OOM");
        }
        return std.fmt.allocPrint(std.heap.page_allocator, "{s}-windows", .{arch}) catch @panic("OOM");
    }
    // Zig compiles C++ with its bundled libc++. The stock MinGW triplets use
    // GCC/libstdc++, which makes C++ vcpkg libraries ABI-incompatible.
    if (builtin.os.tag == .linux and target.result.os.tag == .windows and target.result.abi == .gnu) {
        const linkage = if (link_static) "static" else "dynamic";
        return std.fmt.allocPrint(std.heap.page_allocator, "{s}-zig-windows-{s}", .{ arch, linkage }) catch @panic("OOM");
    }
    if (link_static) {
        return std.fmt.allocPrint(std.heap.page_allocator, "{s}-mingw-static", .{arch}) catch @panic("OOM");
    }
    return std.fmt.allocPrint(std.heap.page_allocator, "{s}-mingw-dynamic", .{arch}) catch @panic("OOM");
}

fn resolveVcpkg(
    b: *std.Build,
    target: std.Build.ResolvedTarget,
    link_static: bool,
    optimize: std.builtin.OptimizeMode,
    explicit: ?[]const u8,
) ?VcpkgPaths {
    const triplet = vcpkgTriplet(target, link_static);
    const candidates = [_][]const u8{
        if (explicit) |e| e else "",
        b.fmt("vcpkg_installed/{s}", .{triplet}),
        b.fmt("build/vcpkg_installed/{s}", .{triplet}),
    };

    const root = blk: {
        for (candidates) |c| {
            if (c.len == 0) continue;
            var dir = std.Io.Dir.cwd().openDir(b.graph.io, c, .{}) catch continue;
            dir.close(b.graph.io);
            break :blk b.dupePath(c);
        }
        if (explicit != null) {
            std.log.err("vcpkg installed directory not found (looked for explicit -Dvcpkg_installed)", .{});
            return null;
        }
        std.log.warn("vcpkg dependencies not found for triplet '{s}'. Run `zig build vcpkg` or pass -Dvcpkg_installed=", .{triplet});
        return null;
    };

    const debug = optimize == .Debug;
    const lib_dir = if (debug)
        b.fmt("{s}/debug/lib", .{root})
    else
        b.fmt("{s}/lib", .{root});
    const lib_dir_manual = b.fmt("{s}/manual-link", .{lib_dir});
    const bin_dir = if (debug)
        b.fmt("{s}/debug/bin", .{root})
    else
        b.fmt("{s}/bin", .{root});

    // Fall back to release lib/bin if debug tree is incomplete.
    const final_lib = if (dirExists(b, lib_dir)) lib_dir else b.fmt("{s}/lib", .{root});
    const final_bin = if (dirExists(b, bin_dir)) bin_dir else b.fmt("{s}/bin", .{root});
    const final_manual = if (dirExists(b, lib_dir_manual)) lib_dir_manual else b.fmt("{s}/lib/manual-link", .{root});

    return .{
        .root = root,
        .include_dir = b.fmt("{s}/include", .{root}),
        .lib_dir = final_lib,
        .lib_dir_manual = final_manual,
        .bin_dir = final_bin,
    };
}

fn dirExists(b: *std.Build, path: []const u8) bool {
    var dir = std.Io.Dir.cwd().openDir(b.graph.io, path, .{}) catch return false;
    dir.close(b.graph.io);
    return true;
}

fn applyVcpkgIncludes(mod: *std.Build.Module, vp: VcpkgPaths) void {
    mod.addIncludePath(.{ .cwd_relative = vp.include_dir });
}

fn applyVcpkgLibPaths(mod: *std.Build.Module, vp: VcpkgPaths) void {
    mod.addLibraryPath(.{ .cwd_relative = vp.lib_dir });
    if (dirExists(mod.owner, vp.lib_dir_manual)) {
        mod.addLibraryPath(.{ .cwd_relative = vp.lib_dir_manual });
    }
    // Always also search the release lib dir for packages without debug builds.
    const release_lib = std.fmt.allocPrint(mod.owner.allocator, "{s}/lib", .{vp.root}) catch return;
    if (!std.mem.eql(u8, release_lib, vp.lib_dir)) {
        mod.addLibraryPath(.{ .cwd_relative = release_lib });
        const release_manual = std.fmt.allocPrint(mod.owner.allocator, "{s}/manual-link", .{release_lib}) catch return;
        if (dirExists(mod.owner, release_manual)) {
            mod.addLibraryPath(.{ .cwd_relative = release_manual });
        }
    }
}

fn debugSuffix(optimize: std.builtin.OptimizeMode) []const u8 {
    return if (optimize == .Debug) "d" else "";
}

/// Link a vcpkg dependency, including MinGW's `lib<name>.dll.a` import-library
/// convention that Zig's system-library search does not currently probe.
fn linkVcpkgLibrary(mod: *std.Build.Module, vp: VcpkgPaths, name: []const u8) void {
    const b = mod.owner;
    const import_lib = b.fmt("{s}/lib{s}.dll.a", .{ vp.lib_dir, name });
    if (std.Io.Dir.cwd().access(b.graph.io, import_lib, .{})) |_| {
        mod.addObjectFile(.{ .cwd_relative = import_lib });
        return;
    } else |_| {}
    mod.linkSystemLibrary(name, .{});
}

fn linkLibdeflate(mod: *std.Build.Module, vp: ?VcpkgPaths) void {
    if (vp) |paths| {
        linkVcpkgLibrary(mod, paths, "deflate");
    } else {
        mod.linkSystemLibrary("deflate", .{});
    }
}

fn linkLua(mod: *std.Build.Module, vp: VcpkgPaths) void {
    linkVcpkgLibrary(mod, vp, "lua");
}

fn linkSpdlog(mod: *std.Build.Module, vp: VcpkgPaths, optimize: std.builtin.OptimizeMode) void {
    const d = debugSuffix(optimize);
    const b = mod.owner;
    linkVcpkgLibrary(mod, vp, b.fmt("spdlog{s}", .{d}));
    linkVcpkgLibrary(mod, vp, b.fmt("fmt{s}", .{d}));
}

fn applySpdlogDefines(mod: *std.Build.Module, link_static: bool) void {
    if (!link_static) {
        mod.addCMacro("SPDLOG_SHARED_LIB", "1");
    }
    mod.addCMacro("SPDLOG_COMPILED_LIB", "1");
    mod.addCMacro("SPDLOG_FMT_EXTERNAL", "1");
    mod.addCMacro("SPDLOG_WCHAR_TO_UTF8_SUPPORT", "1");
}

fn linkSdl3(mod: *std.Build.Module, vp: VcpkgPaths) void {
    linkVcpkgLibrary(mod, vp, "SDL3");
}

fn linkNlohmannJson(mod: *std.Build.Module, vp: VcpkgPaths) void {
    // Header-only; include path from vcpkg is enough.
    _ = mod;
    _ = vp;
}

fn linkSpeexdsp(mod: *std.Build.Module, vp: VcpkgPaths) void {
    linkVcpkgLibrary(mod, vp, "speexdsp");
}

fn linkGlew(mod: *std.Build.Module, vp: VcpkgPaths, optimize: std.builtin.OptimizeMode, link_static: bool) void {
    const b = mod.owner;
    if (link_static) {
        mod.addCMacro("GLEW_STATIC", "1");
    }
    const d = debugSuffix(optimize);
    linkVcpkgLibrary(mod, vp, b.fmt("glew32{s}", .{d}));
}

fn linkCatch2(mod: *std.Build.Module, vp: VcpkgPaths, optimize: std.builtin.OptimizeMode) void {
    _ = optimize;
    // Zig's Debug mode injects UBSan into vcpkg's Catch2d archive, but this
    // project links its test executable without the sanitizer runtime. Use
    // the non-instrumented release Catch2 archive for both configurations.
    linkVcpkgLibrary(mod, vp, "Catch2");
    linkVcpkgLibrary(mod, vp, "Catch2Main");
}

// ─── source lists ───────────────────────────────────────────────────────

const core_sources_common = [_][]const u8{
    "src/Core/Core.cpp",
    "src/Core/Alloc.cpp",
    "src/Core/Cheats.cpp",
    "src/Core/Memory/PifLut.cpp",
    "src/Core/Memory/DMA.cpp",
    "src/Core/Memory/FlashRAM.cpp",
    "src/Core/Memory/Memory.cpp",
    "src/Core/Memory/Pif.cpp",
    "src/Core/Memory/Savestates.cpp",
    "src/Core/Memory/ParityChecker.cpp",
    "src/Core/Memory/Summercart.cpp",
    "src/Core/Memory/TLB.cpp",
    "src/Core/R4300/Debug.cpp",
    "src/Core/R4300/PureInterp.cpp",
    "src/Core/R4300/Cop0.cpp",
    "src/Core/R4300/Cop1.cpp",
    "src/Core/R4300/Cop1D.cpp",
    "src/Core/R4300/Cop1Helpers.cpp",
    "src/Core/R4300/Cop1L.cpp",
    "src/Core/R4300/Cop1S.cpp",
    "src/Core/R4300/Cop1W.cpp",
    "src/Core/R4300/Disasm.cpp",
    "src/Core/R4300/Exception.cpp",
    "src/Core/R4300/Interrupt.cpp",
    "src/Core/R4300/R4300.cpp",
    "src/Core/R4300/Recomp.cpp",
    "src/Core/R4300/RegImm.cpp",
    "src/Core/R4300/Rom.cpp",
    "src/Core/R4300/Special.cpp",
    "src/Core/R4300/Timers.cpp",
    "src/Core/R4300/Tracelog.cpp",
    "src/Core/R4300/BC.cpp",
    "src/Core/R4300/VCR.cpp",
};

const dynarec_sources = [_][]const u8{
    "Assemble.cpp",
    "GBc.cpp",
    "GCop0.cpp",
    "GCop1.cpp",
    "GCop1D.cpp",
    "GCop1Helpers.cpp",
    "GCop1L.cpp",
    "GCop1S.cpp",
    "GCop1W.cpp",
    "GR4300.cpp",
    "GRegImm.cpp",
    "GSpecial.cpp",
    "GTLB.cpp",
    "RegCache.cpp",
    "RJump.cpp",
};

const views_sources = [_][]const u8{
    "src/Views.Win32/ThreadPool.cpp",
    "src/Views.Win32/capture/encoders/VFWEncoder.cpp",
    "src/Views.Win32/capture/encoders/FFmpegEncoder.cpp",
    "src/Views.Win32/capture/CaptureManager.cpp",
    "src/Views.Win32/capture/Resampler.cpp",
    "src/Views.Win32/Config.cpp",
    "src/Views.Win32/Hotkey.cpp",
    "src/Views.Win32/DialogService.cpp",
    "src/Views.Win32/components/CoreUtils.cpp",
    "src/Views.Win32/components/Validators.cpp",
    "src/Views.Win32/components/Cheats.cpp",
    "src/Views.Win32/components/ConfigDialog.cpp",
    "src/Views.Win32/components/CrashManager.cpp",
    "src/Views.Win32/components/Dispatcher.cpp",
    "src/Views.Win32/components/MGECompositor.cpp",
    "src/Views.Win32/components/MovieDialog.cpp",
    "src/Views.Win32/components/PianoRoll.cpp",
    "src/Views.Win32/components/RecentItems.cpp",
    "src/Views.Win32/components/RomBrowser.cpp",
    "src/Views.Win32/components/Seeker.cpp",
    "src/Views.Win32/components/Statusbar.cpp",
    "src/Views.Win32/components/UpdateChecker.cpp",
    "src/Views.Win32/components/FilePicker.cpp",
    "src/Views.Win32/components/CLI.cpp",
    "src/Views.Win32/components/SettingsListView.cpp",
    "src/Views.Win32/components/HotkeyTracker.cpp",
    "src/Views.Win32/components/CommandPalette.cpp",
    "src/Views.Win32/components/ParameterPalette.cpp",
    "src/Views.Win32/components/TextEditDialog.cpp",
    "src/Views.Win32/components/ReorderableListView.cpp",
    "src/Views.Win32/Loggers.cpp",
    "src/Views.Win32/action/ActionManager.cpp",
    "src/Views.Win32/action/ActionMenu.cpp",
    "src/Views.Win32/action/AppActions.cpp",
    "src/Views.Win32/Main.cpp",
    "src/Views.Win32/lua/LuaHelpers.cpp",
    "src/Views.Win32/lua/LuaManager.cpp",
    "src/Views.Win32/lua/LuaCallbacks.cpp",
    "src/Views.Win32/lua/LuaRegistry.cpp",
    "src/Views.Win32/lua/LuaRenderer.cpp",
    "src/Views.Win32/lua/LuaDialog.cpp",
    "src/Views.Win32/lua/presenters/DCompPresenter.cpp",
    "src/Views.Win32/lua/presenters/GDIPresenter.cpp",
    "src/Views.Win32/Messenger.cpp",
    "src/Views.Win32/plugin/Plugin.cpp",
    "src/Views.Win32/plugin/ZEPlugin.cpp",
    "src/Views.Win32/plugin/M64RRPlugin.cpp",
    "src/Views.Win32/ResizeAnchor.cpp",
};
