#!/bin/bash -x
#
#Script to generate trick play files for specified file(s) and speeds
#
#---------------------------------------
# variables
#---------------------------------------
app_dir=$CVP2_ROOT/bin
app="indexVideo"
src_dir=""
des_dir=""
res_dir="Resource.0"
mime_type="Unknown"
item_dir=""
profile="Unknown"
profile_found=no
item_info=""
file_count=0
#
#
cmd_array=( "$@" )
agr_count="$#"
arglist=("$@")
tp_speed=1
file_type="mpg"
#
#

array_p=""
function read_file_inarray()
{
while read line
do
    array_p+=("$line")
done < $1
}

function validate_profile()
{
#echo "Validating profile: $1"
test_string=$1
profile_found="no"
mime_type="Unknown"
match_index=
for ((i=0; i < ${#array_p[*]}; i++))
do
    #echo "${array_p[i]}"
	if [[ "${array_p[i]}" == *mime_type* ]]; then
		tmp="${array_p[i]}"
		mime_type=${tmp#mime_type=}
		#echo "mime found: $mime_type"
		#echo "$mime_type"
	fi
	if [ "${array_p[i]}" = $test_string ]; then
		match_index=$i
		profile_found="yes"
		break
	fi
done
if [ "$profile_found" = "no" ]; then 
	echo "Sorry the profile was not found !!!"
else
	echo "Valid Profile: ${array_p[match_index]} , mime_type=$mime_type" 
fi
}

function add_file_resource_info()	#input required is the path, file name, source file name
{
	#fullname=$3
	#fname=$(basename $fullname)
	#echo "basename=${fname%.*}" > $1/$2
	echo "basename=$3" > $1/$2
	echo "profile=$profile" >> $1/$2
	echo "mime-type=$mime_type" >> $1/$2
	echo "protected=false" >> $1/$2
	echo "converted=false" >> $1/$2
	#echo "bitrate=" >> $1/$2
	#echo "audio-channels=" >> $1/$2
	#echo "audio-sample-frequency=" >> $1/$2
	#echo "audio-bits-per-sample=" >> $1/$2
	#echo "video-resolution=" >> $1/$2
	#echo "video-color-depth=" >> $1/$2
	#echo "in-progress=" >> $1/$2
	#echo "time-window=" >> $1/$2
	#echo "byte-window=" >> $1/$2
	#echo "uri=" >> $1/$2

}

function remove_dir()
{
	dir=$1
	if [ -d "$dir" ] ; then
		#echo "Removing directory: $dir"		
		rm -rf $dir > /dev/null 2>&1
	fi
}

found_item_dir=no
function find_directory()
{
item_dir=""
pattern=$2
dir=def$3_profile
for _dir in $1/*"${pattern}"*; do
    [ -d "${_dir}" ] && dir="${_dir}" && found_item_dir=yes && break
done
item_dir="$dir"
}


function tpgen()
{	#this function is called with two arguments
tp_speed=$2
file_type=$1
echo " " 
echo "Generating files for speed: $tp_speed, file type: $1"
dir=''
for FILE in $src_dir/*.$1
	do
		((file_count++))
		tmp=${FILE// /_}
		full_filename=$(basename $tmp)
		base_filename="${full_filename%.*}"
		extension="${full_filename##*.}"
		base_filename=${base_filename//./_} 
		full_filename=${full_filename//./_} 
		echo "-----------------------------"
		echo "File_$file_count: $FILE"
		echo "-----------------------------"
		# use directory "./ndx" as a default temporary directory for generating binary index file
		# remove the ndx directory for clean start, it will be recreated
		remove_dir "ndx"
		# Create media directory
		if [ ! -d "$media_dir" ]; then
			mkdir $media_dir
			echo "Creating media directory: $media_dir"
		fi
		#-----------------------------------
		#
		# create directory structure in the destination directory 
		# with name FILE.item and sub-directory Resource.0	
		#
		# check if the directory containing the name exist
		# echo "Creating directory: $FILE.item"
		#
		# create/write item.info file in this directory
		#
		ref_name="$profile"_"$base_filename"

		dir=$media_dir/$ref_name
		item_ref_file=$dir/$ref_name.item

		if [ ! -d "$dir" ]; then
			mkdir -p $dir
			echo "Creating directory: $dir"
		fi

                #
		# Create item ref file
		#----------------------------------
		# 
		now="$(date +'%Y-%m-%dT%T%Z')"	#date format: 2013-08-09T21:35:09Z
		echo "Writing to ref file: $dir/$item_ref_file"
		echo "[item]" > $item_ref_file
		echo "title=$base_filename" >> $item_ref_file
		echo "date=$now" >> $item_ref_file	
		
		#
		#
		if [ "$profile_found" = "yes" ]; then 
			dir="$media_dir/$ref_name/$profile.0"
		else
			dir="$media_dir/$ref_name/$res_dir"
		fi
		echo "Check for directory: $dir"
		if [ ! -d "$dir" ]; then
			mkdir $dir
			echo "Creating directory: $dir"
		fi
		final_dest=$dir
		#
		# generate resource.info file
		#
		add_file_resource_info $final_dest "resource.info" $base_filename
		#
		# check for mp4, no indexing done for mp4 files
		#
		#
		if [[ $mime_type == *mp4* ]]; then
			echo "mime_type: $mime_type"
			echo "File type is MP4, indexing is not done for mp4"
			echo "just the original stream file will be copied"
			cp -f $FILE $final_dest/$base_filename.1_1.$extension
		else
			echo "Indexing file:" $FILE
			cp -f $FILE $final_dest/$base_filename.$extension
			NEW_FILE="$final_dest/$base_filename.$extension"
			$app_dir/$app $NEW_FILE ndx >> tpgenlog.txt
			$app_dir/$app $NEW_FILE ndx/0000000000.ndx $tp_speed $final_dest >> tpgenlog.txt
			if [ "$tp_speed" = "1" ] ; then
				mv $NEW_FILE $final_dest/$base_filename.1_1.$extension
			else
				rm -f $NEW_FILE
			fi
			#
			#following commented out code uses the original file name for indexing
			#$app_dir/$app $FILE ndx >> tpgenlog.txt
			#$app_dir/$app $FILE ndx/0000000000.ndx $tp_speed $final_dest >> tpgenlog.txt
			#if [ "$tp_speed" = "1" ] ; then
			#	cp -f $FILE $final_dest/$base_filename.1_1.$extension
			#fi
			#
		fi
		echo "-----------------------------------------"
		echo " " >> tpgenlog.txt
		echo "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!" >> tpgenlog.txt
		echo " " >> tpgenlog.txt
		remove_dir "ndx"
	done
}




function tpgen_all()
{
# process stream files for specified speeds
for arg in "${arglist[@]:$1}"; do
	ext_list=(ts mpg mp4 TS MPG MP4)
	for extn in ${ext_list[@]}; do	
		count=0
		for file in ${src_dir}/*.$extn; do
			if [ -f "${file}" ]; then
				count=$((count+1))
			fi	
		done
		if [ "$count" -gt 0 ] ; then 
			tpgen $extn $arg
		fi
	done
done
}


function process_profiles()
{
_dir=$1
for dir in $_dir/*/
do
    dir=${dir%*/}
    profile=${dir##*/}
	echo " "
	echo "processing $profile"
	validate_profile $profile
	if [ "$profile_found" = "yes" ]; then 
		# mime_type is already found and assigned at this point
		src_dir=$_dir/$profile
		tpgen_all $2
	else
		echo "Invalid Profile, skip processing for this directory" 
	fi
done
}


function usage()
{
	echo " "
	echo "Usage: $0 src_dir/ dest_dir/ [-p ./profiles.txt] <speed> <speed>"
	echo "where: src_dir/  is the directory path where stream files are located"
	echo "       dest_dir/ is the directory path where media directories/files will be generated"
	echo "       option [-p ./profiles.txt]" 
	echo "       if the -p is specified, then './profiles.txt' must be provided"
	echo "       './profile.txt' is the text file containing the valid profiles and mime types information"
	echo "       <speed> can be multiple speeds in range <-100 to -1> or <1 to 100>"
	echo "example: 								"
	echo "       ./tpgen2 ./streams/ ./media -p ./profile.txt 1 2 4 8"
	echo "       ./tpgen2 ./streams/ ./media 1 2 4 8"
	echo " Note: if -p option is not specified then only the media files located in src_dir/ will be processed."
	echo "       if -p option is specified then only the media files contained in sub-directories with valid profile name"
	echo "       as subdirectory name will be processed. Valid profile names are contained in the 'profile.txt' file."
	exit 1;
}

#------------------------
# main execution
#------------------------

[[ $# -lt 3 ]] && usage

#---------------------------
#source directory/file names
#---------------------------
if [ -n "$1" ] && [ -n "$2" ]; then
	main_dir="$PWD"
	src_dir="$1"
	media_dir="$2/media"

	mkdir -p "$2"

	echo "-------------------------------------------"
	echo " main_dir: $main_dir"
	echo "  src dir: $src_dir"
	echo " dest_dir: $media_dir"
	echo " refs_dir: $refs_dir"
	echo "--------------------------------------------"
fi

if [[ $# -ge 5 ]] && [[ "$3" == "-p" ]]; then
	echo "-p option specified"
	read_file_inarray $4
	if [[ ${#array_p[*]} -ge 1 ]]; then 
			process_profiles $1 4	#speed is specified in argument index 4
	else
		echo "No valid profiles found in file: $4"
	fi
else
	echo "-p option not specified, processing stream files without profile information."
	tpgen_all 2		#speed is specified in argument index 1
fi
