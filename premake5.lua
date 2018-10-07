-- https://github.com/premake/premake-core/wiki

-- require "./premake5/premake-androidmk/androidmk"
-- http://gulu-dev.com/post/2016-04-17-premake-android-mk

local action = _ACTION or ""

solution "vgfx"
    configurations { "Debug", "Release" }
    language "C++"

    location (action)
    platforms {"x64"}

    configuration "Debug"
        optimize "Debug"
        defines { "DEBUG" }
        symbols "On"
        targetsuffix "-d"

    configuration "Release"
        defines { "NDEBUG" }
        optimize "On"

    configuration "vs*"
        defines {
            -- "FALCOR_D3D12",
            "_SCL_SECURE_NO_WARNINGS",
            "_CRT_SECURE_NO_WARNINGS",
            "WIN32",
        }
        systemversion "10.0.17134.0"

        os.mkdir("bin")

        configuration "x64"
            targetdir ("bin")

    flags {
        "MultiProcessorCompile"
    }

    includedirs { 
        path.join("$(VULKAN_SDK)", "include"),
    }

    project "glfw"
        kind "StaticLib"
        
        files {
            "third_party/glfw/include/**",
            "third_party/glfw/src/win32_*",
            "third_party/glfw/src/init*",
            "third_party/glfw/src/egl*",
            "third_party/glfw/src/wgl*",
            "third_party/glfw/src/init.c",
            "third_party/glfw/src/input.c",
            "third_party/glfw/src/context.c",
            "third_party/glfw/src/monitor.c",
            "third_party/glfw/src/osmesa_context.c",
            "third_party/glfw/src/vulkan.c",
            "third_party/glfw/src/window.c",
        }

        defines {
            "_GLFW_WIN32",
        }

        libdirs {
        
        }

        removefiles {
            -- "../Falcor/Framework/Source/Raytracing/**",
        }

    
    project "vgfx"
        kind "StaticLib"

        files {
            "tinyvk.*",
        }

        libdirs {
            
        }

        removefiles {
            -- "../Falcor/Framework/Source/Raytracing/**",
        }

    function create_example_project( example_path )
        leaf_name = string.sub(example_path, string.len("samples/src/") + 1, -5);

        project (leaf_name)
            kind "ConsoleApp"
            files { 
                example_path,
            }

            includedirs {
                ".",
                path.join("$(VULKAN_SDK)", "include"),
                "third_party/glfw/include",
                "third_party/stb",
                "third_party/glm",
            }
            
            libdirs {
                "bin",
                path.join("$(VULKAN_SDK)", "lib"),
            }

            defines {
                "TINY_RENDERER_VK",
                "GLM_FORCE_RADIANS",
                "GLM_FORCE_DEPTH_ZERO_TO_ONE",
                "GLM_ENABLE_EXPERIMENTAL",
            }

            debugdir "bin" 

            configuration "Debug"
                links {
                    "vgfx-d",
                    "glfw-d",
                    "vulkan-1",
                }

            configuration "Release"
                links {
                    "vgfx",
                    "vulkan-1",
                    "glfw",
                }

    end

    local examples = os.matchfiles("samples/src/*")
    for _, example in ipairs(examples) do
        create_example_project(example)
    end
