#!/bin/bash

filen=$1
target_file=$2
union_name=db_acl_qualifier_mask_t

union_fields=$(cat inc/db_acl_qualifier_masks.h  | grep \\\} | tr '}' ' '  | tr ';' ' '  | cut -f2 -d ' ')

echo "
#ifndef __${target_file}__
#define __${target_file}__

typedef union {
" > $filen

for i in $union_fields ; do
    fieldn=$(echo $i | sed -e 's/db_acl_//g' | sed -e 's/_t//g')
    echo "  $i ${fieldn};" >> $filen
done

echo "
} $union_name;

#endif" >> $filen
