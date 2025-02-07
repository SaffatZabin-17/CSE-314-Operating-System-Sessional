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
        extension="${filename##*.}"
        if [ "$extension" = "c" ] || [ "$extension" = "java" ] || [ "$extension" = "py" ]; then
            echo "$filename"
        fi
	fi
}

search_file()
{
    local directory="$1"
    local file=""
    
    file=$(find "$directory" -type f \( -name "*.c" -o -name "*.java" -o -name "*.py" \) -print -quit)
    
    if [ -n "$file" ]; then
        echo "$file"
    fi
}

verbose=false
exec=true

if [ "$5" = "-v" ]; then
    verbose=true
fi

if [ "$6" = "-noexecute" ]; then
    exec=false
fi

print_verbose()
{
    if [ "$verbose" = true ]; then
        echo "$1"
    fi
}

count=0

for file in $3/*.txt
    do  
        count=$(($count + 1))
done

print_verbose "Found $count test files" 


cd "$1"          #Inside -> workspace/submissions

for file in *.zip
  do
    mkdir unzipped
    filename="$file"
    file_name=${filename%.zip}
    student_id=${file_name: -7}
    print_verbose "Organizing files of $student_id"
    unzip "$file" -d unzipped >/dev/null
    cd unzipped                     #Inside-> workspace/submissions/unzipped
    name=`visit *`
    #echo $student_id     #1805xxx
    #echo $name           #sth.x x=c, java, py
    extension="${name##*.}"
    name=${name%.*}
    #echo $extension
    cd ..                           ##Inside-> workspace/submissions
    if [ $extension = "c" ]; then
        extension="C"
        cd ..                       #Inside-> workspace
        cd "$2"/C                #Inside-> workspace/targets/C
    elif [ $extension = "java" ]; then
        extension="Java"
        cd ..                       #Inside-> workspace
        cd "$2"/Java             #Inside-> workspace/targets/Java
    else
        extension="Python"
        cd ..                       #Inside-> workspace
        cd "$2"/Python           #Inside-> workspace/targets/Python
    fi
    mkdir "$student_id"
    file_path_2=""
    cd "$student_id"                #Inside-> workspace/targets/<extension>/<student_id>
    if [ $extension = "C" ]; then
        touch main.c
        file_path_2="$2/$extension/$student_id/main.c"
    elif [ $extension = "Java" ]; then
        touch Main.java
        file_path_2="$2/$extension/$student_id/Main.java"
    else 
        file_path_2="$2/$extension/$student_id/main.py"
        touch main.py
    fi    
    cd ..                           #Inside-> workspace/targets/<extension>
    cd ..                           #Inside-> workspace/targets
    cd ..                           #Inside-> workspace
    file_path_1=$(search_file "$1")
    #echo $file_path_1
    #echo $file_path_2
    cp "$file_path_1" "$file_path_2"
    cd submissions                  #Inside-> workspace/submissions
    rm -r unzipped
done

#       ============================================================== File organization done ===========================================================================

cd ..                               #Inside-> workspace

if [ "$exec" = true ]; then

    for file in $3/*.txt
        do  
            for entry in targets/C/*
            do  
                cp "$file" "$entry"
            done
            for entry in targets/Java/*
            do  
                cp "$file" "$entry"
            done
            for entry in targets/Python/*
            do  
                cp "$file" "$entry"
            done
    done

    for file in $4/*.txt
        do  
            for entry in targets/C/*
            do  
                cp "$file" "$entry"
            done
            for entry in targets/Java/*
            do  
                cp "$file" "$entry"
            done
            for entry in targets/Python/*
            do  
                cp "$file" "$entry"
            done
    done


    cd targets                  # Inside workspace/targets

    touch result.csv
    echo "student_id,type,matched,not_matched" > "result.csv"
    match_count=0
    non_match_count=0

    for entry in *; do
        if [ "$entry" = "C" ]; then
            cd C                                  # Inside workspace/targets/C
            for c_entry in *; do
                print_verbose "Executing Files of $c_entry"
                cd "$c_entry"                     #Inside-> worksapce/targets/C/<student_id>
                g++ main.c -o main.out
                for test_file in test*.txt; do
                    output_file="out${test_file#test}"
                    ./main.out < "$test_file" > "$output_file"
                    rm "$test_file"
                done
                for((i=1; i<=count; i++))
                    do 
                        answer_file="ans${i}.txt"
                        output_file="out${i}.txt"
                        
                        match_status=$(diff "$answer_file" "$output_file")
                        if [ -z "$match_status" ]; then
                            match_count=$(($match_count + 1))
                        fi
                done
                non_match_count=$(($count-$match_count))
                for ans_file in ans*.txt; do
                    rm "$ans_file"
                done
                cd ..                             # Inside workspace/targets/C
                cd ..                             #Inside-> worksapce/targets
                echo "$c_entry,C,$match_count,$non_match_count" >> "result.csv"
                match_count=0
                non_match_count=0
                cd C
            done
            cd ..                                 # Inside workspace/targets
        elif [ "$entry" = "Java" ]; then
            cd Java
            for java_entry in *; do
                print_verbose "Executing Files of $java_entry"
                cd "$java_entry"
                javac Main.java
                for test_file in test*.txt; do
                    output_file="out${test_file#test}"
                    java Main < "$test_file" > "$output_file"
                    rm "$test_file"
                done
                for((i=1; i<=count; i++))
                    do 
                        answer_file="ans${i}.txt"
                        output_file="out${i}.txt"
                        
                        match_status=$(diff "$answer_file" "$output_file")
                        if [ -z "$match_status" ]; then
                            match_count=$(($match_count + 1))
                        fi
                done
                non_match_count=$(($count-$match_count))
                for ans_file in ans*.txt; do
                    rm "$ans_file"
                done
                cd ..                             # Inside workspace/targets/Java
                cd ..                             #Inside-> worksapce/targets
                echo "$java_entry,Java,$match_count,$non_match_count" >> "result.csv"
                match_count=0
                non_match_count=0
                cd Java
            done
            cd ..
        elif [ "$entry" = "Python" ]; then
            cd Python
            for python_entry in *; do
                print_verbose "Executing Files of $python_entry"
                cd "$python_entry"
                for test_file in test*.txt; do
                    output_file="out${test_file#test}"
                    python3 main.py < "$test_file" > "$output_file"
                    rm "$test_file"
                done
                for((i=1; i<=count; i++))
                    do 
                        answer_file="ans${i}.txt"
                        output_file="out${i}.txt"
                        
                        match_status=$(diff "$answer_file" "$output_file")
                        if [ -z "$match_status" ]; then
                            match_count=$(($match_count + 1))
                        fi
                done
                non_match_count=$(($count-$match_count))
                for ans_file in ans*.txt; do
                    rm "$ans_file"
                done
                cd ..                             # Inside workspace/targets/Python
                cd ..                             #Inside-> worksapce/targets
                echo "$python_entry,Python,$match_count,$non_match_count" >> "result.csv"
                match_count=0
                non_match_count=0
                cd Python
            done
            cd ..
        fi
    done
    cd ..                                           # Inside workspace
fi

#  ==============================================================Code COmpilation done================================================================
