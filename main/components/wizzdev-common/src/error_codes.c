#include "error_codes.h"


const char* getErrorDescription(EErrorCode errorCode)
{
    switch (errorCode)
    {
        #define ENTRY(name, description) case name: return description;
            ERROR_CODES_TABLE
        #undef ENTRY

        default:
        {
            return "Unknown error";
        }
    }
}
