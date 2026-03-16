#
# Reads files like AUTHORS.txt or CONTRIBUTORS.txt and extracts
# a comma-separated list of all person names that is returned as 
# PERSONS_LIST.
#
# read_person_list (<SRC_FILE>)
#
# SRC_FILE - file to read names from
# OUT_LIST - comma-separated list of person names
#
function(READ_PERSON_LIST SRC_FILE OUT_LIST)
	file(STRINGS "${SRC_FILE}" PERSON_LINES)

	set(PERSON_NAMES)
	foreach (PERSON_LINE IN LISTS PERSON_LINES)
		# Skip empty lines or comments
		if (PERSON_LINE MATCHES "^\\s*$" OR PERSON_LINE MATCHES "^#")
			continue()
		endif()

		# Extract name before the email
		if (PERSON_LINE MATCHES "^([^<]+)<")
			string(STRIP "${CMAKE_MATCH_1}" PERSON_NAME)
			list(APPEND PERSON_NAMES "${PERSON_NAME}")
		endif()
	endforeach()
	list(JOIN PERSON_NAMES ", " PERSONS_LIST)

	set(${OUT_LIST} "${PERSONS_LIST}" PARENT_SCOPE)
endfunction()