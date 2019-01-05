#!/bin/bash

#Note to TA:
#*sometimes* the 20 point matrix is too slow, sometimes it's not
#it does work, though!
###########################################
#Program 1
#CS 344, Fall 2018
#Sara Hovey
###########################################

###########################################
#Sources~
#https://stackoverflow.com/questions/6980090/how-to-read-from-a-file-or-stdin-in-bash
#https://bash.cyberciti.biz/guide/$IFS
#https://stackoverflow.com/questions/2990414/echo-that-outputs-to-stderr
#https://www.tldp.org/LDP/Bash-Beginners-Guide/html/sect_10_02.html
#https://www.gnu.org/software/bash/manual/bashref.html#Shell-Parameter-Expansion
###########################################

###########################################
#Helper functions
###########################################

###########################################
#Check the validity of one file
###########################################
file_check_single(){
    #Check if the file is found
    if [ ! -f $1 ]; then
        >&2 echo "ERROR: file not found"
        exit 1
    elif  ! [ -r $1 ]; then
        >&2 echo "ERROR: file not found"
        exit 1
    fi
}
###########################################
#Check the validity of two files
###########################################
file_check_double(){
    #Check if the file is found
    if [ ! -f $1 ] || [ ! -f $2 ]; then
        >&2 echo "ERROR: file not found"
        exit 1
    fi
}

###########################################
#Dims: prints the dimensions
#of the given matrix as
#[rows columns]
###########################################
dims() {
  rows=0
  cols=0
  #I had been using ( cat $this_file | wc -l) and ( head -n 1 $this_file | wc -w) but it was finicky
  
  #Iterate through the matrix
  #An incrementation on the outer loop represents a row
  #an incrementation on the inner loop represents a column
  while IFS="$(printf '\t')" read line
  do
    rows=$(($rows + 1))
    
    if [ $cols = 0 ]
    then
      for i in $line
      do
        cols=$(($cols + 1))
      done
    fi
  done < "${1:-/dev/stdin}" #This takes input from either args or stdin~
  #Print out the results
  echo "$rows $cols"
  exit 0

#     columns=$( head -n 1 $1 | wc -w)
#     rows=$( cat $1 | wc -l)

}

###########################################
#Transpose: reflect a given matrix across
#its diagonal, return the new matrix
###########################################
transpose() {
#Make a new array
    #originally tried something like this
    #mapfile < $1 lines1
    #but it was a PIA with stdin data
    #Looks cool though
    declare -A arr
    
    #Get the dimensions of the matrix to transpose
    these_dims=$(dims $1)
    rows=$(echo "$these_dims" | cut -f 1 -d " ")
    cols=$(echo "$these_dims" | cut -f 2 -d " ")
    
    i=0
    #Read from the input matrix
    while IFS="$(printf '\t')" read line
    do
    j=0
    for element in $line
    do
      #Populate array with data from the input file
      arr[$j, $i]=$element
      j=$(($j + 1))
    done
    #increment i, representing rows
    i=$(($i + 1))
    done < "${1:-/dev/stdin}" #Gets input from args or stdin

    original_rows=$i
    original_cols=$j
    
    #We can transpose the matrix by echoing it
    #in column-major order instead of row-major
    for j in $( seq 0 $(($original_cols - 1 )))
    do
      line=
      for i in $( seq 0 $(($original_rows)))
      do
        num=${arr[$j, $i]}
        line="$line$num\\t"
      done
      
      #remove trailing space
      line=${line::-4}
      echo -e $line
    done
    exit 0
}

###########################################
#Mean: given a matrix, return the mean of 
#each row as a row vector
###########################################
mean() {

  #build up a string
  #this string will be a row of sums from which
  #to take the average in the next loop
  i=0
  result=
  while IFS="$(printf '\t')" read line
  do
    j=1
    sums_list=
    for element in $line
    do
      sums_list[$j]=$((${sums_list[$j]} + $element))
      j=$(($j + 1))
    done
    i=$(($i + 1))
  done < "${1:-/dev/stdin}" #Get input from args or stdin

  #Get the dimesions to see how to take the avg
  these_dims=$(dims $1)
  rows=$(echo "$these_dims" | cut -f 1 -d " ")
  cols=$(echo "$these_dims" | cut -f 2 -d " ")
  
  #for every number in that line, find the avg
  for idx in $(seq 1 $(($j - 1)))
  do
    cur_sum=0
    if [ ${sums_list[$idx]} -gt 0 ];then
      cur_sum=1
    fi
    #Rounding z o n e
    #(a +(b/2)*((a>0)*2-1 )) / b
    cur_val=${sums_list[$idx]}
    #i can't get this working without escaping the * and using expr near the divs and mults :<
    cur_sum=$(expr $(( $(expr $(($(expr $cur_sum \* 2) - 1)) \* $(expr $i / 2)) + $cur_val )) / $i)
    
    #append the sum to the current running result
    result="$result$cur_sum\\t"
  done
  
  #remove trailing whitespace
  result=${result::-2}
  echo -e $result
  exit 0
}

###########################################
#Add: add two matrices element-wise. This
#produces a new matrix of each result
###########################################
add() {

  #Make a call to dims for each matrix we are getting as input
  dims1=$(dims $1)
  dims2=$(dims $2)


  #If the matrices have the same dimensions, they are compatible
  #Use the values we got back from dims to compare
  if [ "$dims1" = "$dims2" ]
  then
    #Add these puppies
    paste -d , $1 $2 | while IFS="$(printf ',')" read -r m1 m2
    do
      #make this new string
      #we're going to append to it as we go
      cur_line=""
      idx=0
      matrix1=($m1)
      matrix2=($m2)
  
      #For each number in the size of a matrix,
      #add the numbers at that element and append it to
      #the cur_line variable
      for element in $m1
      do
        #get the values at the current index
        cur_sum=$((${matrix1[idx]} + ${matrix2[idx]}))
        #put a tab after them
        cur_line="$cur_line$cur_sum\\t"
        #increment
        idx=$(($idx + 1))
      done
      
      #make a substring
      cur_line=${cur_line::-2}
      echo -e $cur_line
    done
  else
    >&2 echo "ERROR: incompatible matrices"
    exit 1
  fi
}

###########################################
#Multiply: perform matrix multiplication on
#two given, compatible matrices
#compatible matrices are such that
#m1 rows = m2 columns and
#m1 cols = m2 rows
###########################################
multiply() {
  #We will load the two matrices into memory from the file
  #this helps cut down on file i/o
  declare -A matrix1
  declare -A matrix2

  #By getting the rows and columns of each,
  #we can determin the dimensions of the 
  #destination matrix
  these_dims1=$(dims $1)
  rows1=$(echo "$these_dims" | cut -f 1 -d " ")
  cols1=$(echo "$these_dims" | cut -f 2 -d " ")
  
  these_dims2=$(dims $1)
  rows2=$(echo "$these_dims" | cut -f 1 -d " ")
  cols2=$(echo "$these_dims" | cut -f 2 -d " ")
  
  #Load the matrices into memory
  #store them in 2 arrays
  row_index=0
  #first array
  while IFS="$(printf '\t')" read line
  do
    col_index=0
    for cell in $line
    do
      matrix1[$row_index, $col_index]=$cell
      col_index=$(expr $col_index + 1)
    done
    row_index=$(expr $row_index + 1)
  done < $1 #only taking data from args here
  rows1=$row_index
  cols1=$col_index

 
  row_index=0
  #second array
  while IFS="$(printf '\t')" read line
  do
    col_index=0
    for cell in $line
    do
      matrix2[$row_index, $col_index]=$cell
      col_index=$(expr $col_index + 1)
    done
    row_index=$(expr $row_index + 1)
  done < $2
  
  
  rows2=$row_index
  cols2=$col_index

  #Make sure the matrices are compatible
  if [ $rows2 != $cols1 ];then
    >&2 echo "ERROR: these matrices are not compatible"
    exit 1
  fi

  #Multiplication is here
  #Now that we've loaded the data into memory, we can do the math
  #Iterate over the rows of the first matrix
  for i in $(seq 0 $(($rows1 - 1)))
  do
    #iterate over the cols of the second
    #there are three loops so that we can "explore" the entirety of both matrices
    #If they are not squares, they will each have a "long" side
    #the k index can be used to index into the longe side of a matrix
    output=
    for j in $(seq 0 $(($cols2 - 1)))
    do
      #reset sum to get each dot product
      #I had to watch the khan academy matrix multiplication video for this :(
      cur_sum=0
      #triple loop ahhh
      for k in $(seq 0 $(($cols1 - 1)))
      do
        #get the elements that will be multiplied together
        val1=${matrix1[$i, $k]}
        val2=${matrix2[$k, $j]}
        
        #get the product of the two
        cur_product=$(expr $val1 \* $val2) #i can't get this working without escaping the * and using expr :<
        #add the new product to the cur_sum
        #this will clear out when we move over to a new row
        #or column that is being multiplied
        cur_sum=$(($cur_sum + $cur_product))
      done
      output="$output$cur_sum\\t"
    done
    
    #get rid of trailing space
    output=${output::-2}
    echo -e $output
  done

}

###########################################
#Arg validation in advance of calling the 
#functions
#this is a little cleaner than
#checking in each function
###########################################
#General checks, then the more specific ones
#Check for too many/too few
if [ $# -gt 3 ];then
    >&2 echo "ERROR: too many params"
    exit 1
fi

if
    [ $# -lt 1 ];then
    >&2 echo "ERROR: too few params"
    exit 1
fi
###########################################
#Functions that take one file as input
###########################################
func=$1
if [ $func == "dims" ] || [ $func == "transpose" ] || [ $func == "mean" ];then
    if [ $# -gt 1 ];then
        file_check_single $2
    fi
    if [ $# -gt 2 ];then
        >&2 echo "ERROR: too many params"
        exit 1
    fi
    
    $1 "${@:2}"
###########################################    
#The others
###########################################
elif [ $func == "add" ] || [ $func == "multiply" ];then
  #Check the validity of the files~      
  if [ $# -gt 1 ];then
      file_check_double $2 $3
  fi
  #check for too few arguments for the things that take 2 inputs
  if [ $# -lt 2 ];then
    >&2 echo "ERROR: too few params"
        exit 1
  fi
  
  $1 "${@:2}"
else
    >&2 echo "ERROR: bad command"
    exit 1
fi


trap 'Signal detected...' SIGINT SIGQUIT SIGTERM SIGHUP