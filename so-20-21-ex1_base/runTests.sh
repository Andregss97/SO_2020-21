#!/bin/bash
NumThreads=$((1+RANDOM%$3))
mkdir $2
for input in inputs/*.txt
do
 echo InputFile=${input} NumThreads=$NumThreads

 file=${input##*/}
 file=${file%.*}
 output=$2/$file-$NumThreads

 ./tecnicofs $input $output $NumThreads
done