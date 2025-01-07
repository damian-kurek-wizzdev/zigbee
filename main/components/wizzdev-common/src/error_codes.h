#ifndef ERROR_CODES_H
#define ERROR_CODES_H


/// Macro with all error code enums and descriptions, gathered for convenience
#define ERROR_CODES_TABLE \
    ENTRY(ERROR_CODE__NO_ERROR, "No error occurred") \
    ENTRY(ERROR_CODE__UNKNWON_ERROR, "Unknown error")


/**
 * @brief Used to return operation status from almost all API functions
 *
 * See ERROR_CODES_TABLE with listing of all errors with description for more informations.
 * To get error description use getErrorDescription(errorCode) function.
 * Macros inside enum class iterate through ERROR_CODES_TABLE table.
 */
typedef enum
{
#define ENTRY(name, description) name,
    ERROR_CODES_TABLE
#undef ENTRY
    SIZE
} EErrorCode;


/**
 * @brief getErrorDescription
 * @param errorCode Code of error for which the description will be returned
 * @return Pointer to null terminated description
 */
const char* getErrorDescription(EErrorCode errorCode);

#endif // ERROR_CODES_H
