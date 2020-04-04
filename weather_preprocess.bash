#!/bin/bash


for f in data/weather/*.TXT; do 
    j="${f%.*}"
    grep "^\[" $f | sed 's/\[//g' | sed 's/\]/,/g' | sed '$ s/.$//' | sed '$ s/.$//'  > $j.json
    sed -i '1 i\\[' $j.json
    echo "]" >> $j.json 
    echo $j.json
done
