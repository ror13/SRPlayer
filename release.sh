#!/bin/sh

./gradlew build || return 1
mkdir -p release/app || return 1
mkdir -p release/lib || return 1

cp app/build/outputs/apk/* release/app/
cp app/src/main/libs/armeabi-v7a/* release/lib

tar -cvzf release.tar.gz release

rm -r ./release


