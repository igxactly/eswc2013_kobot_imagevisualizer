#!/bin/sh
if [ $# -eq 2 ]; then
	sed -r '/[A-za-z]/d' $1 \
	| sed -r 's/(320,.+|[0-9]+,240.+)\}//g' \
	| sed -r 's/[0-9]+,[0-9]+ \{//g' \
	| sed -r 's/\}/,/g' \
	| tr -d '[:space:]' > $2
else 
	echo "usage: $0 <infile> <outfile>"
fi

exit 0;
