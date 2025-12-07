#!/bin/bash

pushd ANOMALY_V4
git_rev=$(git rev-list --count HEAD)
echo git_rev=$git_rev
popd

if [[ $# -lt 1 ]]; then
    echo
    echo Usage: $0 tar_file_prefix
    echo
    exit 1
fi

echo

tar_file_prefix=$1

tar_file=${tar_file_prefix}_rev_${git_rev}_$(date +"%Y_%m_%d__%H_%M").tar.gz

echo Will tar off ANOMALY_V4 to date/hour-stamped tar.gz file $tar_file
echo
read -p "Is this OK? " yn
echo
if [[ $yn == y ]] || [[ $yn == Y ]]; then
	tar czvf $tar_file ANOMALY_V4
else
    echo
    echo ANOMALY_V4 not archived.
    echo
fi

