#ifndef __SELF_CHECK_H__
#define __SELF_CHECK_H__

#include <glib.h>

#define CHECK_RESULT(type, expression, expected_value) \
G_STMT_START { \
	before_check (#expression, __FILE__, __LINE__); \
	check_##type##_result (expression, expected_value); \
} G_STMT_END

#define CHECK_INTEGER_RESULT(expression, expected_value) \
	CHECK_RESULT(integer, expression, expected_value)
#define CHECK_STRING_RESULT(expression, expected_value) \
	CHECK_RESULT(string, expression, expected_value)
#define CHECK_BOOLEAN_RESULT(expression, expected_value) \
	CHECK_RESULT(integer, expression, expected_value)
#define CHECK_POINTER_RESULT(expression, expected_value) \
	CHECK_RESULT(pointer, expression, expected_value)

void before_check               (const char    *expression,
				 const char    *file_name,
				 int            line_number);
void after_check                (void);
void report_check_failure       (char          *result,
				 char          *expected);
void check_integer_result       (long           result,
				 long           expected);
void check_string_result        (char          *result,
				 const char    *expected);
void check_pointer_result       (gconstpointer  result,
				 gconstpointer  expected);

#endif /* __SELF_CHECK_H__ */
