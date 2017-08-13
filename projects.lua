BOOK_CODE_PATH = "E:/book-code"
THIRD_PARTY = "E:/book-code/3rdparty"
WORK_PATH = os.getcwd()
includeexternal (BOOK_CODE_PATH .. "/premake-vs-include.lua")




workspace(path.getname(os.realpath(".")))
    language "C++"
    location "build/%{_ACTION}/%{wks.name}"    
    if _ACTION == "vs2015" then
        toolset "v140_xp"
    elseif _ACTION == "vs2013" then
        toolset "v120_xp"
    end

    include (BOOK_CODE_PATH .. "/common.lua")   
    

    group "test"       
        


        create_console_project("NamedPipeClient", "src")
            includedirs
            {
                "%{THIRD_PARTY}/doctest",                
                "%{THIRD_PARTY}",
            }
            links
            {
                --"gtest",
            }
            
        

        create_console_project("NamedPipeServer", "src")
            includedirs
            {
                "%{THIRD_PARTY}/doctest",                
                "%{THIRD_PARTY}",
            }
            links
            {
                --"gtest",
            }
        
        create_console_project("CppNamedPipeClient", "src")
            includedirs
            {
                "%{THIRD_PARTY}/doctest",                
                "%{THIRD_PARTY}",
            }
            links
            {
                --"gtest",
            }
      
        create_console_project("CppNamedPipeServer", "src")
            includedirs
            {
                "%{THIRD_PARTY}/doctest",                
                "%{THIRD_PARTY}",
            }
            links
            {
                --"gtest",
            }    

        create_mfc_project("namedpipes", "src")
            characterset "MBCS"
            includedirs
            {
                "%{THIRD_PARTY}/doctest",                
                "%{THIRD_PARTY}",
            }
            links
            {
                --"gtest",
            }        

        create_mfc_project("namedPipe", "src")
            characterset "MBCS"
            includedirs
            {
                "%{THIRD_PARTY}/doctest",                
                "%{THIRD_PARTY}",
            }
            links
            {
                --"gtest",
            }        

        create_console_project("IoCompletionPort-NamedPipe-Server", "src")
            includedirs
            {
                "%{THIRD_PARTY}/doctest",                
                "%{THIRD_PARTY}",
            }
            links
            {
                --"gtest",
            }
            
        create_console_project("IoCompletionPort-NamedPipe-Client", "src")
            includedirs
            {
                "%{THIRD_PARTY}/doctest",                
                "%{THIRD_PARTY}",
            }
            links
            {
                --"gtest",
            }
        
        create_console_project("simple_pipe_client", "src")
            includedirs
            {
                "%{THIRD_PARTY}/doctest",                
                "%{THIRD_PARTY}",
            }
            links
            {
                --"gtest",
            }

        create_console_project("OverlappedServer", "src")
            includedirs
            {
                "%{THIRD_PARTY}/doctest",                
                "%{THIRD_PARTY}",
            }
            links
            {
                --"gtest",
            }

        create_console_project("simple_pipe_server", "src")
            includedirs
            {
                "%{THIRD_PARTY}/doctest",                
                "%{THIRD_PARTY}",
            }
            links
            {
                --"gtest",
            }

        
        