#!/bin/bash
#
# This file is part of DynExp.
#
# This script is intended to facilitate the generation of header and source files for DynExp
# hardware adapters, meta instruments, instruments and modules from their respective templates.
# It makes use of https://github.com/jbopp/code-template.git
#
# Save variables as json file as <ObjectName>.json within this folder. Call this script like
# ./run-code-template.sh <TemplateObjectName> <ObjectName>
# move generated files <ObjectName>.cpp and <ObjectName>.h to respective folders and add them
# to the DynExp project.
#
# Templates for JSON variable files can be generated (written to <ObjectName>.json) from the
# respective templates themselves invoking
# ./run-code-template.sh --extract-vars-json <TemplateObjectName> <ObjectName>
#

UsageStr="Usage: run-code-template.sh [--extract-vars-json] <TemplateNameWithoutExt> <DestinyNameWithoutExt>"

if [[ -z $1 ]]; then
	echo "No arguments supplied."
	echo "$UsageStr"

	exit 2
fi

if [[ "$1" == "--extract-vars-json" ]]; then
	if [[ $# -ne 3 ]]; then
		echo "Illegal number of arguments."
		echo "$UsageStr"

		exit 2
	fi

	python3 code-template/codetempl.py --extract-vars-json $2.htpl $2.cpptpl > $3.json
else
	if [[ $# -ne 2 ]]; then
		echo "Illegal number of arguments."
		echo "$UsageStr"

		exit 2
	fi

	if [[ "$1" == "Module" ]]; then
		python3 code-template/codetempl.py --force --search-dir . --map-ext cpp:$1.cpptpl --map-ext h:$1.htpl --map-ext ui:$1.uitpl --vars-json ./$2.json ./$2.cpp ./$2.h ./$2.ui
	else
		python3 code-template/codetempl.py --force --search-dir . --map-ext cpp:$1.cpptpl --map-ext h:$1.htpl --vars-json ./$2.json ./$2.cpp ./$2.h
	fi

	# Replace line ending (LF to CRLF)
	awk -v f="$2" '{printf "%s\r\n", $0 > f"_tmp.cpp"}' $2.cpp
	awk -v f="$2" '{printf "%s\r\n", $0 > f"_tmp.h"}' $2.h
	rm $2.cpp $2.h
	mv $2_tmp.cpp $2.cpp
	mv $2_tmp.h $2.h

	if [[ "$1" == "Module" ]]; then
		awk -v f="$2" '{printf "%s\r\n", $0 > f"_tmp.ui"}' $2.ui
		rm $2.ui
		mv $2_tmp.ui $2.ui
	fi
fi
