dependencies = {
	basePath = "./deps"
}

function dependencies.load()
	dir = path.join(dependencies.basePath, "premake/*.lua")
	deps = os.matchfiles(dir)

	for i, dep in pairs(deps) do
		dep = dep:gsub(".lua", "")
		require(dep)
	end
end

function dependencies.imports()
	for i, proj in pairs(dependencies) do
		if type(i) == 'number' then
			proj.import()
		end
	end
end

function dependencies.projects()
	for i, proj in pairs(dependencies) do
		if type(i) == 'number' then
			proj.project()
		end
	end
end

newaction {
	trigger = "generate-buildinfo",
	description = "Sets up build information file like version.h.",
	onWorkspace = function(wks)

		local oldRevNumber = "(none)"
		print ("Reading :: " .. path.getdirectory(wks.location) .. "/src/version.hpp")
		local oldVersionHeader = io.open(path.getdirectory(wks.location) .. "/src/version.hpp", "r")
		if oldVersionHeader ~=nil then
			local oldVersionHeaderContent = assert(oldVersionHeader:read('*a'))
			oldRevNumber = string.match(oldVersionHeaderContent, "#define REVISION (%d+)")
			if oldRevNumber == nil then
				oldRevNumber = 0
			end
		end

		-- generate version.hpp with a revision number if not equal
		local revNumber = oldRevNumber + 1
			print ("Update " .. oldRevNumber .. " -> " .. revNumber)
			local versionHeader = assert(io.open(path.getdirectory(wks.location) .. "/src/version.hpp", "w"))
			versionHeader:write("/* Automatically generated by premake5. */\n")
			versionHeader:write("\n")
			versionHeader:write("#define REVISION " .. revNumber .. "\n")
			versionHeader:close()
	end
}

dependencies.load()

workspace "iw3xo-radiant"
	location "./build"
	objdir "%{wks.location}/obj"
	targetdir "%{wks.location}/bin/%{cfg.buildcfg}"
	
    configurations { 
        "Debug", 
        "Release" 
    }

	platforms "Win32"
	architecture "x86"

    buildoptions "/std:c++latest"
	systemversion "latest"
    symbols "On"
    staticruntime "On"

    disablewarnings {
		"4239",
		"4505",
		"4996",
	}

    defines { 
        "_SILENCE_ALL_CXX17_DEPRECATION_WARNINGS" 
    }

    configuration "windows"
		defines { 
            "_WINDOWS", 
            "WIN32" 
        }
        
	configuration "Release"
        optimize "Full"
		
        defines { 
            "NDEBUG" 
        }

		flags { 
            "MultiProcessorCompile", 
            "LinkTimeOptimization", 
            "No64BitChecks" 
        }
		
	configuration "Debug"
        optimize "Debug"
		defines { 
            "DEBUG", 
            "_DEBUG" 
        }

		flags { 
            "MultiProcessorCompile", 
            "No64BitChecks" 
        }
		
    configuration {}

	startproject "iw3r"
	project "iw3r"
		kind "SharedLib"
		language "C++"

		pchheader "STDInclude.hpp"
		pchsource "src/STDInclude.cpp"

		files {
			"./src/**.rc",
			"./src/**.hpp",
			"./src/**.cpp",
		}

		includedirs {
			"%{prj.location}/src",
			"./src",
		}

		resincludedirs {
			"$(ProjectDir)src"
		}

        buildoptions { 
            "/Zm100 -Zm100" 
        }

        -- Virtual paths
		--[[ if not _OPTIONS["no-new-structure"] then
			vpaths {
				["Headers/*"] = { "./src/**.hpp" },
				["Sources/*"] = { "./src/**.cpp" },
				["Resource/*"] = { "./src/**.rc" },
			}
		end

		vpaths {
			["Docs/*"] = { "**.txt","**.md" },
		} ]]

        -- Specific configurations
		flags { "UndefinedIdentifiers" }
		warnings "Extra"

        
		configuration "Release"
			flags { 
                "FatalCompileWarnings" 
            }
			
			-- Pre-build
			prebuildcommands {
			"cd %{_MAIN_SCRIPT_DIR}",
			"tools\\premake5 generate-buildinfo"
			}
            
		configuration {}

        dependencies.imports()

        group "Dependencies"
            dependencies.projects()