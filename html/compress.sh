#!/bin/bash
src=html
dest=compressed

if [[ -d $dest ]]
then
	echo remove old direcotry
	rm -rf $dest
fi

echo create direcotry \"$dest\"
mkdir $dest
echo "copy files"
cp -r $src $dest
echo "compresing $1";
find $dest -type f -exec gzip "{}" \; -exec mv "{}.gz" "{}" \;
echo "done"
