#!/bin/bash

VERSION=$(git describe | sed -e "s/^v//")
echo "echo \"$VERSION\" > .tarball-version"
echo 'touch --date="$(git log -n 1 --pretty=format:%ci)" .tarball-version'
echo "tmp_tar=\"$(mktemp)\""
echo "git archive --format=tar --prefix=neoglogg-$VERSION/ v$VERSION >\$tmp_tar"
echo "tmp_dir=\"$(mktemp -d)\""
echo "tar xf \$tmp_tar --directory \$tmp_dir"
echo "cp -p .tarball-version \$tmp_dir/neoglogg-$VERSION/"
echo "tar czf neoglogg-$VERSION.tar.gz --directory \$tmp_dir neoglogg-$VERSION/"
echo "rm .tarball-version \$tmp_tar"
