#! /bin/sh

# merge new lines to existing .po files
po_files=($(find -E . -type f -regex "^.*po$"))
for po_input in ${po_files[*]}
do
  mo_output="./build/${po_input%po}mo"
  mkdir -p "${mo_output%/*}"
  printf "msgfmt %s -o %s\n" $po_input $mo_output 
  msgfmt $po_input -o $mo_output
done 
