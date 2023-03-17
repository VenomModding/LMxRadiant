1.) Install C++ Build tools (msbuild)
2.) Add msbuild to the "PATH" environment variable (eg. "%ProgramFiles(x86)%\Microsoft Visual Studio\2022\Community\Msbuild\Current\Bin")
3.) Add "BO1_ROOT" environment variable with path to your bo1 directory (eg. "BO1_ROOT" "D:\BO1-Modtools")
4.) Drag and drop all files from ".vscode/tracked\" into ".vscode\ (we ignore all files in .vscode/ besides the readme)
5.) Open "t5xo-radiant.code-workspace" 
6.) Run task "update_submodules" or open "update_submodules.bat"
7.) Run task "generate-buildfiles" or open "generate-buildfiles.bat"

Use provided build-tasks to build debug/release builds with the option to copy t5xr.dll and t5xr.pdb to %BO1_ROOT%/bin.
Run->Start Debugging will build and copy a debug build to to %BO1_ROOT%/bin and launch T5xRadiant.