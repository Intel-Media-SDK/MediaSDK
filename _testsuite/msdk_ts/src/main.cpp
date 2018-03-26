#include "stdafx.h"
#include "msdk_ts.h"
#include <exception>
#include <fstream>

int main(int argc, char * argv[]){
    msdk_ts::test_result res = msdk_ts::resFAIL;

    allow_debug_output();
    assert(strlen(msdk_ts_INDENT) == msdk_ts_INDENT_SIZE);
    try{
        if (argc == 3 && strcmp("-f", argv[1]) == 0)
        {
            std::ifstream file(argv[2]);
            msdk_ts::ts test(file);
            res = test.run();
        }
        else
        {
            msdk_ts::ts test(std::cin);
            res = test.run();
        }
    }catch(std::exception e){
        std::cout << "FAILED: Exception: "<< e.what() << "\n";
        return msdk_ts::resEXCPT;
    }catch(...){
        std::cout << "FAILED: unknown exception\n";
        return msdk_ts::resEXCPT;
    }

    return res;
}