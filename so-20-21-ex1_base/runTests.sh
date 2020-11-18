#!/bin/bash
 
mkdir $2
for input in $1/*.txt
do
 for NumThreads in $(seq 1 $3)
 do
  echo InputFile = ${input} NumThreads = $NumThreads

  file=${input##*/}
  file=${file%.*}
  output=$2/$file-$NumThreads
 
  ./tecnicofs $input $output $NumThreads | tail -n 1
  
 done
done