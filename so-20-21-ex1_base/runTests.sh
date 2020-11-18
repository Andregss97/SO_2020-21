#!/bin/bash
GREEN='\033[0;32m'
NC='\033[0m'
 
mkdir $2
for input in $1/test2.txt
do
 for NumThreads in $(seq 1 $3)
 do
  printf "${GREEN}--------------------------------------------------------------------\n\n"
  echo InputFile = ${input} NumThreads = $NumThreads
  printf "\n--------------------------------------------------------------------\n${NC}"

  file=${input##*/}
  file=${file%.*}
  output=$2/$file-$NumThreads
 
  ./tecnicofs $input $output $NumThreads
 done
done