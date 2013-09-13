#!/bin/bash
#
#Script to generate trick play files for specified file(s) and speeds
#
#
app_dir=/usr/local/bin
app="indexVideo"
src_dir=""
#des_dir="./tp_files/"
res_dir="Resource.0"
#
#
cmd_array=( "$@" )
agr_count="$#"
arglist=("$@")
tp_speed=1
item_info=""

function get_item_info()
{
	fullname=$1
	fname=$(basename $fullname)
	item_info="${fname%.*} reference file with play speed support"
}

function add_file_resource_info()	#input required is the path, file name, source file name
{
	fullname=$3
	fname=$(basename $fullname)
	echo "basename=${fname%.*}" > $1/$2
	echo "uri=" >> $1/$2
	echo "profile=" >> $1/$2
	echo "mime-type=" >> $1/$2
	echo "protected=false" >> $1/$2
	echo "converted=false" >> $1/$2
	echo "bitrate=" >> $1/$2
	echo "audio-channels=" >> $1/$2
	echo "audio-sample-frequency=" >> $1/$2
	echo "audio-bits-per-sample=" >> $1/$2
	echo "video-resolution=" >> $1/$2
	echo "video-color-depth=" >> $1/$2
	echo "in-progress=" >> $1/$2
	echo "time-window=" >> $1/$2
	echo "byte-window=" >> $1/$2
}



function tpgen()
{	#this function is called with two arguments
tp_speed=$2
echo " " 
echo "Generating files for speed: $tp_speed, file type: $1"
dir=''
for FILE in $src_dir/*.$1
	do
		#use directory "./ndx" as a default directory for generating binary index file
		dir=ndx
		if [ -d "$dir" ] ; then
			rm -rf $dir
		#	echo "Removing directory: $dir"
		fi
		#
		# create directory structure in the destination directory 
		# with name FILE.item and sub-directory Resource.0	
		#
		#echo "Creating directory: $FILE.item"
		dir=$FILE.item
		if [ ! -d "$dir" ]; then
			mkdir $FILE.item
			echo "Creating directory: $dir"
		fi
		#
		# create/write item.info file in this directory
		get_item_info $FILE
		#echo "title=$item_info"
		echo "title=$item_info " > $FILE.item/item.info
		#
		#
		#echo "Creating directory: $FILE.item/$res_dir"
		dir="$FILE.item/$res_dir"
		if [ ! -d "$dir" ]; then
			mkdir $FILE.item/$res_dir
			echo "Creating directory: $dir"
		fi
		final_dest=$FILE.item/$res_dir
		#
		# generate resource.info file
		#
		add_file_resource_info $final_dest "resource.info" $FILE
		#
		echo "Indexing file:" $FILE
		$app_dir/$app $FILE ndx >> tpgenlog.txt
		$app_dir/$app $FILE ndx/0000000000.ndx $tp_speed $final_dest >> tpgenlog.txt
		echo " " >> tpgenlog.txt
		echo "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!" >> tpgenlog.txt
		echo " " >> tpgenlog.txt
	done
}

function tpgen_all()
{
# process all .ts and .mpg files for specified speeds

for arg in "${arglist[@]:1}"; do

	mpg_count=0
	ts_count=0
	echo " "
	for file in ${src_dir}/*.mpg
	do
		if [ -f "${file}" ]; then
			mpg_count=$((mpg_count+1))
		fi	
	done
	
	for file in ${src_dir}/*.ts
	do
		if [ -f "${file}" ]; then
			ts_count=$((ts_count+1))
		fi	
	done
	
	if [ "$mpg_count" -gt 0 ] ; then 
		echo "processing $mpg_count .mpg files"
		tpgen mpg $arg
	fi
	
	if [ "$ts_count" -gt 0 ] ; then 
		echo "processing $mpg_count .ts files"
		tpgen ts $arg
	fi
done
}

function usage()
{
	echo " "
	echo "Usage: $0 src_dir/ <speed> <speed>"
	echo "where: src_dir is the directory path where stream files are located"
	echo "       <speed> can be multiple speeds in range <-100 to -1> or <1 to 100>"
	echo "example: 								"
	echo "          ./tpgen2 ./streams/ 1 2 4 8"
	exit 1;
}

#------------------------
# main 
#------------------------

[[ $# -lt 2 ]] && usage

#-------------------------
#source directory/file name
if [ -n "$1" ]
then
	src_dir="$1"
	echo "------------------------------"
	echo "source directory is: $src_dir"
	echo "------------------------------"
fi

tpgen_all



