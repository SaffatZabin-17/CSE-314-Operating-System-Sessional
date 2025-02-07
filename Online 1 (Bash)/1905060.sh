#!/bin/bash

visit()
{
        if [ -d "$1" ]
        then
                for i in "$1"/*
                do
                        visit "$i"
                done
        elif [ -f "$1" ]
        then
                filename=$(basename "$1")
                echo "$1"
                string_size=${#$filename}
                cd output_dir
                mkdir "$string_size"
                cp $1 output_dir/"$string_size"
        fi
}

search_files()
{
        local directory="$1"
        local file=""

        file=$(find "$directory" -type f -print -quit)

        if [ -n "$file" ]; then
                echo "$file"
        fi
}

mkdir output_dir

file_name=`visit $1`
echo "$file_name"

echo "=================================================================="
file_path=`search_files $1`
echo "$file_path"


